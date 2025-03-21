# Cutting Board
Automated robotic jar advancer for making distillation cuts. Supports pint or quart size Mason jars and any diameter Lazy Susan turntable.

_**This project began on February 19, 2024 and does not yet have a first official release.**_

You may contact me directly at https://panhandleponics.com<br>
Subscribe to the official YouTube channel at https://www.youtube.com/@PanhandlePonics<br>

**Facebook Idiot:** _Why would you bother?_<br>
**Me:** It's only a "bother" for somebody who couldn't do it if they had to. I'd rather have nicer things.

---

This project is the result of numerous requests for the same additional feature to the [RPi Smart Still](https://github.com/larry-athey/rpi-smart-still) system. However, this would actually be a stand-alone device rather than a feature of that system.

Keep in mind that this is a prototype project, not a cosmetically polished product that I intend to sell as a kit. These aren't hard to build, but if you're not a computer/electronics hobbyist, you'll want to find somebody who is to build it for you. Honestly, kids in STEM classes build this kind of stuff all the time.

Many people seem to think that it's possible for a computer to know what flavors they prefer _(or not)_ in their distillate. Sorry, AI can't even do that because it would require gas chromatography to determine what's in there at what concentrations, and that's anything but a quick process. Then the AI would need to be trained on what you like or dislike. Seriously, lay off the sci-fi movies, they're anything but based on reality.

No, Genio and iStill don't have this capability, either. The most they can do is pause the run so you can toss your heads/foreshots based on the volume and ABV of the wash in your boiler. What's more, it's just based on an approximation using simple math, there's honestly nothing high tech about it.

This device allows you to automate your cuts by automatically swapping out jars once they are filled about one inch from the top. These can be pint or quart size Mason jars placed on any diameter Lazy Susan turntable under your condenser outlet. You simply calibrate the controller to the circumference of your turntable, then cycle through the eight 45 degree positions and put your jars in place on the turntable.

## Theory of Operation

The system design is based on two 20mm Nema 17 stepper motors and two optical switches connected to a LilyGo T-Display S3 touch screen ESP32 module. One motor drives the turntable using friction drive by way of a TPU 3D printed geared pulley against the edge of the turntable. The other motor drives a 3D printed DIY linear actuator that raises and lowers an arm with a float switch to detect when a jar is full.

Once a jar is full, the float will rise and break the light beam of an optical switch. This tells the ESP32 to raise the arm, advance the turntable 45 degrees to the next jar position, then lower the arm again. The other optical switch is on the linear actuator frame itself to detect when it is at it's zero position. A calibration procedure runs at every startup to find the zero position of the linear actuator.
