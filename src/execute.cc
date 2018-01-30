#include "execute.h"

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>

#include <unistd.h>


namespace detail {

ExecExecuter::ExecExecuter(int argc, char *argv[], int start_arg)
    : _argv{nullptr}, _argc{0}
{
    int end_arg = start_arg;

    while (end_arg < argc) {
        std::string arg{argv[end_arg]};

        if (arg == "--")
            break;

        end_arg++;
    }

    if (end_arg == start_arg) {
        /* We directly found a "--" at the beginning of the arguments again.
         * That is not OK because this means, that there isn't even a program
         * specified. Hence we will fail hard here! */
        throw std::runtime_error{"Missing definition of a program"};
    }

    _argc = end_arg - start_arg;
    _argv = new char*[_argc + 1]; /* We need one additional space in the array,
                                     because we have to add NULL at the end. */

    for (int i = start_arg, j = 0; i < end_arg; ++i, ++j) {
        int len = std::strlen(argv[i]);

        _argv[j] = new char[len + 1];
        std::strcpy(_argv[j], argv[i]);
    }

    /* Terminate the argument vector */
    _argv[_argc] = nullptr;
}

ExecExecuter::ExecExecuter(const ExecExecuter& other) :
    _argv{nullptr}, _argc{other._argc}
{
    _argv = new char*[_argc + 1];

    for (int i = 0; i < _argc; ++i) {
        int len = std::strlen(other._argv[i]);

        _argv[i] = new char[len + 1];
        std::strcpy(_argv[i], other._argv[i]);
    }

    /* Terminate the argument vector */
    _argv[_argc] = nullptr;
}

ExecExecuter::~ExecExecuter()
{
    if (!_argv)
        return;

    for (char** arg = _argv; *arg != nullptr; ++arg) {
        delete[] *arg;
    }

    delete[] _argv;
    _argv = nullptr;
}

std::string ExecExecuter::repr() const
{
    return std::string{_argv[0]};
}

int ExecExecuter::run()
{
    if (::execvp(_argv[0], _argv) == -1)
        throw std::runtime_error{"Failed to execute 'execvp'"};

    /* If we get to this point, we are screwed anyways */
    return 1;
}

Executer* ExecExecuter::clone() const
{
    return new ExecExecuter(*this);
}


FunctionExecuter::FunctionExecuter(const std::function<int(void)> &func) : 
    _func{func}
{}

std::string FunctionExecuter::repr() const
{
    return std::string{"<func>"};
}

int FunctionExecuter::run()
{
    return _func();
}

Executer* FunctionExecuter::clone() const
{
    return new FunctionExecuter(*this);
}

} /* namespace detail */
