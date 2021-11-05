
# TimeMachine

## WWVB Controlled Clock NTP/GPS Synchronization with up to a 2M range

  

![The TimeMachine](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachine.gif?raw=true)

#### Features

- Supports both NTP and GPS time sync and auto-switches between them (GPS priority)

- When in GPS mode uses the GPS PPS (Pulse Per Second) to control modulation exactly in sync with GPS

- Uses the cheap and available ESP32 platform which has two CPU cores and plenty of RAM

- The Heltec board can be powered by and recharge a lithium ion battery to help get

- Built on top of RTOS / Arduino and uses task/interrupt based programming models

- Uses an AD9833 wave generator to generate an accurate 60khz sine wave carrier frequency

- Uses a sine wave generator coupled to an amplifier to create a more efficient and less noisy RF transmitter

- Uses a digital dual potentiometer to control amplifier gain and attenuation for modulation

  

#### Background

  

TimeMachine is a project I've been thinking about for years. I have several WWVB based clocks around my house and in the spring time change they have reception issues that cause them not to sync. I've waited up to a couple of weeks in some cases to see if they would eventually pick up a signal to no avail.

Over the past few years I've looked at some of the options out there to broadcast a simulated WWVB signal. There are many open source projects on Github that use various methods to accomplish this. There is even an iPhone app that uses the iPhone speaker to generate enough of a signal that it can sync the correct time. All of these required taking down the clocks and placing them very close to whichever device was transmitting. Many of the projects were incomplete with non-working code or hardware. Some were based on Arduino boards that had no RTC or wifi so could only send a pre-programmed time.

After trying several boards I found that the Heltech WIFI 32 board was almost perfect for this task. Based on the ESP32 architecture, it is dual core and has a lot of RAM. It has an OLED display built to allow a status display to see what the system is doing at any given time.  I added a marquee that displays the bits being broadcasted.

  ### Call out
  One of the most difficult parts of creating a time transmitter is understanding the math behind how the time is modulated into a radio signal using pulses in the AM signal.  I didn't have the time or patience to write this part myself so I looked to the work others have done already.  I tried the code from several transmitters before settling on using the code from Henner Zeller's [txtempus](https://github.com/hzeller/txtempus).  This project has modulators for many different radio clocks and was easy to implement with a minor change.  It was also the simplest and most comprehensive out of all of them.  


### The board

![The TimeMachine](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachineBoard.jpg?raw=true)

  ### What's next
 - Add time zone selection in the web config interface.  This only affects the display, the radio only broadcasts in GMT so local tz does not matter.
 - It should be easy to implement the rest of the txttempus radio encoders to allow to broadcast in any station's format
 - Implement a PWM modulator to allow folks that don't need the range to use a simple wire antenna connected to the ESP32's DAC instead of the current radio
 - Add code to be more creative with the OLED display, like icons or number of satellites
 - Add a voltage booster to the circuit to more appropriately feed the LM358
 - Make encoder calculations a seperate task so that it can be run on a free core
 - Use the internal ADC to measure the output voltage and tune the DAC's automagically
 - Using the Arduino layer adds a lot of overhead and prevents us from properly utilizing both cores since it forces us to do all IO on the same core as the Arduino library.  What it brings is all the hardware drivers that TimeMachine depends on.  We could port out the drivers then move it to native espressif32.

### Environment

For development I'm using Visual Studio with the PlatformIO plugin. Until the capability to set time zone in a web interface is implemented, you should begin by editing TimeMachine.cpp with your correct time zone offset. Look for these lines:

    // Set this to your time zone offset w/o consideration of DST.
    // Eventually add a setting in the web portal.
    int timeZoneOffsetHours = -5;
    -5 is the correct value for eastern time.

When you open the project you'll see there are a few different build targets.  If you build debug you'll get lots of serial output with the tradeoff that your clock transmissions might be off by a couple of MS.  If you're not going to monitor the serial port then I recommend you use the release code.

The wrover target is in there for debugging which is possible on an esp32 wrover reference board.  You may need to comment out the #include<heltec.h>.

### The Schematic

Here is the wiring schematic. Note that in my build only one 3.3v lead is connected to the bread board bus.

If you were going to power this from USB only, you could connect VCC to the 5 volt output. This board is capable of being powered from a battery, but what I found is the 5V output changes to the level of the battery, resulting in things not working quite right. 5V output will better align to the capabilities of the LM358 and produce a better sine wave output.

  

![enter image description here](https://github.com/kevinherzig/TimeMachine/blob/master/img/TimeMachineSchematic.png?raw=true)

  

### Parts List

[Heltec WiFi Kit 32 with OLED Screen](https://www.amazon.com/HiLetgo-Display-Bluetooth-Internet-Development/dp/B07DKD79Y9/)

  

[MCP41100-I/P Dual Digital Potentiometer](https://www.digikey.com/en/products/detail/microchip-technology/MCP42100-I-P/362090)

  

[LM358 OpAmp](https://www.amazon.com/gp/product/B077BR9KT2/)

  

[AD9833 Programmable Signal Generator Breakout Board](https://www.amazon.com/gp/product/B08PZ5FR51)

  

[NEO 6M GPS ](https://www.amazon.com/gp/product/B07P8YMVNT/)

  

[60Khz tuned antenna](https://www.amazon.com/gp/product/B07PK7WJYR/)
