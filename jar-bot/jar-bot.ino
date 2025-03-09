//------------------------------------------------------------------------------------------------
// Jar-Bot - Cutting Board Brain | (CopyLeft) 2025-Present | Larry Athey (https://panhandleponics.com)
//
// The following lazy susan turntable was used during development https://www.amazon.com/dp/B0C65XVDGS
// This is advertised as 14", but measures 350 mm in diameter.
//
// Nema 17 motor to DRV8825 connections
// Black → 1A
// Green → 1B
// Blue  → 2A
// Red   → 2B
//
// 1/8 step (1600 steps per revolution) M0-High, M1-High, M2-Low
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
#define ARM_ZERO_SWITCH 12       // Optical arm zero position sense pin
//------------------------------------------------------------------------------------------------
bool GotInterrupt = false;       // True if touch input has been detected on the screen
int RotorSize = 20736;           // Rotor (turntable) circumference in motor steps
int JarDistance = RotorSize / 8; // Total motor steps between jars (motor steps to move 45 degrees)
int ArmUpperPos = 64000;         // Float arm upper position (1600 steps per mm of vertical lift)
int ArmLowerPos = 32000;         // Float arm lower position "                                  "
int ArmCurrentPos = 0;           // Current vertical position of the float arm
int StepperPulse = 965;          // Stepper motor pulse width per on/off state (microseconds)
int MotorSteps = 1600;           // Stepper motor steps per revolution
int OneDegree = MototSteps / 360;// Stepper motor steps for one degree of rotation
byte CurrentMode = 1;            // 1 = Home Screen, 2 = Rotor Config, 3 = Float Arm Config
byte ActiveButton = 0;           // Currently selected touch-screen button
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

  // Initialize the external device GPIO pins
  pinMode(STEPPER_ENABLE_1,OUTPUT); digitalWrite(STEPPER_ENABLE_1,LOW);
  pinMode(STEPPER_ENABLE_2,OUTPUT); digitalWrite(STEPPER_ENABLE_2,LOW);
  pinMode(STEPPER_PULSE,OUTPUT);
  pinMode(STEPPER_DIRECTION,OUTPUT);
  pinMode(FLOAT_SWITCH,INPUT_PULLUP);    // GPIO pin goes low when the light beam is obstructed
  pinMode(ARM_ZERO_SWITCH,INPUT_PULLUP); // "                                                 "

  // Get the last calibration settings from flash memory
  GetMemory();
  if (JarDistance == 0) {
    // New chip, flash memory not initialized
    RotorSize   = 20736;
    ArmUpperPos = 64000;
    ArmLowerPos = 32000;
    SetMemory();
  }

  // Initialize the touch-screen reset line
  pinMode(TOUCH_RES,OUTPUT);
  digitalWrite(TOUCH_RES,LOW);
  delay(500);
  digitalWrite(TOUCH_RES,HIGH);

  Wire.begin(SDA,SCL);
  if (Touch.init()) {
    Serial.println("Touch screen interface initialized");
    attachInterrupt(TOUCH_INT,[] { GotInterrupt = true; },FALLING);
  } else {
    Serial.println("Touch screen interface not detected");
  }

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

  // Initialize the float arm by raising it 5 mm and then lowering it until the lower limit switch triggers
  InitializeArm();
  // Raise the float arm to its upper position
  SetArmPos(ArmUpperPos);
  // Move the rotor 45 degrees forward
  SwitchJars(1);
  // Move the rotor back
  SwitchJars(0);
  // Lower the float arm to its down position
  SetArmPos(ArmLowerPos);
}
//------------------------------------------------------------------------------------------------
void GetMemory() { // Get the last calibration settings from flash memory on startup
  preferences.begin("prefs",true);
  RotorSize   = preferences.getUInt("rotor_size",0);
  ArmUpperPos = preferences.getUInt("arm_upper_pos",0);
  ArmLowerPos = preferences.getUInt("arm_lower_pos",0);
  preferences.end();
  JarDistance = RotorSize / 8;
}
//------------------------------------------------------------------------------------------------
void SetMemory() { // Update flash memory with the current calibration settings
  preferences.begin("prefs",false);
  preferences.putUInt("rotor_size",RotorSize);
  preferences.putUInt("arm_upper_pos",ArmUpperPos);
  preferences.putUInt("arm_lower_pos",ArmLowerPos);
  preferences.end();
  JarDistance = RotorSize / 8;
}
//------------------------------------------------------------------------------------------------
void InitializeArm() { // Set the float arm to it's lower limit position to locate absolute zero
  digitalWrite(STEPPER_ENABLE_2,HIGH);
  delay(10);
  digitalWrite(STEPPER_DIRECTION,HIGH); // Clockwise
  for (int x = 1; x <= (MotorSteps * 5); x ++) { // Raise the arm 5 mm (threaded rod with 1 mm pitch)
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(StepperPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(StepperPulse);
  }
  digitalWrite(STEPPER_DIRECTION,LOW); // Counter-Clockwise
  while(digitalRead(ARM_ZERO_SWITCH) == HIGH) { // Lower the arm to find zero
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(StepperPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(StepperPulse);
  }
  digitalWrite(STEPPER_ENABLE_2,LOW);
}
//------------------------------------------------------------------------------------------------
void SetArmPos(int Position) { // Move the float arm up or down to a specific position
  int Steps;

  if (Position > ArmCurrentPos) {
    digitalWrite(STEPPER_DIRECTION,HIGH); // Clockwise
    Steps = Position - ArmCurrentPos;
  } else if (Position < ArmCurrentPos) {
    digitalWrite(STEPPER_DIRECTION,LOW); // Counter-Clockwise
    Steps = ArmCurrentPos - Position;
  } else {
    return;
  }
  digitalWrite(STEPPER_ENABLE_2,HIGH);
  delay(10);
  for (int x = 1; x <= Steps; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(StepperPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(StepperPulse);
    if (digitalRead(ARM_ZERO_SWITCH) == LOW) break;
  }
  digitalWrite(STEPPER_ENABLE_2,LOW);
}
//------------------------------------------------------------------------------------------------
void SwitchJars(byte Direction) { // Rotates the turntable/rotor 45 degrees forward or backward
  digitalWrite(STEPPER_DIRECTION,Direction);
  digitalWrite(STEPPER_ENABLE_1,HIGH);
  delay(10);
  for (int x = 1; x <= JarDistance; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(StepperPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(StepperPulse);
  }
  digitalWrite(STEPPER_ENABLE_1,LOW);
}
//-----------------------------------------------------------------------------------------------
void JarAdvance(byte Direction) { // Lift the arm, switch jars, lower the arm
  SetArmPos(ArmUpperPos);
  SwitchJars(Direction);
  SetArmPos(ArmLowerPos);
}
//-----------------------------------------------------------------------------------------------
void DrawButton(byte WhichOne) { // Draws and highlights the specified button on the screen

}
//------------------------------------------------------------------------------------------------
void ScreenUpdate() { // Plot the off-screen buffer and then pop it to the touch screen display
  canvas->fillScreen(BLACK);

  canvas->flush();
}
//-----------------------------------------------------------------------------------------------
bool RegionPressed(int Xpos,int Ypos,int X1,int Y1,int X2,int Y2) { // Screen button press evaluator
  if ((Xpos >= X1) && (Xpos <= X2) && (Ypos >= Y1) && (Ypos <= Y2)) {
    return true;
  } else {
    return false;
  }
}
//-----------------------------------------------------------------------------------------------
void ProcessTouch(int Xpos,int Ypos) { // Handle touch-screen presses
  // If Xpos is a negative number, the user has pressed the home button, Ypos is irrelevant

}
//------------------------------------------------------------------------------------------------
void IncValue(byte WhichOne) { // Increment the value associated with the active screen button
  /*if (WhichOne == 0) {
    if (CurrentMode < 3) CurrentMode ++;
  } else if (WhichOne == 2) {
    if (UserPower < 100) UserPower ++;
  } else if (WhichOne == 3) {
    if (UserTemp1 < 100) UserTemp1 ++;
  } else if (WhichOne == 4) {
    if (UserTemp1 < 100) UserTemp1 ++;
  } else if (WhichOne == 5) {
    if (UserTemp2 < 100) UserTemp2 ++;
  } else if (WhichOne == 6) {
    if (UserTime < 24) UserTime ++;
  }*/
  ScreenUpdate();
}
//-----------------------------------------------------------------------------------------------
void DecValue(byte WhichOne) { // Decrement the value associated with the active screen button
  /*if (WhichOne == 0) {
    if (CurrentMode > 1) CurrentMode --;
  } else if (WhichOne == 2) {
    if (UserPower > 10) UserPower --;
  } else if (WhichOne == 3) {
    if (UserTemp1 > 30) UserTemp1 --;
  } else if (WhichOne == 4) {
    if (UserTemp1 > 30) UserTemp1 --;
  } else if (WhichOne == 5) {
    if (UserTemp2 > 30) UserTemp2 --;
  } else if (WhichOne == 6) {
    if (UserTime > 1) UserTime --;
  }*/
  ScreenUpdate();
}
//-----------------------------------------------------------------------------------------------
void ProcessButton(byte WhichOne) { // Handle increment/decrement button inputs
  byte HoldCounter = 0;

  if (WhichOne == 1) {
    // Increment active screen button value by 1
    IncValue(ActiveButton);
    while (digitalRead(INC_BTN) == 0) {
      delay(10);
      HoldCounter ++;
      if (HoldCounter == 150) { // User is intentionally holding the + button
        while (digitalRead(INC_BTN) == 0) {
          IncValue(ActiveButton);
          delay(250);
        }
      }
    }
  } else {
    // Decrement active screen button value by 1
    DecValue(ActiveButton);
    while (digitalRead(DEC_BTN) == 0) {
      delay(10);
      HoldCounter ++;
      if (HoldCounter == 150) { // User is intentionally holding the - button
        while (digitalRead(DEC_BTN) == 0) {
          DecValue(ActiveButton);
          delay(250);
        }
      }
    }
  }
  if (CurrentMode > 1) SetMemory();
}
//------------------------------------------------------------------------------------------------
void loop() {
  // Check for touch-screen presses and handle as necessary
  if (GotInterrupt) {
    if (Touch.read()) {
      int Xpos,Ypos;
      TP_Point Point = Touch.getPoint(0);
      Xpos = 320 - Point.y; // Touch.setRotation() doesn't work for some reason
      Ypos = Point.x;       // "                                              "
      ProcessTouch(Xpos,Ypos);
      while (Touch.read()) delay(10);
    }
    GotInterrupt = false;
  }
  // Check for Value+ keypresses and handle as necessary
  if (digitalRead(INC_BTN) == 0) ProcessButton(1);
  // Check for Value- keypresses and handle as necessary
  if (digitalRead(DEC_BTN) == 0) ProcessButton(0);
  // If not in configuation mode, check the float switch and advance to the next jar if necessary
  if ((CurrentMode == 1) && (digitalRead(FLOAT_SWITCH) == LOW)) JarAdvance(1);
  // Give the CPU a break between loops
  delay(10);
}
//------------------------------------------------------------------------------------------------
