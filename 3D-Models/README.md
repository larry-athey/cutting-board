# 3D Model Notes

No, I haven't put any effort into a fancy case here because this whole project is nothing but a prototype, it's pretty much just a 3D printed version of what Radio Shack used to sell. The front half of the [Airhead](https://github.com/larry-athey/airhead) case and buttons are used for the LilyGo T-Display S3 here. Install your Lilygo in the front half of the case and use four M3 bolts and nuts to mount it to the **CB-Case-Top.stl** model.

Ultimately, my plan is to have the case for the LilyGo T-Display S3 and all of the electronics joined together with the motor mount and linear actuator. This project kinda got put on the back burner because of other tasks, so this is a slow work in progress.

<img width="1024" src="https://github.com/user-attachments/assets/58808227-68ae-4d1a-b59f-1ea3bf1fd624">

The model **CB-Flange-Gripper.stl** should be printed with TPU or other rubber-like filament to reduce noise and vibrations. A 22 mm flange from Amazon (https://www.amazon.com/dp/B0BF46M4NH) was used to attach it to the stepper motor. I recommend attaching the gripper to the flange with silicone and allowing it to set up, then use 10 mm M2 self-tapping screws to lock it in place. This allows you to make sure that the gripper is centered on the flange before driving screws into it.

Since the linear actuator that raises and lowers the float arm is nothing high tech, it will be noisy at times unless you add some kind of buffer on the internal slider. For example, I applied blue painter's tape to the sides and back of **CB-Riser-Inside-Pint.stl** to tame down vibration noise. Most arts & crafts stores sell velvet tape to prevent items from scratching table tops, which would probably be a better choice in this case.

The model **CB-Turntable-Segment.stl** must be printed four times and needs to be attached to the top of a 12" to 13" Lazy Susan to form a full circle. These provide placement markers for your jars and the gear drive that the model **CB-Flange-Gripper.stl** meshes with. You will want to apply some sort of non-skid surface or silicone coasters to the placement markers to keep your jars in place and immune to vibrations. Make absolutely sure that these four segments are perfectly centered and there are no gaps between them.

If you are running a larger still and need to fill quart jars, you will want to use the **CB-Riser-Inside-Quart.stl** model instead of the pint size version that I used. The vertical travel distance is still the same, you're just using a taller inside riser with the linear actuator.
