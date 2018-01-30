#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <string>
#include <functional>

#include "execute.h"
#include "measure.h"
#include "process.h"


class Program
{
   private:
    Executer* _exec;
    MeasureType _mt;
    std::string _redirect;

   private:
    Program(Executer *exec, MeasureType mt, const std::string &redirect="");

   public:
    Program(int argc, char *argv[], int start_arg, MeasureType mt, const std::string &redirect="");
    Program(const std::function<int(void)> &func, MeasureType mt, const std::string &redirect="");

    Program(const Program& other);
    Program(Program&& other);

    ~Program();

    Program& operator=(const Program& other);
    Program& operator=(Program&& other);

    ProcessPtr run();

    std::string name() const;
    std::string type() const;
};

#endif /* __PROGRAM_H__ */
