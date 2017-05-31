#ifndef __MEASURE_H__
#define __MEASURE_H__

#include <string>

#include "energy.h"
#include "time.h"


class Process;

enum MeasureType {
    NONE,
    ETEAM,
    MSR
};

class Measure
{
   public:
    static Measure* measure_with(MeasureType type, Process *proc);

   protected:
    bool _running;
    Process *_proc;

   private:
    Time _last_proc_time;

    double _measured;
    double _not_measured;

    void update_times();

   private:
    virtual bool start_() = 0;
    virtual bool stop_() = 0;
    virtual void reset_() = 0;

   public:
    Measure(Process *proc);
    virtual ~Measure() = default;

    virtual std::string repr() const = 0;

    bool start();
    bool stop();

    void reset();

    virtual Energy energy() = 0;
    double rate();
};

namespace detail {

class NoMeasure : public Measure
{
   public:
    NoMeasure(Process *proc) : Measure{proc}
    {}

    std::string repr() const { return "none"; }

    bool start_() { return true; }
    bool stop_() { return true; }

    void reset_() {}

    Energy energy() { return Energy{}; }
};

class ETeamMeasure : public Measure
{
   public:
    ETeamMeasure(Process *proc);

    std::string repr() const { return "eteam"; }

    bool start_();
    bool stop_();

    void reset_() {};

    Energy energy();
};

namespace rapl {

struct Value {
   public:
    unsigned long pkg;
    unsigned long core;
    unsigned long dram;
    unsigned long gpu;

    static Value read();
};

Energy consumed_energy(const Value &start, const Value &end);

} /* namespace rapl */

class MSRMeasure : public Measure
{
   private:
    Energy _accum_energy;
    rapl::Value _last_rapl;

   public:
    MSRMeasure(Process *proc);

    std::string repr() const { return "msr"; }

    bool start_();
    bool stop_();

    void reset_();

    Energy energy();
};

} /* namespace detail */

#endif /* __MEASURE_H__ */
