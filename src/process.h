#ifndef __PROCESS_H__
#define  __PROCESS_H__

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <unistd.h>

#include "execute.h"
#include "measure.h"
#include "energy.h"
#include "time.h"


using Clock = std::chrono::high_resolution_clock;

class Process
{
   public:
    enum State {
        RUNNING = 'R',
        SLEEPING = 'S',
        UNINTERRUPTIBLE = 'D',
        ZOMBIE = 'Z',
        STOPPED = 'T',
        PAGING = 'W',
        DEAD = 'X',
        UNKNOWN = 'U',
        INVALID
    };

   private:
    pid_t _pid;

    Measure *_measure;
    Executer *_exec;

    typename Clock::time_point _start;

    std::string _out_redir;
    bool _owned;

   private:
    Process(Executer *exec, MeasureType mt, const std::string &redirect);

   public:
    Process(int argc, char *argv[], int start_arg, MeasureType mt=ETEAM, const std::string &redirect="");
    Process(const std::function<int(void)> &func, MeasureType mt=ETEAM, const std::string &redirect="");

    ~Process();

    bool run();

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

using ProcessPtr = std::shared_ptr<Process>;

#endif /* __PROCESS_H__ */
