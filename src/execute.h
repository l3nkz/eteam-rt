#ifndef __EXECUTE_H__
#define __EXECUTE_H__

#include <functional>
#include <string>

class Executer
{
   public:
    virtual ~Executer() = default;

    virtual std::string repr() const = 0;
    virtual int run() = 0;
};

namespace detail {

class ExecExecuter : public Executer
{
   private:
    char** _argv;

   public:
    ExecExecuter(int argc, char *argv[], int start_arg=1);
    ~ExecExecuter();

    std::string repr() const;
    int run();
};

class FunctionExecuter : public Executer
{
   private:
    std::function<int(void)> _func;

   public:
    FunctionExecuter(const std::function<int(void)> &func);

    std::string repr() const;
    int run();
};

} /* namespace detail */

#endif /* __EXECUTE_H__ */



