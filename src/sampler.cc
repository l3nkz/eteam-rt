#include "sampler.h"

#include <cmath>

#include <unistd.h>


Sampler::Sampler(ProcessPtr process, double rate, int interval) :
    _process{process}, _measure{0}, _not_measure{0}, _running{false}, _sampling{true},
    _measuring{false}
{
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
        if (!_process->finished())
            _process->measure()->stop();

        alarm(_not_measure);
    } else {
        if (_process->finished())
            _process->measure()->start();

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
