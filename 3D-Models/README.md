# 3D Model Notes

No, I haven't put any effort into a fancy case here because this whole project is nothing but a prototype, it's pretty much just a 3D printed version of what Radio Shack used to sell. The front half of the [Airhead](https://github.com/larry-athey/airhead) case and buttons are used for the LilyGo T-Display S3 here. Install your Lilygo in the front half of the case and use four M3 bolts and nuts to mount it to the **CB-Case-Top.stl** model.

<img width="1024" src="https://github.com/user-attachments/assets/58808227-68ae-4d1a-b59f-1ea3bf1fd624">

The model **CB-Flange-Gripper.stl** should be printed with TPU or other rubber-like filament so that it can be pressed tightly against the edge of the lazy susan turntable and still allow the stepper motor to turn without too much resistance against it. You can stretch this model vertically in your slicing software to fit the edge of your turntable. The following turntable was used during the development of this project. https://www.amazon.com/dp/B0C65XVDGS

A 22 mm flange from Amazon (https://www.amazon.com/dp/B0BF46M4NH) was used here. I recommend attaching the gripper to the flange with silicone and allowing it to set up, then use 10 mm M2 self-tapping screws to lock it in place. This allows you to make sure that the gripper is centered on the flange before driving screws into it.

As for the **CB-Float-Actuator.stl** model, be sure to sand down the area about 5mm below the cross member so that it doesn't bind against its opening at the end of the float arm. This actuator only needs to rise about 1mm to tell the ESP32 brain that the jar is full. But if it binds, you could end up over filling a jar and making a huge mess.

You will also likely need to progressively sand off the top edge of the **CB-Float-Actuator.stl** model to fine tune it to the break point of your optical switch. There's no way to make a one size fits all version of this actuator because all optical switches are not created equal. Remove little by little until the switch isn't tripped in the actuator's resting position.

Since the linear actuator that raises and lowers the float arm is nothing high tech, it will be noisy at times unless you add some kind of buffer on the internal slider. For example, I applied blue painter's tape to the sides and back of **CB-Riser-Inside-Pint.stl** to tame down vibration noise. Adding two layers also helps tighten up the fit between the two sections.

If you are running a larger still and need to fill quart jars, you will want to use the **CB-Riser-Inside-Quart.stl** model instead of the pint size version that I used. The vertical travel distance is still the same, you're just using a taller inside riser with the linear actuator.
