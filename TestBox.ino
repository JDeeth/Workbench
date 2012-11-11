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

#include "EncoderLCD.h"
#include "OmniTune.h"
#include "DialController.h"
#include "GearLights.h"

void setup() {
  setupEncoderLCD();
  setupGearLights();
}

void loop() {
  FlightSim.update();

  loopEncoderLCD();
  loopGearLights();
}
