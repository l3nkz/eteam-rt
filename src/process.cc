#include "process.h"

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


Process::Process(Executer *exec, MeasureType mt, const std::string &redirect) :
    _pid{-1}, _measure{Measure::measure_with(mt, this)}, _exec{exec},
    _out_redir{redirect}, _owned{true}
{}

Process::Process(int argc, char *argv[], int start_arg,
        MeasureType mt, const std::string &redirect) :
    Process(new detail::ExecExecuter(argc, argv, start_arg), mt, redirect)
{}

Process::Process(const std::function<int(void)> &func,
        MeasureType mt, const std::string &redirect) :
    Process(new detail::FunctionExecuter(func), mt, redirect)
{}

Process::~Process()
{
    if (_owned) {
        if (!finished())
            kill();

        wait();
    }

    delete _measure;
    delete _exec;
}

bool Process::run()
{
    if (running())
        return true;

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
        _measure->reset();
        _measure->start();
    } else {
        return false;
    }

    return true;
}

void Process::disown()
{
    _owned = false;
}

int Process::wait()
{
    int status;

    ::waitpid(_pid, &status, 0);
    _pid = -1;

    return WEXITSTATUS(status);
}

Process::State Process::state() const
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

bool Process::valid() const
{
    return state() != INVALID;
}

bool Process::running() const
{
    return state() == RUNNING;
}

bool Process::finished() const
{
    auto st = state();

    return (st == ZOMBIE) || (st == DEAD);
}

bool Process::stopped() const
{
    return state() == STOPPED;
}

bool Process::blocked() const
{
    auto st = state();

    return (st == UNINTERRUPTIBLE) || (st == SLEEPING) || (st == PAGING);
}

std::string Process::name() const
{
    return _exec->repr();
}

std::string Process::type() const
{
    return _measure->repr();
}

pid_t Process::pid() const
{
    return _pid;
}

Energy Process::energy()
{
    return _measure->energy();
}

Time Process::time() const
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

double Process::rate()
{
    return _measure->rate();
}

void Process::signal(int signum) const
{
    if (_pid == -1)
        return;

    ::kill(_pid, signum);
}

void Process::cont() const
{
    signal(SIGCONT);
}

void Process::stop() const
{
    signal(SIGSTOP);
}

void Process::kill() const
{
    signal(SIGKILL);
}

void Process::term() const
{
    signal(SIGTERM);
}
