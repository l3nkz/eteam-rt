#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <string>

#include "execute.h"
#include "measure.h"
#include "process.h"


struct SamplerConf
{
    double rate;
    int interval;
};

class Sampler
{
   private:
    ProcessPtr _process;

    int _measure;
    int _not_measure;

    bool _running;
    bool _sampling;
    bool _measuring;

   public:
    Sampler(ProcessPtr process, double rate, int interval);

    void start();
    void step();
    void stop();
};

#endif /* __SAMPLER_H__  */
