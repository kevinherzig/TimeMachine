#ifndef __TIME_ENCODER_MCP41_H
#define __TIME_ENCODER_MCP41_H
#include "TimeEncoder.h"
#include <MCP41xxx.h>

class TimeEncoderMCP41 : public TimeEncoder
{
    public:
    void Init(void *Parms);
    void Modulate(int isHigh);

    private:
    MCP41xxx *_mcp;
    int _level_low;
    int _level_high;
    int _level_amp;
    int _ignore_modulation;
};

class TimeEncoderMCP41Parms
{
    public:
    int pin;
    int level_init;
    int level_low;
    int level_high;
    int level_amp;
    int _ignore_modulation;
};

#endif