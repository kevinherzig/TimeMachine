#include "TimeEncoderSerial.h"
#include <Arduino.h>


    void TimeEncoderSerial::Init(void *p)
    {
        //Serial.begin(((TimeEncoderSerialParms *) p)->BaudRate);
    }

     void TimeEncoderSerial::Modulate(int isHigh)
     {
         if(isHigh)
            Serial.println("H");
         else
            Serial.println("L");
     }
