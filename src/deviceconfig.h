#include "TimeEncoderSerial.h"

#define _USE_AD9833_CONST_CARRIER

#ifdef _USE_AD9833_CONST_CARRIER
#include "TimeEncoderAD9833.h"

int useStaticOutput = true;
int pin_AD9833 = 26;
int modulation_frequency = 60000;

TimeEncoderAD9833Parms timeEncoderAD9833Parms =
    {
        ._pin = pin_AD9833,
        ._ignore_modulation = useStaticOutput,
        ._broadcast_frequency = modulation_frequency};

TimeEncoderAD9833 encoder_AD9833;
#endif

// ---------- Serial Port Monitor Ouptut
bool USE_DECODER_SERIAL = false;
// Change your serial baud rate here
TimeEncoderSerialParms timeEncoderParms = {115200};

// #define _USE_POT_GAIN

#ifdef _USE_POT_GAIN
#include "TimeEncoderMCP41.h"
int pin_Gain = 14;
const int ampGain = 210;

TimeEncoderMCP41Parms timeEncoderAD9833Parms_GAIN =
    {
        .pin = pin_Gain,
        .level_init = ampGain,
        .level_low = 0,
        .level_high = 0,
        ._ignore_modulation = true};

TimeEncoderMCP41 decoder_MCP41_Gain;

#endif

#define _USE_POT_ATTENUATE

#ifdef _USE_POT_ATTENUATE

#include "TimeEncoderMCP41.h"
int pin_Attenuate = 27;
const int outputAttenuate17 = 220; 
const int outputAttenuate0 = 0;
const int ampGain = 180;

TimeEncoderMCP41Parms timeEncoderMCP41Parms_Amp = {
    .pin = pin_Attenuate,
    .level_init = 255,
    .level_low = outputAttenuate17,
    .level_high = outputAttenuate0,
    .level_amp = ampGain,
    ._ignore_modulation = false};

TimeEncoderMCP41 decoder_MCP41_Attenuate;

#endif
