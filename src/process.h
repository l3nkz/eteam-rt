#ifndef __PROCESS_H__
#define  __PROCESS_H__

#include <memory>
#include <string>

#include <unistd.h>

#include "measure.h"
#include "energy.h"
#include "time.h"


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

    virtual ~Process() {}

    virtual void disown() = 0;
    virtual int wait() = 0;

    virtual State state() const = 0;
    virtual bool valid() const = 0;
    virtual bool running() const = 0;
    virtual bool finished() const = 0;
    virtual bool stopped() const = 0;
    virtual bool blocked() const = 0;

    virtual std::string name() const = 0;
    virtual std::string type() const = 0;
    virtual pid_t pid() const = 0;

    virtual Measure *measure() = 0;

    virtual Energy energy() = 0;
    virtual Time time() const = 0;
    virtual double rate() = 0;

    virtual void signal(int signum) const = 0;
    virtual void cont() const = 0;
    virtual void stop() const = 0;
    virtual void kill() const = 0;
    virtual void term() const = 0;
};


using ProcessPtr = std::shared_ptr<Process>;

#endif /* __PROCESS_H__ */
