# TimeMachine
## Radio Controlled Clock Synchronization with a 2M range

![The TimeMachine](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachine.gif?raw=true)
#### Features
 - Supports both NTP and GPS time sync and auto-switches between them (GPS priority)
 - When in GPS mode uses the GPS PPS (Pulse Per Second) to control modulation exactly in sync with GPS
 - Uses the cheap and available ESP32 platform which has two CPU cores and plenty of RAM
 - Built on top of RTOS / Arduino and uses  task/interrupt based programming models
 - Uses an AD9833 wave generator to generate an accurate 60khz sine wave carrier frequency
 - Utilizes an LM358 opamp to create an amplifier
 - Uses a digital dual potentiometer to control amplifier gain and attenuation for modulation

#### Background

TimeMachine is a project I've been thinking about for years.  I have several WWVB based clocks around my house and in the spring time change they have reception issues that cause them not to sync.  I've waited up to a couple of weeks in some cases to see if they would eventually pick up a signal to no avail.

Over the past few years I've looked at some of the options out there to broadcast a simulated WWVB signal.  There are many open source projects on Github that use various methods to accomplish this.  There is even an iPhone app that uses the iPhone speaker to generate enough of a signal that it can sync the correct time.  All of these required taking down the clocks and placing them very close to whichever device was transmitting.  Many of the projects were incomplete with non-working code or hardware.  Some were based on Arduino boards that had no RTC or wifi so could only send a pre-programmed time.

After trying several boards I found that the Heltech WIFI 32 board was almost perfect for this task.   Based on the ESP32 architecture, it is dual core and has a lot of RAM. It has an OLED display built in which would allow me to program a status display which would allow me to see what the system was doing at any given time.  

### The board
![The TimeMachine](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachineBoard.jpg?raw=true)

### Environment
For development I'm using Visual Studio with the PlatformIO plugin.  Until the capability to set time zone in a web interface is implemented, you should begin by editing TimeMachine.cpp with your correct time zone offset.  Look for these lines:

// Set this to your time zone offset w/o consideration of DST.
// Eventually add a setting in the web portal.
int  timeZoneOffsetHours = -5;

-5 is the correct value for eastern time.

### The Schematic

Here is the wiring schematic.  Note that in my build only one 3.3v lead is connected to the bread board bus.  

If you were going to power this from USB only, you could connect VCC to the 5 volt output.  This board is capable of being powered from a battery, but what I found is the 5V output changes to the level of the battery, resulting in things not working quite right.  5V output will better align to the capabilities of the LM358 and produce a better sine wave output.

![enter image description here](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachineSchematic.png?raw=true)

### Parts List

[Heltec WiFi Kit 32 with OLED Screen](https://www.amazon.com/HiLetgo-Display-Bluetooth-Internet-Development/dp/B07DKD79Y9/)

[MCP41100-I/P Dual Digital Potentiometer](https://www.digikey.com/en/products/detail/microchip-technology/MCP42100-I-P/362090)

[LM358 OpAmp](https://www.amazon.com/gp/product/B077BR9KT2/)

[AD9833 Programmable Signal Generator Breakout Board](https://www.amazon.com/gp/product/B08PZ5FR51)

[NEO 6M GPS ](https://www.amazon.com/gp/product/B07P8YMVNT/)

[60Khz tuned antenna](https://www.amazon.com/gp/product/B07PK7WJYR/)
