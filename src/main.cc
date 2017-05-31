#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/signalfd.h>
#include <sys/types.h>

#include "process.h"


class Config {
   public:
    class DisplayUsage
    {};

    class MissingArgument
    {
       private:
        std::string option_name;

       public:
        MissingArgument(const std::string &name) :
            option_name{name}
        {}

        std::string name() const
        {
            return option_name;
        }
    };

    class InvalidArgument
    {
       private:
        std::string option_name;
        std::string argument_value;

       public:
        InvalidArgument(const std::string &name, const std::string &argument) :
            option_name{name}, argument_value{argument}
        {}

        std::string name() const
        {
            return option_name;
        }

        std::string argument() const
        {
            return argument_value;
        }
    };

    class MissingOption
    {
       private:
        std::string option_name;

       public:
        MissingOption(const std::string &name) :
            option_name{name}
        {}

        std::string name() const
        {
            return option_name;
        }
    };

    class InvalidOption
    {
       private:
        std::string option_name;

       public:
        InvalidOption(const std::string &name) :
            option_name{name}
        {}

        std::string name() const
        {
            return option_name;
        }
    };

   private:
    enum Options {
        OPT_REPEAT,
        OPT_AUTOTERM,
        OPT_SYNCSTART,
        OPT_REDIRECT,
        OPT_SAMPLING,
        OPT_PATTERN,
        OPT_INFO,
    };

    static const char *short_opts;
    static struct option long_opts[];

   public:
    enum Output {
        NONE = 0x0,
        INFO = 0x1,
        STATS = 0x2,
        ENERGY = 0x4,
        FULL = INFO | STATS | ENERGY
    };

    int repeat = 1;
    bool auto_terminate = false;
    bool sync_start = false;
    std::string redirect = {"/dev/null"};
    std::tuple<double,int> sampling = {1.0, 10};
    bool energy_pattern = false;
    Output info = ENERGY;

   public:
    static Config parse(int argc, char *argv[]);

    double sampling_rate() const;
    int sampling_interval() const;
    std::string info_string() const;
};

const char *Config::short_opts = ":h";

struct option Config::long_opts[] = {
    {"help",        no_argument,        nullptr,    'h'},
    {"repeat",      required_argument,  nullptr,    OPT_REPEAT},
    {"term",        no_argument,        nullptr,    OPT_AUTOTERM},
    {"sync",        no_argument,        nullptr,    OPT_SYNCSTART},
    {"redirect",    required_argument,  nullptr,    OPT_REDIRECT},
    {"sampling",    required_argument,  nullptr,    OPT_SAMPLING},
    {"pattern",     no_argument,        nullptr,    OPT_PATTERN},
    {"info",        required_argument,  nullptr,    OPT_INFO},
    {nullptr,       0,                  nullptr,    0}
};

Config Config::parse(int argc, char *argv[])
{
    /* Tell getopt *not* to print any errors by itself! */
    opterr = 0;

    Config c;

    bool done = false;

    while (!done) {
        switch (getopt_long(argc, argv, short_opts, long_opts, nullptr)) {
            case 'h':
                throw DisplayUsage{};
            case OPT_REPEAT:
                try {
                    c.repeat = std::stoi(optarg);
                    break;
                } catch (...) {
                    throw InvalidArgument("--repeat", optarg);
                }
            case OPT_AUTOTERM:
                c.auto_terminate = true;
                break;
            case OPT_SYNCSTART:
                c.sync_start = true;
                break;
            case OPT_REDIRECT:
                c.redirect = std::string{optarg};
                break;
            case OPT_SAMPLING:
                try {
                    std::string val{optarg};

                    std::string rate_val, length_val;
                    double rate;
                    int length;

                    auto pos = val.find(':');
                    if (pos == std::string::npos) {
                        rate_val = val;
                    } else {
                        rate_val = val.substr(0, pos);
                        length_val = val.substr(pos+1);
                    }

                    rate = rate_val.empty() ? 1.0 : std::stod(rate_val);
                    length = length_val.empty() ? 10 : std::stoi(length_val);

                    if (rate < 0.1 || rate > 1.0)
                        throw InvalidArgument{"--sampling (rate)", optarg};
                    if (length < 10)
                        throw InvalidArgument{"--sampling (length)", optarg};

                    c.sampling = std::make_tuple(rate, length);
                    break;
                } catch (...) {
                    throw InvalidArgument{"--sampling", optarg};
                }
            case OPT_PATTERN:
                c.energy_pattern = true;
                break;
            case OPT_INFO: {
                std::string val{optarg};

                if (val == "none")
                    c.info = NONE;
                else if (val == "info")
                    c.info = INFO;
                else if (val == "stats")
                    c.info = STATS;
                else if (val == "energy")
                    c.info = ENERGY;
                else if (val == "full")
                    c.info = FULL;
                else
                    throw InvalidArgument{"--info", optarg};

                break;
            }
            case ':':
                throw MissingArgument(argv[optopt]);
            case '?':
                throw InvalidOption(argv[optind-1]);
            default: /* -1 */
                done = true;
        }
    }

    return c;
}

double Config::sampling_rate() const
{
    return std::get<0>(sampling);
}

int Config::sampling_interval() const
{
    return std::get<1>(sampling);
}

std::string Config::info_string() const
{
    switch (info) {
        case NONE:
            return "none";
        case INFO:
            return "info";
        case STATS:
            return "stats";
        case ENERGY:
            return "energy";
        case FULL:
            return "full";
        default:
            return "unknown";
    }
}


class Sampler
{
   private:
    std::vector<ProcessPtr> _processes;

    int _measure;
    int _not_measure;

    bool _running;
    bool _sampling;
    bool _measuring;

   public:
    Sampler(const std::vector<ProcessPtr> &processes, const Config &conf);

    void start();
    void step();
    void stop();
};

Sampler::Sampler(const std::vector<ProcessPtr> &processes, const Config &conf) :
    _processes{processes}, _measure{0}, _not_measure{0}, _running{false}, _sampling{true},
    _measuring{false}
{
    double rate = conf.sampling_rate();
    int interval = conf.sampling_interval();

    if (rate == 1.0) {
        _sampling = false;
    } else {
        _measure = std::round(interval * rate);
        _not_measure = std::round(interval * (1.0 - rate));
    }
}

void Sampler::start()
{
    if (_running)
        return;

    /* Remember that we are now started running */
    _running = true;

    /* When a program is started, it is always started with measuring enabled */
    _measuring = true;

    /* If necessary set the alarm for the switch to not measuring */
    if (_sampling)
        alarm(_measure);
}

void Sampler::step()
{
    if (!_sampling || !_running)
        return;

    if (_measuring) {
        for (auto &proc : _processes) {
            if (proc->finished())
                continue;

            proc->measure()->stop();
        }

        alarm(_not_measure);
    } else {
        for (auto &proc : _processes) {
            if (proc->finished())
                continue;

            proc->measure()->start();
        }

        alarm(_measure);
    }

    _measuring = !_measuring;
}

void Sampler::stop()
{
    if (!_running)
        return;

    /* Remember that we are not running anymore. */
    _running = false;
}


class ProcessWatcher
{
   private:
    struct ProcessHandle {
        ProcessPtr process;
        int runs;
        std::vector<std::tuple<Energy, Time, double>> stats;

        ProcessHandle(ProcessPtr process) :
            process{process}, runs{0}, stats{}
        {}
    };

    std::vector<ProcessHandle> _processes;
    int _sfd;

    int _runs;
    bool _automatic_terminate;
    bool _synced_start;

    Sampler _sampler;

   private:
    void prepare_signal_fd(const std::vector<unsigned int> &sigs = {SIGCHLD});
    void close_signal_fd();
    int wait_for_signal();

    bool start_processes();
    bool restart_processes();
    void kill_processes();

    void print_process_stats(const ProcessHandle &ph);

   public:
    ProcessWatcher(const std::vector<ProcessPtr> &processes, const Config &conf);
    ProcessWatcher(const ProcessWatcher&) = delete;
    ProcessWatcher(ProcessWatcher &&o);

    ~ProcessWatcher();

    ProcessWatcher& operator=(const ProcessWatcher&) = delete;
    ProcessWatcher& operator=(ProcessWatcher &&o);

    void loop();

    void display_process_stats();
    void display_sampling_stats();
};

void ProcessWatcher::prepare_signal_fd(const std::vector<unsigned int> &sigs)
{
    sigset_t signals;

    sigemptyset(&signals);
    for (auto sig : sigs)
        sigaddset(&signals, sig);

    sigprocmask(SIG_BLOCK, &signals, nullptr);

    _sfd = signalfd(-1, &signals, SFD_CLOEXEC);

    if (_sfd < 0)
        throw std::runtime_error{"Failed to initialize signal FD!"};
}

void ProcessWatcher::close_signal_fd()
{
    if (_sfd > 0)
        close(_sfd);

    _sfd = -1;
}

int ProcessWatcher::wait_for_signal()
{
    if (_sfd < 0)
        throw std::runtime_error{"Signal FD not properly initialized!"};

    signalfd_siginfo si;
    read(_sfd, &si, sizeof(si));

    return si.ssi_signo;
}

bool ProcessWatcher::start_processes()
{
    bool any_started = false;

    for (auto &ph : _processes) {
        if (ph.runs >= _runs)
            continue;

        ph.process->run();
        ph.runs++;

        any_started = true;
    }

    return any_started;
}

bool ProcessWatcher::restart_processes()
{
    /* Automatically kill all other processes if the first one is done. */
    if (_automatic_terminate) {
        for (auto &ph : _processes) {
            if (!ph.process->finished())
                ph.process->kill();
        }
    }

    /* If synced_start is enabled only continue when all processes finished. */
    if (_synced_start) {
        bool all_finished = true;

        for (auto &ph : _processes) {
            all_finished &= ph.process->finished();
        }

        if (!all_finished)
            return true;
    }

    /* Restart all processes that are already finished */
    bool any_started = false;

    for (auto &ph : _processes) {
        if (ph.process->finished()) {
            /* Save latest stats and clean the process. */
            ph.stats.emplace_back(std::make_tuple(ph.process->energy(), ph.process->time(), ph.process->rate()));
            ph.process->wait();

            /* Restart the process if possible */
            if (ph.runs >= _runs)
                continue;

            ph.process->run();
            ph.runs++;

            any_started = true;
        }
    }

    return any_started;
}

void ProcessWatcher::kill_processes()
{
    for (auto &ph : _processes) {
        if (!ph.process->finished()) {
            ph.process->kill();

            /* Get the latest statistics from the process */
            ph.stats.emplace_back(std::make_tuple(ph.process->energy(), ph.process->time(), ph.process->rate()));
            ph.process->wait();
        }
    }
}

void ProcessWatcher::print_process_stats(const ProcessWatcher::ProcessHandle &ph)
{
    std::cout << "pkg,core,dram,gpu,user,system,looped,exec,wall,loops,rate" << std::endl;

    for (auto &stat : ph.stats) {
        Energy e = std::get<0>(stat);
        Time t = std::get<1>(stat);
        double rate = std::get<2>(stat);

        std::cout << e.package << "," << e.core << "," << e.dram << "," << e.gpu << ","
            << t.user << "," << t.system << "," << t.looped << ","
            << t.user + t.system - t.looped << "," << t.wall << ","
            << e.loops << "," << rate*100 << std::endl;
    }
}

ProcessWatcher::ProcessWatcher(const std::vector<ProcessPtr> &processes, const Config &conf) :
    _processes{}, _sfd{-1}, _runs{conf.repeat}, _automatic_terminate{conf.auto_terminate},
    _synced_start{conf.sync_start}, _sampler{processes, conf}
{
    prepare_signal_fd({SIGCHLD, SIGALRM, SIGINT});

    for (auto &proc : processes) {
        _processes.emplace_back(proc);
    }
}

ProcessWatcher::ProcessWatcher(ProcessWatcher &&o) :
    _processes{std::move(o._processes)}, _sfd{o._sfd}, _runs{o._runs}, _automatic_terminate{o._automatic_terminate},
    _synced_start{o._synced_start}, _sampler{std::move(o._sampler)}
{
    o._sfd = -1;
}

ProcessWatcher::~ProcessWatcher()
{
    close_signal_fd();
}

void ProcessWatcher::loop()
{
    bool done = false;

    if (!start_processes()) {
        std::cout << "Failed to start any processes!" << std::endl;
        return;
    }

    _sampler.start();

    while (!done) {
        int sig = wait_for_signal();

        switch (sig) {
            case SIGALRM:
                _sampler.step();
                break;
            case SIGCHLD:
                done = !restart_processes();
                break;
            case SIGINT:
                kill_processes();
                done = true;
                break;
            default:
                std::cout << "Catched unknown signal" << std::endl;
                done = true;
        }
    }

    _sampler.stop();
}

void ProcessWatcher::display_process_stats()
{
    if (_processes.size() == 1) {
        print_process_stats(_processes[0]);
    } else {
        for (auto &ph : _processes) {
            std::cout << "= " << ph.process->name() << " (" << ph.process->type() << ") =" << std::endl;
            print_process_stats(ph);
        }
    }
}


void usage(const std::string &prog, int exit_code=EXIT_FAILURE)
{
    std::cout
        << "Usage: " << prog << " [OPTIONS] -- [?-!] PROG [ARGS...] [-- [?-!] PROG [ARGS...]...]" << std::endl
        << "Execute the given program(s) with enabled energy accounting." << std::endl
        << std::endl
        << "Options:" << std::endl
        << " -h, --help         Print this help message" << std::endl
        << " --repeat=N         Repeat the execution N times (default=1)" << std::endl
        << " --term             Terminate other processes if the first one exits" << std::endl
        << " --sync             Synchronize starts of processes" << std::endl
        << " --redirect=FILE    Redirect output of processes to FILE (default='/dev/null')" << std::endl
        << "                      [use '' for no redirect]" << std::endl
        << " --sampling=R:L     Use *random sampling* with a rate of R (default=1.0) and" << std::endl
        << "                      an interval of L (default=10) seconds" << std::endl
        << " --pattern          Generate a special energy pattern before and after each" << std::endl
        << "                      benchmark run" <<std::endl
        << " --info=TYPE        Define how much information should be displayed (default=energy)" << std::endl
        << "                      [available options are: none, info, stats, energy, full]" << std::endl;

    exit(exit_code);
}

class InvalidProgramDefinition
{
   private:
    std::string _error;

   public:
    InvalidProgramDefinition(const std::string &error) :
        _error{error}
    {}

    std::string error() const
    { 
        return _error;
    }
};

bool parse_measure_type(const std::string &arg, MeasureType &mt)
{
    if (arg == "!") {
        mt = NONE;
        return true;
    } else if (arg == "-") {
        mt = MSR;
        return true;
    } else if (arg == "?") {
        mt = ETEAM;
        return true;
    }

    return false;
}

void parse_program_definition(int argc, char *argv[], int pos, std::vector<ProcessPtr> &procs,
        const Config &conf)
{
    MeasureType mt = ETEAM;

    /* The first argument will define which measurement type should be used. */
    if (parse_measure_type(argv[pos], mt))
        pos++;

    /* Check again before parsing the program that the user not accidentally specified the
     * measurement type twice in the program definition. */
    if (parse_measure_type(argv[pos], mt))
        throw InvalidProgramDefinition{"The measurement type is specified twice. Which one should I use?"};

    try {
        procs.emplace_back(std::make_shared<Process>(argc, argv, pos, mt, conf.redirect));
    } catch(...) {
        throw InvalidProgramDefinition{"Malformed program definition."};
    }
}

int main(int argc, char *argv[])
{
    /* Ok, lets parse our command line arguments */
    Config conf;
    try {
        conf = Config::parse(argc, argv);
    } catch(Config::DisplayUsage) {
        usage(argv[0], EXIT_SUCCESS);
    } catch(Config::MissingOption &e) {
        std::cout << "Failed to specify option '" << e.name() << "'" << std::endl << std::endl;
        usage(argv[0]);
    } catch(Config::InvalidOption &e) {
        std::cout << "Unknown option: " << e.name() << std::endl << std::endl;
        usage(argv[0]);
    } catch(Config::MissingArgument &e) {
        std::cout << "Missing argument for option '" << e.name() << "'" << std::endl << std::endl;
        usage(argv[0]);
    } catch(Config::InvalidArgument &e) {
        std::cout << "Invalid argument for option '" << e.name() << "': " << e.argument() << std::endl << std::endl;
        usage(argv[0]);
    }

    int pos = 1;

    while (pos < argc) {
        std::string val{argv[pos]};

        if (val == "--") {
            /* Ok we found the start of the program definition, let us start */
            break;
        }

        pos++;
    }

    /* Parse all the programs in our internal representation. */
    std::vector<ProcessPtr> procs;

    while (++pos < argc) {
        /* We just jumped over the "--" hence until the next occurrence of a "--" is
         * the program definition. */
        try {
            parse_program_definition(argc, argv, pos, procs, conf);
        } catch (InvalidProgramDefinition &e) {
            std::cout << e.error() << std::endl << std::endl;
            usage(argv[0]);
        }

        /* Forward our current position until the end of the current program definition. */
        while (pos < argc) {
            std::string val{argv[pos]};

            if (val == "--")
                break;

            pos++;
        }
    }

    if (procs.size() == 0) {
        std::cout << "No program was specified." << std::endl << std::endl;
        usage(argv[0]);
    }

    if (conf.info & Config::INFO) {
        std::cout << "Active configuration:" << std::endl
            << " repeat=" << conf.repeat << std::endl
            << " auto_terminate=" << conf.auto_terminate << std::endl
            << " sync_start=" << conf.sync_start << std::endl
            << " redirect=" << (conf.redirect.empty() ? "NONE" : conf.redirect) << std::endl
            << " sampling=" << conf.sampling_rate() << ":" << conf.sampling_interval() << std::endl
            << " energy_pattern=" << conf.energy_pattern << std::endl
            << " info=" << conf.info_string() << std::endl;

        std::cout << "Measured programs:" << std::endl;
        for (auto &proc : procs) {
            std::cout << " " << proc->name() << " (" << proc->type() << ")" << std::endl;
        }
    }

    /* Start the processes and watch them */
    if (conf.info & Config::INFO)
        std::cout << "Start measuring" << std::flush;

    ProcessWatcher pw{procs, conf};
    pw.loop();

    if (conf.info & Config::INFO)
        std::cout << " --> Done" << std::endl;

    /* Display statistics and energy consumption */
    if (conf.info & Config::ENERGY) {
        pw.display_process_stats();
    }

    return 0;
}
