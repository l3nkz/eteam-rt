#include "measure.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <eteam.h>

#include <unistd.h>
#include <fcntl.h>

#include "process.h"
#include "energy.h"
#include "time.h"


Measure* Measure::measure_with(MeasureType type, Process *proc)
{
    switch (type) {
        case NONE:
            return new detail::NoMeasure{proc};
        case ETEAM:
            return new detail::ETeamMeasure{proc};
        case MSR:
            return new detail::MSRMeasure{proc};
        default:
            throw std::invalid_argument{"Invalid measurement type"};
    }
}

std::string Measure::measure_name(MeasureType type)
{
    switch (type) {
        case NONE:
            return detail::NoMeasure::name;
        case ETEAM:
            return detail::ETeamMeasure::name;
        case MSR:
            return detail::MSRMeasure::name;
        default:
            throw std::invalid_argument{"Invalid measurement type"};
    }
}

void Measure::update_times()
{
    if (!_proc->valid())
        return;

    Time cur_proc_time = _proc->time();
    double delta_s = (cur_proc_time.user + cur_proc_time.system - cur_proc_time.looped) - \
                     (_last_proc_time.user + _last_proc_time.system - _last_proc_time.looped);

    if (_running)
        _measured += delta_s;
    else
        _not_measured += delta_s;

    _last_proc_time = cur_proc_time;
}

Measure::Measure(Process *proc) :
    _running{false}, _proc{proc}, _last_proc_time{0},
    _measured{0}, _not_measured{0}
{}

bool Measure::start()
{
    if (_running)
        return true;

    if (!_proc->valid() || _proc->finished())
        return false;

    update_times();

    if (this->start_()) {
        _running = true;
        return true;
    } else {
        return false;
    }
}

bool Measure::stop()
{
    if (!_running)
        return true;

    if (!_proc->valid())
        return false;

    update_times();

    if (this->stop_()) {
        _running = false;
        return true;
    } else {
        return false;
    }
}

void Measure::reset()
{
    if (_proc->valid()) {
        _last_proc_time = _proc->time();
    } else {
        _last_proc_time = {};
    }

    _measured = 0;
    _not_measured = 0;

    this->reset_();
}

double Measure::rate()
{
    update_times();

    if ((_measured + _not_measured) == 0)
        return 0;

    return _measured / (_measured + _not_measured);
}

namespace detail {

const std::string NoMeasure::name = {"none"};


const std::string ETeamMeasure::name = {"eteam"};

ETeamMeasure::ETeamMeasure(Process *proc) :
    Measure{proc}
{}

bool ETeamMeasure::start_()
{
    return start_energy(this->_proc->pid()) == 0;
}

bool ETeamMeasure::stop_()
{
    if (this->_proc->finished())
        return true;

    return stop_energy(this->_proc->pid()) == 0;
}

void ETeamMeasure::reset_()
{
    this->_running = false;
}

Energy ETeamMeasure::energy()
{
    Energy e;

    if (!_proc->valid())
        return e;

    std::stringstream path;
    path << "/proc/" << _proc->pid() << "/energystat";

    std::ifstream estat{path.str(), std::ios::in};

    if (estat.is_open()) {
        estat >> e.package >> e.dram >> e.core >> e.gpu >> e.loops;

        estat.close();
    }

    return e;
}


namespace rapl {

enum class msr_nr : unsigned int {
    PKG = 0x611,
    CORE = 0x639,
    DRAM = 0x619,
    GPU = 0x641,

    UNIT = 0x606
};

enum class msr_offset : unsigned int {
    PKG = 0,
    CORE = PKG,
    DRAM = PKG,
    GPU = PKG,

    UNIT = 8
};

enum class msr_mask : unsigned int {
    PKG = 0xffffffff,
    CORE = PKG,
    DRAM = PKG,
    GPU = PKG,

    UNIT = 0x1f00
};

template <typename EnumClass>
auto to_underlying(const EnumClass &e) -> typename std::underlying_type<EnumClass>::type
{
    return static_cast<typename std::underlying_type<EnumClass>::type>(e);
}

static unsigned int read_msr(const msr_nr &nr, const msr_offset &offset, const msr_mask &mask)
{
    int msr = open("/dev/cpu/0/msr", O_RDONLY);

    if (msr > 0) {
        uint64_t val;

        lseek(msr, to_underlying(nr), SEEK_SET);
        read(msr, &val, sizeof(uint64_t));
        close(msr);

        return (val & to_underlying(mask)) >> to_underlying(offset);
    } else {
        throw std::runtime_error{"Failed to open msr file!"};
    }
}

Value Value::read()
{
    Value val;

    val.pkg = read_msr(msr_nr::PKG, msr_offset::PKG, msr_mask::PKG);
    val.core = read_msr(msr_nr::CORE, msr_offset::CORE, msr_mask::CORE);
    val.dram = read_msr(msr_nr::DRAM, msr_offset::DRAM, msr_mask::DRAM);
    val.gpu = read_msr(msr_nr::GPU, msr_offset::GPU, msr_mask::GPU);

    return val;
}


class Unit {
   private:
    static bool _initialized;
    static unsigned int _unit;

   public:
    static unsigned int get();
};

bool Unit::_initialized = false;
unsigned int Unit::_unit = 1;

unsigned int Unit::get()
{
    if (!_initialized) {
        auto val = read_msr(msr_nr::UNIT, msr_offset::UNIT, msr_mask::UNIT);

        _unit = 1000000 / (1 << val);
        _initialized = true;
    }

    return _unit;
}

unsigned long calculate_consumed(const unsigned long start, const unsigned long end)
{
    if (start <= end)
    {
        return end - start;
    } else {
        // rapl overflow
        return (1UL << 32) - start + end;
    }
}

Energy consumed_energy(const Value &start, const Value &end)
{
    Energy e;

    e.package = calculate_consumed(start.pkg, end.pkg) * Unit::get();
    e.core = calculate_consumed(start.core, end.core) * Unit::get();
    e.dram = calculate_consumed(start.dram, end.dram) * Unit::get();
    e.gpu = calculate_consumed(start.gpu, end.gpu) * Unit::get();

    return e;
}

} /* namespace rapl */


const std::string MSRMeasure::name = {"msr"};

MSRMeasure::MSRMeasure(Process *proc) :
    Measure{proc}, _accum_energy{}, _last_rapl{}
{}

bool MSRMeasure::start_()
{
    _last_rapl = rapl::Value::read();

    return true;
}

bool MSRMeasure::stop_()
{
    auto current_rapl = rapl::Value::read();
    _accum_energy += consumed_energy(_last_rapl, current_rapl);

    return true;
}

void MSRMeasure::reset_()
{
    _accum_energy = {};

    if (this->_running)
        _last_rapl = rapl::Value::read();
}

Energy MSRMeasure::energy()
{
    if (this->_running) {
        auto current_rapl = rapl::Value::read();

        _accum_energy += consumed_energy(_last_rapl, current_rapl);
        _last_rapl = current_rapl;
    }

    return _accum_energy;
}

} /* namespace detail */
