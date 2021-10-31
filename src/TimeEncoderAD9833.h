#ifndef __TIME_ENCODER_AD9833_H
#define __TIME_ENCODER_AD9833_H
#include "TimeEncoder.h"
#include <MD_AD9833.h>
#include <Arduino.h>

class TimeEncoderAD9833 : public TimeEncoder
{
    public:
    void Init(void *Parms);
    void Modulate(int isHigh);

    private:
    MD_AD9833 *_mcp;
    int _ignore_modulation;
};

class TimeEncoderAD9833Parms
{
    public:
    int _pin;
    int _ignore_modulation;
    int _broadcast_frequency;
};

#endif