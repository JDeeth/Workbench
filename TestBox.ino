//#include "usb_api.h"

///////////////////////////////////////////////////////////////////////////////
//
// OmniTune
//
// Radio tuner firmware for X-Plane. Designed for a 16x2 display and two
// encoders with integral pushbuttons, or similar.
//
// This code is written for the PJRC Teensy board, v2.0 or higher, using the
// Arduino+Teensyduino framework. This instance of the code is completely
// independent of the PC, other than for power; it does not connect to X-Plane.
//
// I don't have any encoders available. This code is written for encoders,
// using tested code which worked well a few months ago, but I cannot test it
// myself at this time unfortunately. Nonetheless I'm pretty sure it will work
// without a problem.
//
// Copyright 2012 Jack Deeth
//
///////////////////////////////////////////////////////////////////////////////

#include <LiquidCrystalFast.h>
#include <Bounce.h>
#include <Encoder.h>

//#define DataRefIdent PROGMEM const char

#include "OmniTune.h"
#include "GearLights.h"



void setup() {
  setupInput();
  setupOutput();
  setupDataref();

  setupGearLights();

  pinMode(LED_BUILTIN, OUTPUT);
}



void loop() {
  FlightSim.update();

  loopGearLights();

  leftIn.update();
  rightIn.update();

  radioInputUpdate();

  if (dispTimer > dispPeriod) {
    dispTimer = 0;
    if(FlightSim.isEnabled())
      radioDisplayUpdate();
    else {
      lcd.print("SimElectronics. ");
      lcd.setCursor (0, 1);
      lcd.print(" WordPress.com  ");
    }
  }
} // loop



