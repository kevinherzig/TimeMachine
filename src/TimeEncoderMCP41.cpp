#include "TimeEncoderMCP41.h"


    void TimeEncoderMCP41::Init(void *p)
    {
        TimeEncoderMCP41Parms *parms = (TimeEncoderMCP41Parms *) p;
        _mcp = new MCP41xxx(parms->pin);

        _level_low = parms->level_low;
        _level_high = parms->level_high;
        _level_amp = parms->level_amp;
        _ignore_modulation = parms->_ignore_modulation;
        _mcp->begin();
        _mcp->analogWrite(0, parms->level_amp);
    }

     void TimeEncoderMCP41::Modulate(int isHigh)
     {
         //Serial.print("modulating "); Serial.println(isHigh);
         //if(_ignore_modulation) return;
        // Serial.print("setting "); Serial.println(isHigh); Serial.print("high "); Serial.println(_level_high); Serial.print("low "); Serial.println(_level_low);
         // val1 + (val2 << 8)

         if(isHigh)
            _mcp->analogWrite(1,_level_high);
         else
            _mcp->analogWrite(1, _level_low);
     }
