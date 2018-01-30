#include "program.h"

#include "normal_process.h"


Program::Program(Executer *exec, MeasureType mt, const std::string &redirect) :
    _exec(exec), _mt(mt), _redirect(redirect)
{}

Program::Program(int argc, char *argv[], int start_arg, MeasureType mt, const std::string &redirect) :
    Program(new detail::ExecExecuter(argc, argv, start_arg), mt, redirect)
{}

Program::Program(const std::function<int(void)> &func, MeasureType mt, const std::string &redirect) :
    Program(new detail::FunctionExecuter(func), mt, redirect)
{}

Program::Program(const Program& other) :
    _exec{other._exec->clone()}, _mt{other._mt}, _redirect{other._redirect}
{}

Program::Program(Program&& other) :
    _exec{other._exec}, _mt{other._mt}, _redirect{std::move(other._redirect)}
{
    other._exec = nullptr;
}

Program::~Program()
{
    if (_exec)
        delete _exec;
}

Program& Program::operator=(const Program& other)
{
    if (_exec)
        delete _exec;

    _exec = other._exec->clone();
    _mt = other._mt;
    _redirect = other._redirect;

    return *this;
}

Program& Program::operator=(Program&& other)
{
    if (_exec)
        delete _exec;

    _exec = other._exec;
    _mt = other._mt;
    _redirect = std::move(other._redirect);

    other._exec = nullptr;

    return *this;
}

ProcessPtr Program::run()
{
    return ProcessPtr{new detail::NormalProcess(_exec->clone(), _mt, _redirect)};
}

std::string Program::name() const
{
    return _exec->repr();
}

std::string Program::type() const
{
    return Measure::measure_name(_mt);
}
