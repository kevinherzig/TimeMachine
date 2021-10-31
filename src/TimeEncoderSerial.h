#include "TimeEncoder.h"

class TimeEncoderSerial : public TimeEncoder
{
    public:
         void Init(void *Parms);
     void Modulate(int isHigh);
};

class TimeEncoderSerialParms
{
    public:
    int BaudRate;
};