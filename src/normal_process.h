#ifndef __NORMAL_PROCESS_H__
#define __NORMAL_PROCESS_H__

#include <chrono>
#include <memory>
#include <string>

#include <unistd.h>

#include "energy.h"
#include "execute.h"
#include "measure.h"
#include "process.h"
#include "time.h"


class Program;


namespace detail {

using Clock = std::chrono::high_resolution_clock;


class NormalProcess : public Process
{
   public:
   private:
    pid_t _pid;

    Measure *_measure;
    Executer *_exec;

    typename Clock::time_point _start;

    std::string _out_redir;
    bool _owned;

   private:
    NormalProcess(Executer *exec, MeasureType mt, const std::string &redirect);

    void start();

    friend class ::Program;

   public:
    ~NormalProcess();

    void disown();
    int wait();

    State state() const;
    bool valid() const;
    bool running() const;
    bool finished() const;
    bool stopped() const;
    bool blocked() const;

    std::string name() const;
    std::string type() const;
    pid_t pid() const;

    Measure *measure()
    {
        return _measure;
    }

    Energy energy();
    Time time() const;
    double rate();

    void signal(int signum) const;
    void cont() const;
    void stop() const;
    void kill() const;
    void term() const;
};


using NormalProcessPtr = std::shared_ptr<NormalProcess>;

} /* namespace detail */

#endif /* __NORMAL_PROCESS_H__ */
