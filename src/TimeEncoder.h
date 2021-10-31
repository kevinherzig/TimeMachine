#ifndef __TIME_ENCODER_H
#define __TIME_ENCODER_H
class TimeEncoder
{
    public:
    virtual void Init(void *Parms);
    virtual void Modulate(int isHigh);
};
#endif

