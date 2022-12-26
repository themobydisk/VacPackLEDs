# Areduino Studio code for VacPack Cosplay Gun
This C++ code powers [my build](https://www.thingiverse.com/make:1063546) of the [Slime Rancher VacPack Cosplay Gun](https://www.thingiverse.com/thing:5366906).  It can be built using the Arduino Studio.  I have tested this on an Arduino Nano and Arduino Uno.

[![Image of VacPack in action](http://img.youtube.com/vi/fQHmXDEwyE4/0.jpg)](http://www.youtube.com/watch?v=fQHmXDEwyE4 "Slime Rancher VacPack")

## Features
- Commented C++ code
- Green light pattern when sucking up slimes
- Red light pattern for shooting slimes
- Pulsing light pulsing pattern during idle
- Sound effects from the game
- Sound library included

## Dependencies
Requires the FastLED Arduino library which can be downloaded for free using Arduino Studio.

## Hardware/Electrical
### Arduino Pinout
|Pin|Description|
|---|-----------|
|D6|WS2812B LED strip|
|D7|Suck button|
|D8|Shoot button|
|D11|Speaker transistor|

### Additional Hardware
The LED strip is a standard 144-LED WS2812B strip.  The number of LEDs can be adjusted in the software, you will see the version checked-in here has 6 LEDs removed.

Any small speaker will do, but I used a 2″  4Ω mini-speaker from [MPJA](https://www.mpja.com/), powered using a TIP31C.

Using the above configuration, my build drew 270mA at idle and 613mA at peak.  The power consumption was largely from the speaker, but the LEDs can consume quite a lot, especially if the brightness is increased.

### 3D printed model
Kudos to crt404 for his project.  For information about the print, visit the Thingiverse links above.
