# Cutting Board Electronics

You may notice that the printed circuit board used here isn't as fancy as the boards for my other projects. The reason being, I created it with the Fritzing program which is geared for hobbyists. If somebody else wants to create a fancier board, I'll happily accept it and replace the current one. PCB design just isn't my forte, EasyEDA and KiCad annoy me to no end.

Keep in mind that the LilyGo T-Display S3 mounts to the bottom side of the PCB and only uses the pins on the bottom edge of the board _(note the +3V and +5V identifiers for orientation)_. This is intentional to make the motor and sensor connections serviceable without the need to disassemble the whole controller unit.

The footprint for VR1 looks like a TO-220 mounting for an LM7803, but is intended for a small 3.3 volt buck regulator which are available from various sources that use the same footprint. I don't recommend using an LM7803 due to the heat that linear regulators generate.

Fritzing didn't allow me to label the pins of the screw terminals for the sensors. If you're looking at the screw terminals straight on with the component side of the PCB upward, the GND is on the left, the +3V is on the right, and the signal line is in the middle.

You will need to set the Vref (current limiting) for each DRV8825 to 0.5 volts. Connect both steppers and turn the potentiometers fully clockwise in advance. You need to put the controller into firmware update mode for this procedure, hold down the Value- button while powering it up and wait for the firmware update banner to appear on the screen. Use a volt meter connected to any ground and the positive lead connected to the screwdriver that you use to adjust the potentiometers. Turn them counter-clockwise until you have a reading of 0.5 volts on each one. Power cycle the controller again and you're golden, you never need to do this again.
