#ifndef CS_ANTICHEAT_RULES_H
#define CS_ANTICHEAT_RULES_H

#include <math.h>
#include <cmath>

inline bool CSCommandScalarValid(float value,float maxAbs) {return std::isfinite(value) && fabsf(value)<=maxAbs;}

struct CSCommandRateBudget
{
    double credit,lastTime;bool initialized;
    CSCommandRateBudget():credit(0),lastTime(0),initialized(false) {}
    bool Consume(double now,double interval)
    {
        if(!std::isfinite(now) || !std::isfinite(interval) || interval<=0) return true;
        const double capacity=fmax(32.0,0.5/interval);
        if(!initialized || now<lastTime) {credit=capacity;lastTime=now;initialized=true;}
        credit=fmin(capacity,credit+fmax(0.0,now-lastTime)/interval);lastTime=now;
        credit-=1;
        // Keep debt bounded so a connection can recover after sustained loss.
        credit=fmax(-capacity,credit);
        return credit>=-2.0;
    }
};
#endif
