#include "energy.h"

Energy Energy::operator+(const Energy &o) const
{
    Energy e{*this};
    e += o;

    return e;
}

Energy &Energy::operator+=(const Energy &o)
{
    package += o.package;
    core += o.core;
    dram += o.dram;
    gpu += o.gpu;

    return *this;
}

Energy Energy::operator-(const Energy &o) const
{
    Energy e{*this};
    e -= o;

    return e;
}

Energy &Energy::operator-=(const Energy &o)
{
    package -= o.package;
    core -= o.core;
    dram -= o.dram;
    gpu -= o.gpu;

    return *this;
}
