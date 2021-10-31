# TimeMachine
## Radio Controlled Clock Synchronization with a 2M range

![enter image description here](https://share.icloud.com/photos/0ovHpJlWq_fS7lH9GW5bUdTlQ)


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