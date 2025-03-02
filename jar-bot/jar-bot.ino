//------------------------------------------------------------------------------------------------
// Jar-Bot - Cutting Board Brain | (CopyLeft) 2025-Present | Larry Athey (https://panhandleponics.com)
//
// The following lazy susan turntable was used during development https://www.amazon.com/dp/B0C65XVDGS
//
// This is advertised as 14", it measures 350 mm in diameter. The CB-Flang-Gripper.stl model has a
// diameter of 27 mm inside where the gripper teeth are. This will require a Nema 17 stepper motor
// to be sent 324 pulses to rotate the turntable by 45 degrees. This will allow 8 pint size Mason
// jars to be used for cuts and may be large or small mouth jars.
//------------------------------------------------------------------------------------------------
#include "Arduino_GFX_Library.h" // Standard GFX library for Arduino, built with version 1.4.9
#include "FreeSans9pt7b.h"       // https://github.com/moononournation/ArduinoFreeFontFile.git 
#include "Preferences.h"         // ESP32 Flash memory read/write library
#define TOUCH_MODULES_CST_SELF   // Tell TouchLib.h to use the CST816 chip routines
#include "TouchLib.h"            // LilyGo touch-screen interface library
//------------------------------------------------------------------------------------------------
Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */,39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */, 45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);
Arduino_Canvas_Indexed *canvas = new Arduino_Canvas_Indexed(320 /* width */, 170 /* height */, gfx);
TouchLib Touch(Wire,SDA,SCL,CTS820_SLAVE_ADDRESS,TOUCH_RES);
//------------------------------------------------------------------------------------------------
Preferences preferences;
//------------------------------------------------------------------------------------------------
void setup() {


}
//------------------------------------------------------------------------------------------------
void loop() {


}
//------------------------------------------------------------------------------------------------
