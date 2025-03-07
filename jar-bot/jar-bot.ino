//------------------------------------------------------------------------------------------------
// Jar-Bot - Cutting Board Brain | (CopyLeft) 2025-Present | Larry Athey (https://panhandleponics.com)
//
// The following lazy susan turntable was used during development https://www.amazon.com/dp/B0C65XVDGS
//
// This is advertised as 14", it measures 350 mm in diameter. The CB-Flange-Gripper.stl model has a
// diameter of 27 mm inside where the gripper teeth are. This will require a Nema 17 stepper motor
// to be sent 975 pulses to rotate the turntable by 45 degrees using full stepe in 5 seconds. If the
// M0 pin on the DRV8825 drivers is pulled high, this changes the motors to run quieter using half
// steps which requires 1950 steps for 45 degrees of movement in 10 seconds.
//
// Nema 17 motor to DRV8825 connections
// Black → 1A
// Green → 1B
// Blue  → 2A
// Red   → 2B
//------------------------------------------------------------------------------------------------
#include "Arduino_GFX_Library.h" // Standard GFX library for Arduino, built with version 1.4.9
#include "FreeSans9pt7b.h"       // https://github.com/moononournation/ArduinoFreeFontFile.git 
#include "Preferences.h"         // ESP32 Flash memory read/write library
#include "Wire.h"                // I2C communications library for touch-screen interface
#define TOUCH_MODULES_CST_SELF   // Tell TouchLib.h to use the CST816 chip routines
#include "TouchLib.h"            // LilyGo touch-screen interface library
//------------------------------------------------------------------------------------------------
// Internal GPIO constants
#define SCL 17                   // I2C clock pin
#define SDA 18                   // I2C data pin
#define SCREEN_BACKLIGHT 38      // Screen backlight LED pin
#define SCREEN_POWER_ON 15       // Screen power on/off pin
#define INC_BTN 0                // Value + button
#define DEC_BTN 14               // Value - button
#define TOUCH_INT 16             // CPU interrupt monitor pin for touch-screen input hook
#define TOUCH_RES 21             // Reset pin for touch-screen controller chip
// External GPIO constants
#define STEPPER_ENABLE_1 1       // Rotor stepper motor driver enable
#define STEPPER_ENABLE_2 2       // Arm stepper motor driver enable
#define STEPPER_PULSE 3          // Stepper motor pulse line (paralleled for both DRV8825 drivers)
#define STEPPER_DIRECTION 10     // Stepper motor direction (paralleled for both DRV8825 drivers)
#define FLOAT_SWITCH 11          // Optical float switch sense pin
#define ARM_ZERO 12              // Optical arm zero position sense pin
//------------------------------------------------------------------------------------------------
int RotorSize = 15600;           // Turntable circumference in motor steps
int JarDistance = 0;             // Total motor steps between jars (RotorSize / 8)
int ArmUpperPos = 18000;         // Float arm upper position (400 steps per mm of vertical lift)
int ArmLowerPos = 2000;          // Float arm lower position "                                 "
//------------------------------------------------------------------------------------------------
Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */,39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */, 45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);
Arduino_Canvas_Indexed *canvas = new Arduino_Canvas_Indexed(320 /* width */, 170 /* height */, gfx);
TouchLib Touch(Wire,SDA,SCL,CTS820_SLAVE_ADDRESS,TOUCH_RES);
//------------------------------------------------------------------------------------------------
Preferences preferences;
//------------------------------------------------------------------------------------------------
void setup() {
  // Enable serial communications for debugging output
  Serial.begin(9600);
  delay(1000);
  Serial.println("");

  // Get the last calibration settings from flash memory
  GetMemory();
  if (JarDistance == 0) {
    // New chip, flash memory not initialized
    JarDistance = RotorSize / 8;
    SetMemory();
  }

  // Initialize the touch-screen reset line
  pinMode(TOUCH_RES,OUTPUT);
  digitalWrite(TOUCH_RES,LOW);
  delay(500);
  digitalWrite(TOUCH_RES,HIGH);

  // Power up the screen and backlight
  pinMode(SCREEN_POWER_ON,OUTPUT);
  digitalWrite(SCREEN_POWER_ON,HIGH);
  ledcSetup(0,2000,8);
  ledcAttachPin(SCREEN_BACKLIGHT,0);
  ledcWrite(0,255); // Screen brightness (0-255)

  // Initialize the graphics library and blank the screen
  gfx->begin();
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);

  // In order to eliminate screen flicker, everything is plotted to an off-screen buffer and popped onto the screen when done
  canvas->begin();
  ScreenUpdate();

  InitializeArm();
}
//------------------------------------------------------------------------------------------------
void GetMemory() { // Get the last user settings from flash memory on startup
  preferences.begin("prefs",true);
  RotorSize   = preferences.getUInt("rotor_size",0);
  JarDistance = preferences.getUInt("jar_distance",0);
  ArmUpperPos = preferences.getUInt("arm_upper_pos",0);
  ArmLowerPos = preferences.getUInt("arm_lower_pos",0);
  preferences.end();
}
//------------------------------------------------------------------------------------------------
void SetMemory() { // Update flash memory with the current user settings
  preferences.begin("prefs",false);
  preferences.putUInt("rotor_size",RotorSize);
  preferences.putUInt("jar_distance",JarDistance);
  preferences.putUInt("arm_upper_pos",ArmUpperPos);
  preferences.putUInt("arm_lower_pos",ArmLowerPos);
  preferences.end();
}
//------------------------------------------------------------------------------------------------
void InitializeArm() {

}
//------------------------------------------------------------------------------------------------
void ScreenUpdate() {
  canvas->fillScreen(BLACK);

  canvas->flush();
}
//------------------------------------------------------------------------------------------------
void loop() {


}
//------------------------------------------------------------------------------------------------
