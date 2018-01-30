#include "normal_process.h"

#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "execute.h"
#include "measure.h"
#include "time.h"
#include "energy.h"


namespace detail {

NormalProcess::NormalProcess(Executer *exec, MeasureType mt, const std::string &redirect) :
    _pid{-1}, _measure{Measure::measure_with(mt, this)}, _exec{exec},
    _out_redir{redirect}, _owned{true}
{
    start();
}

NormalProcess::~NormalProcess()
{
    if (_pid != -1 && _owned) {
        if (!finished())
            kill();

        wait();
    }

    delete _measure;
    delete _exec;
}

void NormalProcess::start()
{
    _start = Clock::now();
    _pid = ::fork();

    if (_pid == 0) {
        /* Child */

        if (!_out_redir.empty()) {
            auto redir = ::open(_out_redir.c_str(), O_WRONLY | O_APPEND | O_CREAT);
            ::dup2(redir, 1);
            ::dup2(redir, 2);
            ::close(redir);
        }

        ::exit(_exec->run());
    } else if (_pid > 0){
        _measure->start();
    } else {
        throw std::runtime_error{"Failed to fork"};
    }
}

void NormalProcess::disown()
{
    _owned = false;
}

int NormalProcess::wait()
{
    int status;

    ::waitpid(_pid, &status, 0);
    _pid = -1;

    return WEXITSTATUS(status);
}

Process::State NormalProcess::state() const
{
    if (_pid == -1)
        return INVALID;

    std::stringstream path;
    path << "/proc/" << _pid << "/stat";
    std::ifstream stat{path.str(), std::ios::in};

    if (stat.is_open()) {
        /* Ignore the first two items in the 'stat' file. */
        stat.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        stat.ignore(std::numeric_limits<std::streamsize>::max(), ' ');

        char st;
        stat >> st;

        stat.close();

        switch(st) {
            case RUNNING:
                return RUNNING;
            case SLEEPING:
                return SLEEPING;
            case UNINTERRUPTIBLE:
                return UNINTERRUPTIBLE;
            case ZOMBIE:
                return ZOMBIE;
            case STOPPED:
                return STOPPED;
            case PAGING:
                return PAGING;
            case DEAD:
                return DEAD;
            default:
                return UNKNOWN;
        }
    } else {
        return INVALID;
    }
}

bool NormalProcess::valid() const
{
    return state() != INVALID;
}

bool NormalProcess::running() const
{
    return state() == RUNNING;
}

bool NormalProcess::finished() const
{
    auto st = state();

    return (st == ZOMBIE) || (st == DEAD);
}

bool NormalProcess::stopped() const
{
    return state() == STOPPED;
}

bool NormalProcess::blocked() const
{
    auto st = state();

    return (st == UNINTERRUPTIBLE) || (st == SLEEPING) || (st == PAGING);
}

std::string NormalProcess::name() const
{
    return _exec->repr();
}

std::string NormalProcess::type() const
{
    return _measure->repr();
}

pid_t NormalProcess::pid() const
{
    return _pid;
}

Energy NormalProcess::energy()
{
    return _measure->energy();
}

Time NormalProcess::time() const
{
    if (_pid == -1)
        return Time{};

    Time t{};

    std::stringstream path;
    path << "/proc/" << _pid << "/stat";
    std::ifstream stat{path.str(), std::ios::in};

    if (stat.is_open()) {
        /* The values that we are interesting in are at position 14 and 15 */
        for (int i = 1; i < 14; ++i)
            stat.ignore(std::numeric_limits<std::streamsize>::max(), ' ');

        stat >> t.user >> t.system;

        stat.close();

        t.user /= sysconf(_SC_CLK_TCK);
        t.system /= sysconf(_SC_CLK_TCK);
    }

    path.str("");
    path << "/proc/" << _pid << "/energystat";
    std::ifstream estat{path.str(), std::ios::in};

    if (estat.is_open()) {
        /* The value that we are interested in is at position 7 */
        for (int i = 1; i < 7; ++i)
            estat.ignore(std::numeric_limits<std::streamsize>::max(), ' ');

        estat >> t.looped;

        estat.close();

        t.looped  /= 1000000.0; /* Convert micro seconds in seconds */
    }

    auto now = Clock::now();

    t.wall = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1,1>>>(now - _start).count();

    return t;
}

double NormalProcess::rate()
{
    return _measure->rate();
}

void NormalProcess::signal(int signum) const
{
    if (_pid == -1)
        return;

    ::kill(_pid, signum);
}

void NormalProcess::cont() const
{
    signal(SIGCONT);
}

void NormalProcess::stop() const
{
    signal(SIGSTOP);
}

void NormalProcess::kill() const
{
    signal(SIGKILL);
}

void NormalProcess::term() const
{
    signal(SIGTERM);
}

} /* namespace detail */
