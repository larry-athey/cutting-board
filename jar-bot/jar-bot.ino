//------------------------------------------------------------------------------------------------
// Jar-Bot - Cutting Board Brain | (CopyLeft) 2025-Present | Larry Athey (https://panhandleponics.com)
//
// Tested up to Espressif ESP32 v2.0.14 due to LilyGo requirements
//
// This code requires the LilyGo T-Display-S3 ESP32 board. You will first need to properly set up
// your Arduino IDE to upload to that board. See: https://github.com/Xinyuan-LilyGO/T-Display-S3
// Make sure that you only install the Espressif ESP32 v2.0.14 board library since the 3.x stuff
// is not backward compatible and will throw syntax errors all over the place. (Nice fkin upgrade)
//
// I have no plans of making this project a turn key kit and I'm sure that other people can come up
// with better ways of driving the turntable and detecting when a jar is full. I would have used a
// timing belt to drive the turntable, but it would have required me to have a router table to make
// a seat for one to be glued around the edge of the turntable to turn it into a big gear. Friction
// drive was my only option.
//
// The following lazy Susan turntable was used during development https://www.amazon.com/dp/B0C65XVDGS
// This is advertised as 14", but measures 350 mm in diameter.
//
// Determining the liquid level in the jar is tricky because ethanol isn't conductive and it's not
// exactly possible to do it optically without the use of a camera and a boatload of crazy code to
// perform comparisons between the empty appearance of the jar and what it looks like now. So, the
// float and a zero-force switch was all that I could think of.
//
// If somebody else wants to expand upon this project and make it all pretty, as well as design a
// printed circuit board to simplify assembly, be my guest! I only took on this project due to a
// number of requests since I uploaded the first video introducing the RPi Smart Still controller
// system. It seems that some people think the smart still controller is lacking because it can't
// do automatic cuts. Never mind the fact that Genio and iStill also don't do that either.
//
// Well, here ya go, it can do automatic cuts now. I've never been one of those cuts guys, I just
// redistill everything until it smooths out and it usually only requires one time unless it's rum
// or something absolute neutral. I'm mainly an all-grain bourbon distiller, single redistillation
// and it instantly goes on toasted oak chips to age. I've never had a single complaint about it.
//
// One of the main reasons that I wouldn't sell the existing prototype system as a kit is because
// the friction drive has a small degree of slip and occasionally requires a bump on the + button
// to push the turntable ahead. Not a big deal to me, but I'm sure that somebody out there would
// pitch a fit over it. So, once again, if you want one of these, you'll have to build your own.
// 
// NOTE: The DRV8825 for the float arm is configured for 1/4 steps and 1/8 steps for the rotor.
//       Refer to the M0/M1/M2 connections in the schematic. Adjust the Vref for each DRV8825
//       driver to 0.32V while the motors are running.
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
#define STEPPER_DIRECTION 3      // Stepper motor direction (paralleled for both DRV8825 drivers)
#define STEPPER_PULSE 10         // Stepper motor pulse line (paralleled for both DRV8825 drivers)
#define FLOAT_SWITCH 11          // Optical float switch sense pin
#define ARM_ZERO_SWITCH 12       // Optical arm zero position sense pin
//------------------------------------------------------------------------------------------------
bool GotInterrupt = false;       // True if touch input has been detected on the screen
long LoopCounter = 0;            // Used for debouncing the touch screen
int RotorSize = 20736;           // Rotor (turntable) circumference in motor steps
int JarDistance = RotorSize / 8; // Total motor steps between jars (motor steps to move 45 degrees)
int ArmUpperPos = 32000;         // Float arm upper position (800 steps per mm of vertical lift)
int ArmLowerPos = 16000;         // Float arm lower position "                                 "
int ArmCurrentPos = 0;           // Current vertical position of the float arm
int ArmPulse = 200;              // Float Arm stepper pulse width per on/off state (microseconds)
int RotorPulse = 800;            // Rotor stepper pulse width per on/off state (microseconds)
int RotorSteps = 1600;           // Rotor stepper motor steps per revolution
int ArmSteps = 800;              // Float arm stepper motor steps per revolution
byte CurrentMode = 1;            // 1 = Home Screen, 2 = Rotor Config, 3 = Float Arm Config
byte ActiveButton = 1;           // Currently selected touch-screen button
String Version = "1.0.1";        // Current release version of the project
//------------------------------------------------------------------------------------------------
// Home screen button coordinates (CurrentMode = 1)
int PrevJarX1 = 0, PrevJarY1 = 0, PrevJarX2 = 158, PrevJarY2 = 84;
int NextJarX1 = 160, NextJarY1 = 0, NextJarX2 = 319, NextJarY2 = 84;
int RotConfX1 = 0, RotConfY1 = 86, RotConfX2 = 158, RotConfY2 = 169;
int ArmConfX1 = 160, ArmConfY1 = 86, ArmConfX2 = 319, ArmConfY2 = 169;
// Configuration button coordinates (CurrentMode = 2/3)
int Conf1_X1 = 0, Conf1_Y1 = 86, Conf1_X2 = 105, Conf1_Y2 = 169;
int Conf2_X1 = 107, Conf2_Y1 = 86, Conf2_X2 = 213, Conf2_Y2 = 169;
int Conf3_X1 = 215, Conf3_Y1 = 86, Conf3_X2 = 319, Conf3_Y2 = 169;
// RGB values for the button and text colors
#define JARBTN RGB565(0,215,0)
#define MODEBTN RGB565(50,50,230)
#define CONFBTN RGB565(0,210,210)
#define TESTBTN RGB565(0,215,0)
#define HILITE RGB565(230,230,230)
#define BTNTEXT RGB565(245,245,245)
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
    if (Serial) Serial.println("Initializing flash memory");
    RotorSize   = 20736;
    ArmUpperPos = 32000;
    ArmLowerPos = 16000;
    SetMemory();
  }

  // Initialize the touch-screen reset line
  pinMode(TOUCH_RES,OUTPUT);
  digitalWrite(TOUCH_RES,LOW);
  delay(500);
  digitalWrite(TOUCH_RES,HIGH);

  Wire.begin(SDA,SCL);
  if (Touch.init()) {
    pinMode(TOUCH_INT,INPUT_PULLUP);
    attachInterrupt(TOUCH_INT,[] { GotInterrupt = true; },FALLING);
    if (Serial) Serial.println("Touch screen interface initialized");
  } else {
    if (Serial) Serial.println("Touch screen interface not detected");
  }

  // Power up the screen and backlight
  pinMode(SCREEN_POWER_ON,OUTPUT);
  digitalWrite(SCREEN_POWER_ON,HIGH);
  ledcSetup(0,2000,8);
  ledcAttachPin(SCREEN_BACKLIGHT,0);
  ledcWrite(0,255); // Screen brightness (0-255)
  pinMode(INC_BTN,INPUT_PULLUP);
  pinMode(DEC_BTN,INPUT_PULLUP);

  // Initialize the graphics library and blank the screen
  gfx->begin();
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);

  // In order to eliminate screen flicker, everything is plotted to an off-screen buffer and popped onto the screen when done
  canvas->begin();
  ScreenUpdate();

  // Initialize the float arm by raising it 5 mm and then lowering it until the lower limit switch triggers
  PopoverMessage("Calibrating Float Arm");
  InitializeArm();
  // Move the float arm to its down position
  SetArmPos(ArmLowerPos);
  ScreenUpdate();

  LoopCounter = millis();
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
  digitalWrite(STEPPER_DIRECTION,LOW);
  for (int x = 1; x <= (ArmSteps * 5); x ++) { // Raise the arm 5 mm (threaded rod with 1 mm pitch)
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(ArmPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(ArmPulse);
  }
  digitalWrite(STEPPER_DIRECTION,HIGH);
  while(digitalRead(ARM_ZERO_SWITCH) == HIGH) { // Lower the arm to find zero
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(ArmPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(ArmPulse);
  }
  digitalWrite(STEPPER_ENABLE_2,LOW);
}
//------------------------------------------------------------------------------------------------
void SetArmPos(int Position) { // Move the float arm up or down to a specific position
  int Steps;
  bool Limit = false;
  long Uptime = millis();

  if (Position > ArmCurrentPos) {
    digitalWrite(STEPPER_DIRECTION,LOW);
    Steps = Position - ArmCurrentPos;
  } else if (Position < ArmCurrentPos) {
    digitalWrite(STEPPER_DIRECTION,HIGH);
    Steps = ArmCurrentPos - Position;
  } else {
    return;
  }
  digitalWrite(STEPPER_ENABLE_2,HIGH);
  delay(10);
  for (int x = 1; x <= Steps; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(ArmPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(ArmPulse);
    if ((Uptime > 15000) && (digitalRead(ARM_ZERO_SWITCH) == LOW)) {
      ArmCurrentPos = 0;
      Limit = true;
      break;
    }
  }
  digitalWrite(STEPPER_ENABLE_2,LOW);
  if (! Limit) ArmCurrentPos = Position;
}
//------------------------------------------------------------------------------------------------
void BumpArm(byte Direction, int Steps, bool Hold) {
  if ((Direction == 0) && (digitalRead(ARM_ZERO_SWITCH) == LOW)) {
    ArmCurrentPos = 0;
    return;
  }
  if (Direction == 1) {
    digitalWrite(STEPPER_DIRECTION,LOW);
  } else {
    digitalWrite(STEPPER_DIRECTION,HIGH);
  }
  if (! Hold) {
    digitalWrite(STEPPER_ENABLE_2,HIGH);
    delay(10);
  }
  for (int x = 1; x <= Steps; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(ArmPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(ArmPulse);
    if (digitalRead(ARM_ZERO_SWITCH) == LOW) {
      ArmCurrentPos = 0;
      break;
    } else {
      if (Direction == 1) {
        ArmCurrentPos ++;
      } else {
        ArmCurrentPos --;
      }
    }
  }
  if (! Hold) digitalWrite(STEPPER_ENABLE_2,LOW);
}
//------------------------------------------------------------------------------------------------
void SwitchJars(byte Direction) { // Rotates the turntable/rotor 45 degrees forward or backward
  digitalWrite(STEPPER_DIRECTION,Direction);
  digitalWrite(STEPPER_ENABLE_1,HIGH);
  delay(10);
  for (int x = 1; x <= JarDistance; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(RotorPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(RotorPulse);
  }
  digitalWrite(STEPPER_ENABLE_1,LOW);
}
//-----------------------------------------------------------------------------------------------
void JarAdvance(byte Direction) { // Lift the arm, switch jars, lower the arm
  ScreenUpdate();
  if (Serial) {
    if (Direction == 1) {
      Serial.println("Mode 1 Jar Advance +");
    } else {
      Serial.println("Mode 1 Jar Advance -");
    }
  }
  PopoverMessage("New Jar Selected");
  if (Serial) Serial.println("Raising Float Arm");
  SetArmPos(ArmUpperPos);
  if (Serial) Serial.println("Advancing Jar Position");
  SwitchJars(Direction);
  if (Serial) Serial.println("Lowering Float Arm");
  SetArmPos(ArmLowerPos);
}
//-----------------------------------------------------------------------------------------------
void BumpRotor(byte Direction, int Steps, bool Hold) {
  digitalWrite(STEPPER_DIRECTION,Direction);
  if (! Hold) {
    digitalWrite(STEPPER_ENABLE_1,HIGH);
    delay(10);
  }
  for (int x = 1; x <= Steps; x ++) {
    digitalWrite(STEPPER_PULSE,HIGH);
    delayMicroseconds(RotorPulse);
    digitalWrite(STEPPER_PULSE,LOW);
    delayMicroseconds(RotorPulse);
  }
  if (! Hold) digitalWrite(STEPPER_ENABLE_1,LOW);
}
//------------------------------------------------------------------------------------------------
void DrawButton(byte WhichOne) { // Draws and highlights the specified button on the screen
  canvas->setFont(&FreeSans9pt7b);
  canvas->setTextColor(BTNTEXT);
  if (WhichOne == 0) {
    canvas->fillRoundRect(PrevJarX1,PrevJarY1,PrevJarX2 - PrevJarX1,PrevJarY2 - PrevJarY1,5,JARBTN);
    canvas->setCursor(PrevJarX1 + 28,PrevJarY1 + 45);
    canvas->print("Previous Jar");
    if (ActiveButton == 0) canvas->drawRoundRect(PrevJarX1,PrevJarY1,PrevJarX2 - PrevJarX1,PrevJarY2 - PrevJarY1,5,HILITE);
  } else if (WhichOne == 1) {
    canvas->fillRoundRect(NextJarX1,NextJarY1,NextJarX2 - NextJarX1,NextJarY2 - NextJarY1,5,JARBTN);
    canvas->setCursor(NextJarX1 + 45,NextJarY1 + 45);
    canvas->print("Next Jar");
    if (ActiveButton == 1) canvas->drawRoundRect(NextJarX1,NextJarY1,NextJarX2 - NextJarX1,NextJarY2 - NextJarY1,5,HILITE);
  } else if (WhichOne == 2) {
    canvas->fillRoundRect(RotConfX1,RotConfY1,RotConfX2 - RotConfX1,RotConfY2 - RotConfY1,5,MODEBTN);
    canvas->setCursor(RotConfX1 + 40,RotConfY1 + 33);
    canvas->print("Configure");
    canvas->setCursor(RotConfX1 + 41,RotConfY1 + 57);
    canvas->print("Turntable");
    if (ActiveButton == 2) canvas->drawRoundRect(RotConfX1,RotConfY1,RotConfX2 - RotConfX1,RotConfY2 - RotConfY1,5,HILITE);
  } else if (WhichOne == 3) {
    canvas->fillRoundRect(ArmConfX1,ArmConfY1,ArmConfX2 - ArmConfX1,ArmConfY2 - ArmConfY1,5,MODEBTN);
    canvas->setCursor(ArmConfX1 + 41,ArmConfY1 + 33);
    canvas->print("Configure");
    canvas->setCursor(ArmConfX1 + 41,ArmConfY1 + 57);
    canvas->print("Float Arm");
    if (ActiveButton == 3) canvas->drawRoundRect(ArmConfX1,ArmConfY1,ArmConfX2 - ArmConfX1,ArmConfY2 - RotConfY1,5,HILITE);
  } else if (WhichOne == 4) {
    canvas->fillRoundRect(Conf1_X1,Conf1_Y1,Conf1_X2 - Conf1_X1,Conf1_Y2 - Conf1_Y1,5,CONFBTN);
    canvas->setCursor(Conf1_X1 + 34,Conf1_Y1 + 33);
    canvas->print("Zero");
    canvas->setCursor(Conf1_X1 + 31,Conf1_Y1 + 57);
    canvas->print("Rotor");
    if (ActiveButton == 4) canvas->drawRoundRect(Conf1_X1,Conf1_Y1,Conf1_X2 - Conf1_X1,Conf1_Y2 - Conf1_Y1,5,HILITE);
  } else if (WhichOne == 5) {
    canvas->fillRoundRect(Conf2_X1,Conf2_Y1,Conf2_X2 - Conf2_X1,Conf2_Y2 - Conf2_Y1,5,CONFBTN);
    canvas->setCursor(Conf2_X1 + 17,Conf2_Y1 + 33);
    canvas->print("Measure");
    canvas->setCursor(Conf2_X1 + 31,Conf2_Y1 + 57);
    canvas->print("Rotor");
    if (ActiveButton == 5) canvas->drawRoundRect(Conf2_X1,Conf2_Y1,Conf2_X2 - Conf2_X1,Conf2_Y2 - Conf2_Y1,5,HILITE);
  } else if (WhichOne == 6) {
    canvas->fillRoundRect(Conf3_X1,Conf3_Y1,Conf3_X2 - Conf3_X1,Conf3_Y2 - Conf3_Y1,5,TESTBTN);
    canvas->setCursor(Conf3_X1 + 32,Conf3_Y1 + 33);
    canvas->print("Test");
    canvas->setCursor(Conf3_X1 + 19,Conf3_Y1 + 57);
    canvas->print("Settings");
    if (ActiveButton == 6) canvas->drawRoundRect(Conf3_X1,Conf3_Y1,Conf3_X2 - Conf3_X1,Conf3_Y2 - Conf3_Y1,5,HILITE);
  } else if (WhichOne == 7) {
    canvas->fillRoundRect(Conf1_X1,Conf1_Y1,Conf1_X2 - Conf1_X1,Conf1_Y2 - Conf1_Y1,5,CONFBTN);
    canvas->setCursor(Conf1_X1 + 40,Conf1_Y1 + 33);
    canvas->print("Set");
    canvas->setCursor(Conf1_X1 + 31,Conf1_Y1 + 57);
    canvas->print("Lower");
    if (ActiveButton == 7) canvas->drawRoundRect(Conf1_X1,Conf1_Y1,Conf1_X2 - Conf1_X1,Conf1_Y2 - Conf1_Y1,5,HILITE);
  } else if (WhichOne == 8) {
    canvas->fillRoundRect(Conf2_X1,Conf2_Y1,Conf2_X2 - Conf2_X1,Conf2_Y2 - Conf2_Y1,5,CONFBTN);
    canvas->setCursor(Conf2_X1 + 40,Conf2_Y1 + 33);
    canvas->print("Set");
    canvas->setCursor(Conf2_X1 + 31,Conf2_Y1 + 57);
    canvas->print("Upper");
    if (ActiveButton == 8) canvas->drawRoundRect(Conf2_X1,Conf2_Y1,Conf2_X2 - Conf2_X1,Conf2_Y2 - Conf2_Y1,5,HILITE);
  } else if (WhichOne == 9) {
    canvas->fillRoundRect(Conf3_X1,Conf3_Y1,Conf3_X2 - Conf3_X1,Conf3_Y2 - Conf3_Y1,5,TESTBTN);
    canvas->setCursor(Conf3_X1 + 32,Conf3_Y1 + 33);
    canvas->print("Test");
    canvas->setCursor(Conf3_X1 + 19,Conf3_Y1 + 57);
    canvas->print("Settings");
    if (ActiveButton == 9) canvas->drawRoundRect(Conf3_X1,Conf3_Y1,Conf3_X2 - Conf3_X1,Conf3_Y2 - Conf3_Y1,5,HILITE);
  }
}
//------------------------------------------------------------------------------------------------
void PopoverMessage(String Msg) { // Display popover message to the user
  int16_t nX = 0, nY = 0, TextX;
  uint16_t nWidth = 0, nHeight = 0;

  canvas->setFont(&FreeSans9pt7b);
  canvas->setTextColor(BLACK);
  canvas->getTextBounds(Msg,0,0,&nX,&nY,&nWidth,&nHeight);
  TextX = round(nWidth / 2);
  canvas->fillRoundRect(160 - TextX - 12,60,nWidth + 26,40,5,WHITE);
  canvas->drawRoundRect(160 - TextX - 12,60,nWidth + 26,40,5,BLACK);
  canvas->setCursor(160 - TextX,85);
  canvas->print(Msg);
  canvas->flush();
}
//------------------------------------------------------------------------------------------------
void ScreenUpdate() { // Plot the off-screen buffer and then pop it to the touch screen display
  if (CurrentMode == 1) {
    if (Serial) {
      // Debugging information
      Serial.println();
      Serial.print("RotorSize: "); Serial.println(RotorSize);
      Serial.print("JarDistance: "); Serial.println(JarDistance);
      Serial.print("ArmUpperPos: "); Serial.println(ArmUpperPos);
      Serial.print("ArmLowerPos: "); Serial.println(ArmLowerPos);
      Serial.print("ArmCurrentPos: "); Serial.println(ArmCurrentPos);
      Serial.println();
    }

    canvas->fillScreen(BLACK);
    DrawButton(0);
    DrawButton(1);
    DrawButton(2);
    DrawButton(3);
  } else {
    canvas->fillScreen(DARKGREY);
    canvas->setFont(&FreeSans9pt7b);
    canvas->setTextColor(BTNTEXT);
    if (CurrentMode == 2) {
      canvas->setCursor(10,20);
      canvas->print("Zero Rotor: use +/- to set start point");
      canvas->setCursor(10,45);
      canvas->print("Measure: rotate turntable + 360 deg");
      canvas->setCursor(10,70);
      canvas->print("Test: verify 45 degree jar divisions");
      DrawButton(4);
      DrawButton(5);
      DrawButton(6);
    } else {
      canvas->setCursor(10,20);
      canvas->print("Set Lower: use +/- to set arm down");
      canvas->setCursor(10,45);
      canvas->print("Set Upper: use +/- to set arm raised");
      canvas->setCursor(10,70);
      canvas->print("Test: verify the arm up/down points");
      DrawButton(7);
      DrawButton(8);
      DrawButton(9);
    }
  }
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
  // If Xpos is a negative number, the user has pressed the home button
  if ((CurrentMode > 1) && (Xpos < 0)) {
    CurrentMode = 1;
    ActiveButton = 1; 
    if (ArmCurrentPos != ArmLowerPos) {
      PopoverMessage("Lowering Float Arm");
      SetArmPos(ArmLowerPos);
    }
    SetMemory();
    ScreenUpdate();
    return;
  }
  if (CurrentMode == 1) {
    if (RegionPressed(Xpos,Ypos,PrevJarX1,PrevJarY1,PrevJarX2,PrevJarY2)) {
      ActiveButton = 0;
      JarAdvance(0);
    } else if (RegionPressed(Xpos,Ypos,NextJarX1,NextJarY1,NextJarX2,NextJarY2)) {
      ActiveButton = 1;
      JarAdvance(1);
    } else if (RegionPressed(Xpos,Ypos,RotConfX1,RotConfY1,RotConfX2,RotConfY2)) {
      ActiveButton = 2;
      ScreenUpdate();
      PopoverMessage("Raising Float Arm");
      SetArmPos(ArmUpperPos);
      ActiveButton = 6;
      CurrentMode = 2;
    } else if (RegionPressed(Xpos,Ypos,ArmConfX1,ArmConfY1,ArmConfX2,ArmConfY2)) {
      ActiveButton = 9;
      CurrentMode = 3;
    }
  } else if (CurrentMode == 2) {
    if (RegionPressed(Xpos,Ypos,Conf1_X1,Conf1_Y1,Conf1_X2,Conf1_Y2)) {
      ActiveButton = 4;
      RotorSize = 0;
      ScreenUpdate();
      PopoverMessage("Turntable Size Zeroed");
      delay(2000);
    } else if (RegionPressed(Xpos,Ypos,Conf2_X1,Conf2_Y1,Conf2_X2,Conf2_Y2)) {
      ActiveButton = 5;
    } else if (RegionPressed(Xpos,Ypos,Conf3_X1,Conf3_Y1,Conf3_X2,Conf3_Y2)) {
      ActiveButton = 6;
      ScreenUpdate();
      PopoverMessage("Advancing Jar Position");
      if (Serial) Serial.println("Mode 3 Jar Advance +");
      SwitchJars(1);
    }
  } else if (CurrentMode == 3) {
    if (RegionPressed(Xpos,Ypos,Conf1_X1,Conf1_Y1,Conf1_X2,Conf1_Y2)) {
      ActiveButton = 7;
      ScreenUpdate();
      PopoverMessage("Move To Lower Position");
      SetArmPos(ArmLowerPos);
    } else if (RegionPressed(Xpos,Ypos,Conf2_X1,Conf2_Y1,Conf2_X2,Conf2_Y2)) {
      ActiveButton = 8;
      ScreenUpdate();
      PopoverMessage("Move To Upper Position");
      SetArmPos(ArmUpperPos);
    } else if (RegionPressed(Xpos,Ypos,Conf3_X1,Conf3_Y1,Conf3_X2,Conf3_Y2)) {
      ActiveButton = 9;
      if (ArmCurrentPos == ArmLowerPos) {
        ScreenUpdate();
        PopoverMessage("Upper Position Test");
        SetArmPos(ArmUpperPos);
      } else {
        ScreenUpdate();
        PopoverMessage("Lower Position Test");
        SetArmPos(ArmLowerPos);
      }
    }
  }
  ScreenUpdate();
}
//------------------------------------------------------------------------------------------------
void IncValue(bool Hold) { // Increment the value associated with the current mode and active screen button
  if (CurrentMode == 1) {
    if (Serial) Serial.println("Mode 1 Rotor Bump +");
    BumpRotor(1,40,Hold);
  } else if (CurrentMode == 2) {
    if (Serial) Serial.println("Mode 2 Rotor Bump +");
    if (ActiveButton < 6) BumpRotor(1,40,Hold);
    if (ActiveButton == 5) {
      RotorSize += 40;
    }
  } else if (CurrentMode == 3) {
    if (ActiveButton < 9) {
      if (Serial) Serial.println("Mode 3 Arm Bump +");
      BumpArm(1,ArmSteps,Hold);
    }
    if (ActiveButton == 7) {
      ArmLowerPos = ArmCurrentPos;
    } else if (ActiveButton == 8) {
      ArmUpperPos = ArmCurrentPos;
    }
  }
}
//-----------------------------------------------------------------------------------------------
void DecValue(bool Hold) { // Decrement the value associated with the current mode and active screen button
  if (CurrentMode == 1) {
    if (Serial) Serial.println("Mode 1 Rotor Bump -");
    BumpRotor(0,40,Hold);
  } else if (CurrentMode == 2) {
    if (Serial) Serial.println("Mode 2 Rotor Bump -");
    if (ActiveButton < 6) BumpRotor(0,40,Hold);
    if (ActiveButton == 5) {
      RotorSize -= 40;
      if (RotorSize < 0) RotorSize = 0;
    }
  } else if (CurrentMode == 3) {
    if (ActiveButton < 9) {
      if (Serial) Serial.println("Mode 3 Arm Bump -");
      BumpArm(0,ArmSteps,Hold);
    }
    if (ActiveButton == 7) {
      ArmLowerPos = ArmCurrentPos;
    } else if (ActiveButton == 8) {
      ArmUpperPos = ArmCurrentPos;
    }
  }
}
//-----------------------------------------------------------------------------------------------
void ProcessButton(byte WhichOne) { // Handle increment/decrement button inputs
  int HoldCounter = 0;

  if (WhichOne == 1) {
    // Increment active screen button value by 1
    IncValue(false);
    while (digitalRead(INC_BTN) == 0) {
      delay(10);
      HoldCounter ++;
      if ((HoldCounter == 100) && (CurrentMode > 1)) { // User is intentionally holding the + button
        if (CurrentMode == 2) {
          digitalWrite(STEPPER_ENABLE_1,HIGH);
        } else {
          digitalWrite(STEPPER_ENABLE_2,HIGH);
        }
        delay(10);
        while (digitalRead(INC_BTN) == 0) IncValue(true);
        if (CurrentMode == 2) {
          digitalWrite(STEPPER_ENABLE_1,LOW);
        } else {
          digitalWrite(STEPPER_ENABLE_2,LOW);
        }
      }
    }
  } else {
    // Decrement active screen button value by 1
    DecValue(false);
    while (digitalRead(DEC_BTN) == 0) {
      delay(10);
      HoldCounter ++;
      if ((HoldCounter == 100) && (CurrentMode > 1)) { // User is intentionally holding the - button
        if (CurrentMode == 2) {
          digitalWrite(STEPPER_ENABLE_1,HIGH);
        } else {
          digitalWrite(STEPPER_ENABLE_2,HIGH);
        }
        delay(10);
        while (digitalRead(DEC_BTN) == 0) DecValue(true);
        if (CurrentMode == 2) {
          digitalWrite(STEPPER_ENABLE_2,LOW);
        } else {
          digitalWrite(STEPPER_ENABLE_2,LOW);
        }
      }
    }
  }
  if (Serial) {
    // Debugging information
    if (ActiveButton == 5) {
      Serial.print("RotorSize: "); Serial.println(RotorSize);
    } else if (ActiveButton == 7) {
      Serial.print("ArmLowerPos: "); Serial.println(ArmLowerPos);
    } else if (ActiveButton == 8) {
      Serial.print("ArmUpperPos: "); Serial.println(ArmUpperPos);
    }
  }
}
//------------------------------------------------------------------------------------------------
void loop() {
  long CurrentTime = millis();
  if (CurrentTime > 4200000000) {
    ESP.restart();
  }

  // Check for touch-screen presses and handle as necessary
  if (GotInterrupt) {
    if (Touch.read()) {
      int Xpos,Ypos;
      TP_Point Point = Touch.getPoint(0);
      Xpos = 320 - Point.y; // Touch.setRotation() doesn't work for some reason
      Ypos = Point.x;       // "                                              "
      if (CurrentTime - LoopCounter >= 1000) {
        ProcessTouch(Xpos,Ypos);
        LoopCounter = CurrentTime;
      }
    }
    GotInterrupt = false;
  }
  // Check for Value+ keypresses and handle as necessary
  if (digitalRead(INC_BTN) == 0) ProcessButton(1);
  // Check for Value- keypresses and handle as necessary
  if (digitalRead(DEC_BTN) == 0) ProcessButton(0);
  // If not in configuation mode, check the float switch and advance to the next jar if necessary
  if ((CurrentMode == 1) && (digitalRead(FLOAT_SWITCH) == LOW)) {
    ActiveButton = 1;
    JarAdvance(1);
    ScreenUpdate();
  }
  // Give the CPU a break between loops
  delay(10);
}
//------------------------------------------------------------------------------------------------
/*
// Create & run a new sketch with the following code to fully erase the flash memory of an ESP32

#include <nvs_flash.h>

void setup() {
  nvs_flash_erase(); // erase the NVS partition and...
  nvs_flash_init();  // initialize the NVS partition.
  while(true);
}

void loop() {

}
*/
//------------------------------------------------------------------------------------------------
