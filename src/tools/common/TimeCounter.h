#ifndef TIMECOUNTER_H__
#define TIMECOUNTER_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"

class _BSPEXPORT TimeCounter
{
public:
    void start()
    {
        _start = I_FloatTime();
    }

    void stop()
    {
        double stop = I_FloatTime();
        _accum += stop - _start;
    }

    double getTotal() const
    {
        return _accum;
    }

    void reset()
    {
        memset(this, 0, sizeof(*this));
    }

// Construction
public:
    TimeCounter()
    {
        reset();
    }
    // Default Destructor ok
    // Default Copy Constructor ok
    // Default Copy Operator ok

protected:
    double _start;
    double _accum;
};

#endif//TIMECOUNTER_H__