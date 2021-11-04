
// TimeMachine, a time transmission system
// Copyright (C) 2018 Kevin Herzig <kevin@herzig.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "NMEAGPS.h"
#include "SoftwareSerial.h"
#include "esp_system.h"
#include "time-signal-source.h"
#include <WiFiManager.h>
#include <SPI.h>
#include "deviceconfig.h"
#include "freertos/semphr.h"
#include <time.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoLog.h>
#include <heltec.h>
#include "WiFi.h"

// WARNING: INTENTIONAL INACCURACY IF SET!
// Defining RANDOM_TIME will apply a random offset to the time.  This allows
// you to see if your clock is really picking up the signal from TimeMachine
// instead of WWVB.  Obviously use this with care especially if you have
// an antenna that gives you some range.
#if _RANDOM_TIME_OFFSET
int artificialOffsetSeconds = random(100, 1000);
#else
int artificialOffsetSeconds = 0;
#endif

// Set this to your time zone offset w/o consideration of DST.
// Eventually add a setting in the web portal.
int timeZoneOffsetHours = -5;

// GPS config.  If you don't use a GPS then
// the system will never get a pulse AND the GPS
// serial monitor code will never get invoked.
// Note that we don't use TX Pin, we have nothing to
// say to the GPS.
static const int RXPin = 5, TXPin = 19, PPSPin = 17;
static const uint32_t GPSBaud = 9600;
SoftwareSerial gpsSerialPort(RXPin, TXPin);
NMEAGPS gpsInterpreter;

WiFiManager wifiManager;
AsyncWebServer server(8080);

enum runMode
{
  NTP,
  GPS,
};

// Other global variables
volatile int currentRunMode = NTP;
volatile unsigned long last_gps_ping = 0;
time_t timeWeAreEncoding = 0;
xQueueHandle semaphore_Encoder = NULL;
xQueueHandle semaphore_GPSSerialReader = NULL;
xQueueHandle semaphore_Display = NULL;
String stringMarquee;

std::vector<TimeEncoder *> encoders;

String displayCurrentLine1;
String displayCurrentLine2;
String displayCurrentLine3;
String displayCurrentLine4;

/////////////////////////////////////////////////////////////////////
void ConfigureModulationDevices()
{
#ifdef _USE_POT_GAIN
  decoder_MCP41_Gain.Init((void *)&timeEncoderAD9833Parms_GAIN);
  encoders.push_back(&decoder_MCP41_Gain);
#endif

#ifdef _USE_POT_ATTENUATE
  decoder_MCP41_Attenuate.Init((void *)&timeEncoderMCP41Parms_Amp);
  encoders.push_back(&decoder_MCP41_Attenuate);
#endif

#ifdef _USE_AD9833_CONST_CARRIER
  encoder_AD9833.Init((void *)&timeEncoderAD9833Parms);
  encoders.push_back(&encoder_AD9833);
#endif
}

/////////////////////////////////////////////////////////////////////
void IRAM_ATTR GPSReadLoop(void *p)
{
  bool isAFix = false;
  gps_fix fix;
  uint8_t serialBuffer[200];
  // The GPS will tell us when it's ready via the PPS signal.
  // so don't bother even turning on the software serial port
  // unless we know the fix is good already.
  xSemaphoreTake(semaphore_GPSSerialReader, portMAX_DELAY);
  Log.info("GPS serial monitor thread woke up");

  gpsSerialPort.begin(9600);

  while (1)
  {
    isAFix = false;
    Log.traceln("Entering GPS Read");
    while (gpsSerialPort.available() && !isAFix)
    {
      Log.traceln("Serial port data available");

      int bytesread = gpsSerialPort.readBytes(serialBuffer, 199);

      // Feed the serial output into the GPS interpreter
      // This isn't the way the example works, but the example
      // code didn't work for me ¯\_(ツ)_/¯

      for (int i = 0; i < bytesread; i++)
      {
        Log.trace("%c", serialBuffer[i]);
        gpsInterpreter.handle(serialBuffer[i]);
      }
      Log.traceln("");

      fix = gpsInterpreter.fix();

      if (fix.valid.time && fix.valid.date)
      {
        isAFix = true;
        Log.traceln("Found GPS fix");
      }
    }

    if (currentRunMode == NTP && isAFix)
    {
      Log.infoln("Switching to GPS run mode");

      // Change from Y2K epoch returned by GPS to UNIX epoch used by system
      timeWeAreEncoding = fix.dateTime + 946684800;
      currentRunMode = GPS;

      // Pause the GPS when we have a fix since we don't need it anymore
      Log.infoln("Shutting down GPS");
      gpsSerialPort.stopListening();
      Log.infoln("GPS Thread going to sleep");
      
      // Since the GPS interrupt handler has been giving the semaphore
      // it's likely set, so clear it before trying to take it
      xSemaphoreTake(semaphore_GPSSerialReader, 0);
      xSemaphoreTake(semaphore_GPSSerialReader, portMAX_DELAY);
      Log.infoln("GPS Thread awoken");
      isAFix = false;
      gpsSerialPort.listen();
    }
  }
}

/////////////////////////////////////////////////////////////////////
time_t IRAM_ATTR HoldOnRTCClock()
{
  int second;
  struct tm *tv_now;

  const TickType_t delay_rtc = 1 / portTICK_PERIOD_MS;
  time_t rtc_time;
  time(&rtc_time);

  tv_now = gmtime(&timeWeAreEncoding);
  second = tv_now->tm_sec;

  while (second == tv_now->tm_sec)
  {
    vTaskDelay(delay_rtc);
    time(&rtc_time);
    tv_now = gmtime(&rtc_time);
  }
  second = tv_now->tm_sec;
  return rtc_time;
}

/////////////////////////////////////////////////////////////////////
void IRAM_ATTR setModulation(bool isHigh)
{
  // Iterate through the active encoders and let them do their thing.
  // Preferably we'll have the radio transmitter first in this stack
  // so that it gets time priority
  std::for_each(encoders.begin(), encoders.end(), [&](TimeEncoder *encoder)
                { encoder->Modulate(isHigh); });
}

/////////////////////////////////////////////////////////////////////
void IRAM_ATTR EncoderLoop(void *)
{

  Log.info("Starting encoder loop");

  struct tm *tv_now;

  WWVBTimeSignalSource ss;
  time_t lastTime = 0;
  TickType_t tickFinish = 0;
  TimeSignalSource::SecondModulation encodedTimeSignal;
  time_t encodingTime;
  encodingTime = timeWeAreEncoding + 1 + artificialOffsetSeconds;
  char timeStringBuffer[100];

  tv_now = gmtime(&encodingTime);
  ss.PrepareMinute(encodingTime + 1);
  tv_now = gmtime_r(&encodingTime + 1, tv_now);
  encodedTimeSignal = ss.GetModulationForSecond(tv_now->tm_sec);

  while (1)
  {
    Log.traceln("Taking encoder semaphore");
    xSemaphoreTake(semaphore_Encoder, portMAX_DELAY);
    Log.traceln("Releasing encoder semaphore");

    tickFinish = xTaskGetTickCount();

    if (encodingTime > 100000)
    {
      for (int i = 0; i < encodedTimeSignal.size(); i++)
      {

        ModulationDuration coding_unit = encodedTimeSignal[i];

        // Apply the modulation change ASAP then we can do other stuff
        setModulation(coding_unit.power == MOD_HIGH);
        Log.traceln("Modulating segment %d - %s for %d ms", i, (coding_unit.power == MOD_HIGH ? "HIGH" : "LOW"), coding_unit.duration_ms);

        // Once we've set the modulation we now have time to do some other stuff
        // like updating the display and doing the math for the next tick.

        strftime(timeStringBuffer, 20, "%H:%M:%S", localtime(&timeWeAreEncoding));
        Log.infoln("Encoding time %s", timeStringBuffer);

        // If this is our first pass through this tick segment then update the marquee
        if (i == 0)
        {
          // Sanity check, did we see a tick?
          if (++lastTime != encodingTime)
          {
            lastTime = encodingTime;
            if (stringMarquee.length() > 9)
              stringMarquee = stringMarquee.substring(1);
            stringMarquee += 'E';
          }

          if (stringMarquee.length() > 9)
            stringMarquee = stringMarquee.substring(1);

          stringMarquee += encodedTimeSignal[0].marker;
          // Serial.print(encodedTimeSignal[0].marker);
        }

        if (coding_unit.duration_ms > 0)
          vTaskDelayUntil(&tickFinish, coding_unit.duration_ms / portTICK_PERIOD_MS);
      }
    }
    else
    {
      Log.infoln("Invalid time, not broadcasting");
    }

    encodingTime = timeWeAreEncoding + artificialOffsetSeconds + 1;
    ss.PrepareMinute(encodingTime);
    tv_now = gmtime_r(&encodingTime, tv_now);
    encodedTimeSignal = ss.GetModulationForSecond(tv_now->tm_sec);
  }
}

/////////////////////////////////////////////////////////////////////
void IRAM_ATTR GPSTick()
{
  Log.traceln("Received GPS tick on interrupt");
  static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // detect skipped pulse per second
  last_gps_ping = millis();

  if (currentRunMode == NTP)
  {
    // If we're getting pinged from the GPS then we know it has a fix.
    // This wakes up the thread reading from serial to reopen the serial
    // port and start listening to the GPS.  As soon as it reads a time it will shut
    // off the port and go to sleep.
    xSemaphoreGiveFromISR(semaphore_GPSSerialReader, &xHigherPriorityTaskWoken);
  }

  if (currentRunMode == GPS)
  {
    // When we're in GPS mode we're using the PPS pulse to increment the current encoding
    // time.
    // We don't do too much because what we want is for the encoding loop to immediately
    // change the output levels.  This should make the encoding coupled with the satellite
    // times that the GPS is receiving.  We'll have to see if any clocks use the exact modulation
    // changes as a zero point.  If so, then we're really coupling the receiving clock to
    // satellite time :)
    timeWeAreEncoding++;

    xSemaphoreGiveFromISR(semaphore_Display, &xHigherPriorityTaskWoken);

    if (xSemaphoreGiveFromISR(semaphore_Encoder, &xHigherPriorityTaskWoken) == pdTRUE)
      portYIELD_FROM_ISR();
  }
}

/////////////////////////////////////////////////////////////////////
void WiFiManagerLoop(void *)
{
  while (true)
  {
    wifiManager.process();
    vTaskDelay(50 / portTICK_RATE_MS);
  }
}

/////////////////////////////////////////////////////////////////////
void IRAM_ATTR RTCLoop(void *)
{
  // if GPS is not active then we'll do the timing based on NTP changes.
  // if the GPS is active then this will happen on it's interrupt
  while (1)
  {

    if (currentRunMode == NTP)
    {
      time_t rtc_time = HoldOnRTCClock();

      // avoid a race condition where the GPS has set the encoding time
      // and changed runmode but RTCLoop is still waiting for the RTC loop tick.
      if (currentRunMode == NTP)
      {
        timeWeAreEncoding = rtc_time;
        xSemaphoreGive(semaphore_Encoder);
        xSemaphoreGive(semaphore_Display);
        vPortYield();
      }
    }
    else
    {
      // GPS ping watchdog is here because this thread is always running.  I guess we
      // could have the watchdog in its own thread...?
      vTaskDelay(100 / portTICK_PERIOD_MS);

      if ((last_gps_ping + 1300) < millis())
      {
        currentRunMode = NTP;
        // Serial.println("switching to NTP");
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////
// You can pass in NULLs for any line that you don't want changed.  This also monitors if something has been
// updated so that we don't waste time doing IO for static updates.

void IRAM_ATTR setDisplay(String *line1 = NULL, String *line2 = NULL, String *line3 = NULL, String *line4 = NULL)
{
  bool updated = false;

  if (line1)
  {
    displayCurrentLine1 = *line1;
    updated = true;
  }
  if (line2)
  {
    displayCurrentLine2 = *line2;
    updated = true;
  }
  if (line3)
  {
    displayCurrentLine3 = *line3;
    updated = true;
  }
  if (line4)
  {
    displayCurrentLine4 = *line4;
    updated = true;
  }

  if (updated)
  {
#ifdef _HELTEC_H_
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->clear();
    Heltec.display->drawStringMaxWidth(Heltec.display->width() / 2, 0, Heltec.display->width(), displayCurrentLine1);
    Heltec.display->drawStringMaxWidth(Heltec.display->width() / 2, 16, Heltec.display->width(), displayCurrentLine2);
    Heltec.display->drawStringMaxWidth(Heltec.display->width() / 2, 32, Heltec.display->width(), displayCurrentLine3);
    Heltec.display->drawStringMaxWidth(Heltec.display->width() / 2, 48, Heltec.display->width(), displayCurrentLine4);
    Heltec.display->display();
#endif
  }
}

/////////////////////////////////////////////////////////////////////
// I feel like we can do a lot more with the screen, like wifi/GPS signal icons
void IRAM_ATTR ScreenLoop(void *pvParameters)
{

  String Date;
  String Time;
  String RunMode;
  String Line4;
  int numCycles = 0;
  bool toggleLine1 = false;
  while (true)
  {
    if ((numCycles++ % 2) == 0)

      toggleLine1 = !toggleLine1;

    char buff1[20];
    char buff2[20];

    strftime(buff1, 20, "%Y-%m-%d", localtime(&timeWeAreEncoding));
    strftime(buff2, 20, "%H:%M:%S", localtime(&timeWeAreEncoding));

    if (toggleLine1)
      Date = buff1;
    else
      Date = WiFi.localIP().toString();

    Time = buff2;

    if (currentRunMode == NTP)
      RunMode = "WWVB - NTP";
    else
      RunMode = "WWVB - GPS";

    setDisplay(&Date, &Time, &RunMode, &stringMarquee);

    // Clock ticking proccesses can cause a refresh by releasing this semaphore
    xSemaphoreTake(semaphore_Display, portMAX_DELAY);
  }
}

/////////////////////////////////////////////////////////////////////
// For tuning digital POT values
// void TestPot()
// {
//   int val1 = 0;
//   int val2 = 4;

//   for (val2 = 0; val2 < 255; val2++)
//   {
//     Serial.println(val1 + (val2 << 8));
//     delay(2000);
//   }
// }

/////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.setShowLevel(false);

#ifdef _HELTEC_H_
  Heltec.begin(true);
  Heltec.display->displayOn();
  Heltec.display->clear();
  Heltec.display->display();
#endif

  Log.notice("    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   ");
  Log.notice("     #######                    #     #                                      ");
  Log.notice("        #    # #    # ######    ##   ##   ##    ####  #    # # #    # ###### ");
  Log.notice("        #    # ##  ## #         # # # #  #  #  #    # #    # # ##   # #     ");
  Log.notice("        #    # # ## # #####     #  #  # #    # #      ###### # # #  # #####  ");
  Log.notice("        #    # #    # #         #     # ###### #      #    # # #  # # #      ");
  Log.notice("        #    # #    # #         #     # #    # #    # #    # # #   ## #      ");
  Log.notice("        #    # #    # ######    #     # #    #  ####  #    # # #    # ###### ");
  Log.notice("     >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   ");

  Log.traceln("Starting wifi");
  WiFi.mode(WIFI_STA);

  wifiManager.setConfigPortalBlocking(false);

  // automatically connect using saved credentials if they exist
  // If connection fails it starts an access point with the specified name
  wifiManager.autoConnect("TimeMachine");
  wifiManager.startWebPortal();

  Log.traceln("Configuring time zone");

  // Need way to set time zone!
  configTime(timeZoneOffsetHours * 3600, 3600, "pool.ntp.org", "time.xfinity.net");

  Log.traceln("Starting web interface");
  AsyncElegantOTA.begin(&server);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP32."); });

  server.begin();

  Log.traceln("Configuring hardware devices");
  ConfigureModulationDevices();

  semaphore_Encoder = xSemaphoreCreateBinary();
  semaphore_GPSSerialReader = xSemaphoreCreateBinary();
  semaphore_Display = xSemaphoreCreateBinary();

  Log.traceln("Creating tasks");
  // We have to run any tasks that do IO on the same core as the Ardunio environment.
  // Without pinning various IO errors occur.  On my OLED I would get a garbled screen.
  // Most of the work happens in the encoding thread and there is really nothing else running
  // so optimizing by moving processes out to the other core through queuing doesn't seem worth it.
  xTaskCreatePinnedToCore(RTCLoop, "RTC Clock Driver", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(ScreenLoop, "Screen Status Driver", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(EncoderLoop, "Signal Modulator", 4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(GPSReadLoop, "GPS Serial Processor", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(WiFiManagerLoop, "Wifi", 4096, NULL, 2, NULL, 0);

  Log.traceln("Setting up GPS interrupt handler");
  // Set up our GPS ticker interrupt
  attachInterrupt(PPSPin, &GPSTick, RISING);
}

/////////////////////////////////////////////////////////////////////
void loop() {}
