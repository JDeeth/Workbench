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



void radioInputUpdate() {

  if(leftIn.fallingEdge()) {     // when left encoder pressed
    --channel;                   // select previous channel
    while (channel < 0)          // if there's no previous channel,
      channel += DATAREF_COUNT;  // go to the last channel.
  }

  if(rightIn.fallingEdge()) {    // when right encoder pressed
    ++channel;                   // select next channel
    while (channel >= DATAREF_COUNT)
      channel -= DATAREF_COUNT;  // when we go past the last, go back to the first
  }

  short leftEncDiff = (leftEnc.read() - leftEncPrev) / ENC_CHANGE_PER_DETENT;
  short rightEncDiff = (rightEnc.read() - rightEncPrev) / ENC_CHANGE_PER_DETENT;

  // reset encoders if they've been turned
  if (leftEncDiff) {
    leftEnc.write(0); //substitute leftEnc.write(0) when real encoders are used
    leftEncPrev = 0;
  }

  if (rightEncDiff) {
    rightEnc.write(0);
    rightEncPrev = 0;
  }

  // tune frequencies if either encoder has been turned
  if (leftEncDiff || rightEncDiff) {

    int freq = dataref[channel];

    switch (channel) { // tune selected channel

      case NAV1:
      case NAV2:
        if (leftEncDiff) {
          // TUNE HI. Increment in megaherts, which is freq * 100
          freq += leftEncDiff * 100;
          // lap to 108-118MHz range
          while (freq < 10800) freq += 1000;
          while (freq >= 11800) freq -= 1000;
        }
        if (rightEncDiff) { //TUNE LO. Increment in decaKHz
          //remove MHz element from freq, leaving decaKHz element
          int mhz = freq / 100;
          freq -= mhz * 100;
          //increment freq in 50KHz (0.05MHz) steps
          freq += rightEncDiff * 5;
          //crop freq to prevent TUNE LO mode from changing TUNE HI digits
          while (freq < 0) freq += 100;
          while (freq >= 100) freq -= 100;
          //reinstate MHz element
          freq += mhz * 100;
        }
        break;

      case COM1:
      case COM2:
        if (leftEncDiff) {//TUNE HI.
          freq += leftEncDiff * 100;
          // lap to 118.00 - 136.00
          while (freq <  11800) freq += 1800;
          while (freq >= 13600) freq -= 1800;
        }
        if (rightEncDiff) { //TUNE LO
          //remove megahertz from freq (digits left of decimal point)
          int mhz = freq / 100;
          freq -= mhz * 100;
          //COM radios change in 25KHz steps, but X-Plane crops down to 10KHz resolution (ie 120.125 becomes 12012, losing the final 5)
          //floating point variable used to reinstate missing 5 KHz when necessary
          float ffreq = freq;
          if ((freq - (10*(freq/10))) == 2 		// if dataref value ends in 2
              || (freq - (10*(freq/10))) == 7) { 	// or dataref value ends in 7
            ffreq += 0.5; 						// then reinstate missing 5KHz
          }
          //increment in 25KHz steps
          ffreq += rightEncDiff * 2.5;
          //convert back to integers (c++ drops the trailing .5 if necessary)
          freq = ffreq;
          //keep freq between 0 and <100 so this operation doesn't affect digits left of decimal point
          while (freq < 0) freq += 100;
          while (freq >= 100) freq -= 100;
          //reinstate the megahertz digits
          freq += mhz * 100;
        }
        break;

      case ADF1:
      case ADF2:
        if (leftEncDiff) { // alter first two digits
          freq += leftEncDiff * 10;
        }
        if (rightEncDiff) { // alter third digit
          freq += rightEncDiff;
        }
        if (freq < 190) freq = 190;
        if (freq >= 600) freq = 600;
        break;

      case XP_CODE:
        // if transponder is active, put it into standby before altering code
        if (dataref[XP_MODE] >= XP_ON)
          dataref[XP_MODE] = XP_STBY;
        // split transponder code into digits
        {
          short dgt[4]; // digit
          dgt[0] = freq/1000;
          dgt[1] = (freq - 1000 *(freq/1000) ) / 100;
          dgt[2] = (freq - 100 * (freq/100) ) / 10;
          dgt[3] = freq - 10 * (freq/10);

          if (channel == XP_CODE) {

            dgt[1] += leftEncDiff;

            if (dgt[1] < 0) {
              dgt[1] = 7;
              --dgt[0];
              if (dgt[0] < 0)
                dgt[0] = 7;
            }

            if (dgt[1] > 7) {
              dgt[1] = 0;
              ++dgt[0];
              if (dgt[0] > 7)
                dgt[0] = 0;
            }

            dgt[3] += rightEncDiff;

            if (dgt[3] < 0) {
              dgt[3] = 7;
              --dgt[2];
              if (dgt[2] < 0)
                dgt[2] = 7;
            }

            if (dgt[3] > 7) {
              dgt[3] = 0;
              ++dgt[2];
              if (dgt[2] > 7)
                dgt[2] = 0;
            }

          } // alter codeDigit

          // recombine codeDigit
          freq = dgt[0] * 1000 + dgt[1] * 100 + dgt[2] * 10 + dgt[3];

        }
        break;

      case XP_MODE:
        freq += leftEncDiff;
        freq += rightEncDiff;
        if (freq < 0)
          freq = 0;
        if (freq >= XP_MODE_COUNT)
          freq = XP_MODE_COUNT - 1;
        break;

    } // switch, tune selected channel

    dataref[channel] = freq;
  }

}

///////////////////////////////////////////////////////////////////////////////
// Display Update
//
// Reads datarefs and draws selected channels onto display
//
void radioDisplayUpdate() {

  /////////////////
  // Set up display
  //
  lcd.clear();
  lcd.setCursor(0, 0);

  /////////////////
  // Print selected channels
  //
  float tmp; // for putting decimal point into integer dataref frequencies

  switch (channel) { // print selected channels
    case NAV1:
    case NAV2:
    case XP_CODE:
    case XP_MODE:
      lcd.print("NAV1");
      if (channel == NAV1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      tmp = dataref[NAV1] / 100.0;
      lcd.print(tmp);

      lcd.setCursor(0, 1);
      lcd.print("NAV2");
      if (channel == NAV2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      tmp = dataref[NAV2] / 100.0;
      lcd.print(tmp);
      break;

    case COM1:
    case COM2:
      lcd.print("COM1");
      if (channel == COM1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      tmp = dataref[COM1] / 100.0;
      lcd.print(tmp);

      lcd.setCursor(0, 1);
      lcd.print("COM2");
      if (channel == COM2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      tmp = dataref[COM2] / 100.0;
      lcd.print(tmp);
      break;

    case ADF1:
    case ADF2:
      lcd.print("ADF1");
      if (channel == ADF1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      lcd.print(dataref[ADF1]);

      lcd.setCursor(0, 1);
      lcd.print("ADF2");
      if (channel == ADF2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      lcd.print(dataref[ADF2]);
      break;

  } // switch, print selected channels

  /////////////////
  // Print transponder code
  //
  lcd.setCursor(12, 1);
  // pad out to right if code has less than four digits
  if(dataref[XP_CODE] < 1000)
    lcd.print("0");
  if(dataref[XP_CODE] < 100)
    lcd.print("0");
  if(dataref[XP_CODE] < 10)
    lcd.print("0");
  lcd.print(dataref[XP_CODE]);

  /////////////////
  // Print transponder mode
  //
  lcd.setCursor(12, 0);
  switch (dataref[XP_MODE]) {

    case XP_OFF:
      lcd.print(" OFF");
      break;

    case XP_STBY:
      lcd.print("STBY");
      break;

    case XP_ON:
      lcd.print(" ON ");
      break;

    case XP_ALT:
      lcd.print(" ALT");
      break;

    case XP_TEST:
    default:
      lcd.print("TEST");
      break;

  } // switch, display transponder mode and code

  /////////////////
  // Transponder selection indication
  //
  switch(channel) {
    case XP_CODE:
      lcd.setCursor(11, 1);
      lcd.print(">");
      break;
    case XP_MODE:
      lcd.setCursor(11, 0);
      lcd.print(">");
      break;
  } // switch, transponder selection indicator

} // radioDisplayUpdate

