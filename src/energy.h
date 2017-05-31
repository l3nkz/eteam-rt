#ifndef __ENERGY_H__
#define __ENERGY_H__

struct Energy
{
    unsigned long long package;
    unsigned long long core;
    unsigned long long dram;
    unsigned long long gpu;

    unsigned long loops;

    Energy operator+(const Energy &o) const;
    Energy &operator+=(const Energy &o);
    Energy operator-(const Energy &o) const;
    Energy &operator-=(const Energy &o);
};

#endif /* __ENERGY_H__ */
