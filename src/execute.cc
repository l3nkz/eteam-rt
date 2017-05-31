#include "execute.h"

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>

#include <unistd.h>


namespace detail {

ExecExecuter::ExecExecuter(int argc, char *argv[], int start_arg)
    : _argv{nullptr}
{
    int end_arg = start_arg, num_args;

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

    num_args = end_arg - start_arg - 1; /* The first argument is the program itself */
    _argv = new char*[num_args + 2]; /* We need one additional space in the array,
                                        because we have to add NULL at the end and another
                                        additional space at the beginning for the program
                                        name itself. */

    for (int i = start_arg, j = 0; i < end_arg; ++i, ++j) {
        int len = std::strlen(argv[i]);

        _argv[j] = new char[len + 1];
        std::strcpy(_argv[j], argv[i]);
    }

    /* Terminate the argument vector */
    _argv[num_args + 1] = nullptr;
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

} /* namespace detail */


