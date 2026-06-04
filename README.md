# Cutting Board
Automated robotic jar advancer for making distillation cuts. Supports pint or quart size Mason jars and requires any 12" - 13" Lazy Susan turntable.

_**This project began on February 19, 2024 and does not yet have a first official release.**_

You may contact me directly at https://panhandleponics.com<br>
Subscribe to the official YouTube channel at https://www.youtube.com/@PanhandlePonics<br>

**Facebook Idiot:** _Why would you bother?_<br>
**Me:** It's only a "bother" for somebody who couldn't do it on their best day. I'd rather have nicer things.

---
<img width="1024" alt="image" src="https://github.com/user-attachments/assets/ac67fffb-ea16-4bee-b143-bc8b68bdbcf1"><br>
---

This project is the result of numerous requests for the same additional feature to the [RPi Smart Still](https://github.com/larry-athey/rpi-smart-still) system. However, this would actually be a stand-alone device rather than a feature of that system.

_Keep in mind that this is a prototype project, not a cosmetically polished product that I intend to sell as a kit. These aren't hard to build, but if you're not a computer/electronics hobbyist, you'll want to find somebody who is to build one for you. Honestly, kids in STEM classes build this kind of stuff all the time._

Many people seem to think that it's possible for a computer to know what flavors they prefer _(or not)_ in their distillate. Sorry, AI can't even do that because it would require gas chromatography to determine what's in there at what concentrations, and that's anything but a quick process. Then the AI would need to be trained on what you like or dislike. Seriously, lay off the sci-fi movies, they're anything but based on reality.

No, Genio and iStill don't have this capability, either. The most they can do is pause the run so you can toss your heads/foreshots based on the volume and ABV of the wash in your boiler. What's more, it's just based on an approximation using simple math, there's honestly nothing high tech about what they're doing.

This device allows you to automate your cuts by automatically swapping out jars once they are filled about 1/2 inch from the top. These can be pint or quart size Mason jars placed on the 3D printed turntable segments attached to any 12" - 13" Lazy Susan turntable under your condenser or parrot outlet. The calibration procedure is simple and only needs to be performed one time.

---

_Oddly enough, even after posting numerous YouTube videos on this project, some people still think this is supposed to be something more than a robotic jar advancer with a jar-full switch. One more time for the people asleep in the back of the room, **you can't do electronic cuts by flavor compound without slowpoke gas chromatography!** Even if you did, by the time you get your results, they would say that you needed to switch jars a couple hours ago._

---

## Theory of Operation

The system design is based on two 20mm Nema 17 stepper motors, an optical switch, and a capacitve touch switch connected to a LilyGo T-Display S3 touch screen ESP32 module. One motor drives the turntable using gear drive by way of a TPU 3D printed geared pulley against the geared edge of a custom 3D printed turntable top. The other motor drives a custom linear actuator that raises and lowers an arm with a capacitive touch switch to detect when a jar is full.

Once a jar is full, the liquid will come into contact with a copper dipstick/antenna at the end of the arm to trigger a capacitive touch switch. This tells the ESP32 to raise the arm, advance the turntable 45 degrees to the next jar position, then lower the arm again. There is an optical switch is on the linear actuator frame itself to detect when it is at it's zero position. A calibration procedure runs at every startup to find the zero position of the linear actuator.

An optional feed arm may be attached that uses a 1/4" ID silicone tube from the proofing parrot to the jar that automatically kinks the tube between jar switches. Not a requirement and not really necessary if you are using wide-mouth jars. But, the travel time between small-mouth jars can lead to a bit of mess on the turntable without it.

_**NOTE:** If you are making absolute lighter fluid (the typical Still Spirits T500 fare) the capacitive touch switch may not detect when a jar is full until it's right up to the brim. This is because the density of pure ethanol is so incredibly low. Weigh a pint of water and a pint of 90% ethanol and you'll see what I mean. There's no flavor to that crap anyway, so you might as well just fill gallon jugs and not waste your time with cuts._
