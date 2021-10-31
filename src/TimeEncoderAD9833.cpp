#include "TimeEncoderAD9833.h"

void TimeEncoderAD9833::Init(void *p)
{
    Serial.println("AD Init");
    TimeEncoderAD9833Parms *parms = (TimeEncoderAD9833Parms *)p;
    String out;
    // out = Serial.println(std::format()("pin {} ignore {}freq {}", parms->_pin, parms->_ignore_modulation, parms->_broadcast_frequency))
    _mcp = new MD_AD9833(parms->_pin);
    _ignore_modulation = parms->_ignore_modulation;

    _mcp->begin();
    _mcp->setMode(MD_AD9833::MODE_SINE);
    _mcp->setFrequency(MD_AD9833::CHAN_0, parms->_broadcast_frequency);
}

void TimeEncoderAD9833::Modulate(int isHigh)
{
    if (_ignore_modulation)
        return;
    if (isHigh)
        _mcp->setMode(MD_AD9833::MODE_SINE);
    else
        _mcp->setMode(MD_AD9833::MODE_OFF);
}
