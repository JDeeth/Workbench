#ifndef OMNITUNE_H
#define OMNITUNE_H


// so we can put idents at global scope, instead of in setup()
#define DataRefIdent PROGMEM const char



////////////////////
// OmniTune modes
//
enum TUNER_MODES {
  NAV1, NAV2, COM1, COM2, ADF1, ADF2, XP_MODE, XP_CODE,
  TUNER_MODE_COUNT
};
int tunerMode = NAV1; // indicates selected channel


// transponder modes
enum XP_MODES {
  XP_OFF, XP_STBY, XP_ON, XP_ALT, XP_TEST,
  XP_MODE_COUNT
};


// Datarefs used by OmniTune
DataRefIdent tunerDataRefIdent[][64] = {
  {"sim/cockpit2/radios/actuators/nav1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/nav2_frequency_hz"},
  {"sim/cockpit2/radios/actuators/com1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/com2_frequency_hz"},
  {"sim/cockpit2/radios/actuators/adf1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/adf2_frequency_hz"},
  {"sim/cockpit/radios/transponder_mode"},
  {"sim/cockpit2/radios/actuators/transponder_code"}
};

FlightSimInteger tunerDataRef[TUNER_MODE_COUNT];

void setupOmniTuneDataref() {
  for (int i = 0; i < TUNER_MODE_COUNT; ++i) {
    tunerDataRef[i].assign((const _XpRefStr_ *) &tunerDataRefIdent[i][0]);
  }
}



////////////////////////////////////////////////////////////////////////
// Tuner-mode input update
//
void updateOmniTuneInput(const int &modeDelta,
                      const int &leftDelta,
                      const int &rightDelta) {

  if (modeDelta) {
    tunerMode += modeDelta;
    if (tunerMode >= TUNER_MODE_COUNT)
      tunerMode -= TUNER_MODE_COUNT;
    if (tunerMode < 0)
      tunerMode += TUNER_MODE_COUNT;
  }

  // tune frequencies if either encoder has been turned
  if (leftDelta || rightDelta) {

    int freq = tunerDataRef[tunerMode];

    switch (tunerMode) { // tune selected channel

      case NAV1:
      case NAV2:
        if (leftDelta) {
          // TUNE HI. Increment in megaherts, which is freq * 100
          freq += leftDelta * 100;
          // lap to 108-118MHz range
          while (freq < 10800) freq += 1000;
          while (freq >= 11800) freq -= 1000;
        }
        if (rightDelta) { //TUNE LO. Increment in decaKHz
          //remove MHz element from freq, leaving decaKHz element
          int mhz = freq / 100;
          freq -= mhz * 100;
          //increment freq in 50KHz (0.05MHz) steps
          freq += rightDelta * 5;
          //crop freq to prevent TUNE LO mode from changing TUNE HI digits
          while (freq < 0) freq += 100;
          while (freq >= 100) freq -= 100;
          //reinstate MHz element
          freq += mhz * 100;
        }
        break;

      case COM1:
      case COM2:
        if (leftDelta) {//TUNE HI.
          freq += leftDelta * 100;
          // lap to 118.00 - 136.00
          while (freq <  11800) freq += 1800;
          while (freq >= 13600) freq -= 1800;
        }
        if (rightDelta) { //TUNE LO
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
          ffreq += rightDelta * 2.5;
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
        if (leftDelta) { // alter first two digits
          freq += leftDelta * 10;
        }
        if (rightDelta) { // alter third digit
          freq += rightDelta;
        }
        if (freq < 190) freq = 190;
        if (freq >= 600) freq = 600;
        break;

      case XP_CODE:
        // if transponder is active, put it into standby before altering code
        if (tunerDataRef[XP_MODE] >= XP_ON)
          tunerDataRef[XP_MODE] = XP_STBY;
        // split transponder code into digits
        {
          short dgt[4]; // digit
          dgt[0] = freq/1000;
          dgt[1] = (freq - 1000 *(freq/1000) ) / 100;
          dgt[2] = (freq - 100 * (freq/100) ) / 10;
          dgt[3] = freq - 10 * (freq/10);

          if (tunerMode == XP_CODE) {

            dgt[1] += leftDelta;

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

            dgt[3] += rightDelta;

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
        freq += leftDelta;
        freq += rightDelta;
        if (freq < 0)
          freq = 0;
        if (freq >= XP_MODE_COUNT)
          freq = XP_MODE_COUNT - 1;
        break;

    } // switch, tune selected channel

    tunerDataRef[tunerMode] = freq;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Tuner-mode Display Update
//
// Reads datarefs and draws selected channels onto display
//
void updateOmniTuneDisplay() {

  /////////////////
  // Set up display
  //
  lcd.clear();
  lcd.setCursor(0, 0);

  /////////////////
  // Print selected channels
  //
  float tmp; // for putting decimal point into integer dataref frequencies

  switch (tunerMode) { // print selected channels
    case NAV1:
    case NAV2:
    case XP_CODE:
    case XP_MODE:
      lcd.print("NAV1");
      if (tunerMode == NAV1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      tmp = tunerDataRef[NAV1] / 100.0;
      lcd.print(tmp);

      lcd.setCursor(0, 1);
      lcd.print("NAV2");
      if (tunerMode == NAV2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      tmp = tunerDataRef[NAV2] / 100.0;
      lcd.print(tmp);
      break;

    case COM1:
    case COM2:
      lcd.print("COM1");
      if (tunerMode == COM1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      tmp = tunerDataRef[COM1] / 100.0;
      lcd.print(tmp);

      lcd.setCursor(0, 1);
      lcd.print("COM2");
      if (tunerMode == COM2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      tmp = tunerDataRef[COM2] / 100.0;
      lcd.print(tmp);
      break;

    case ADF1:
    case ADF2:
      lcd.print("ADF1");
      if (tunerMode == ADF1)
        lcd.print(">");
      lcd.setCursor(5, 0);
      lcd.print(tunerDataRef[ADF1]);

      lcd.setCursor(0, 1);
      lcd.print("ADF2");
      if (tunerMode == ADF2)
        lcd.print(">");
      lcd.setCursor(5, 1);
      lcd.print(tunerDataRef[ADF2]);
      break;

  } // switch, print selected channels

  /////////////////
  // Print transponder code
  //
  lcd.setCursor(12, 1);
  // pad out to right if code has less than four digits
  if(tunerDataRef[XP_CODE] < 1000)
    lcd.print("0");
  if(tunerDataRef[XP_CODE] < 100)
    lcd.print("0");
  if(tunerDataRef[XP_CODE] < 10)
    lcd.print("0");
  lcd.print(tunerDataRef[XP_CODE]);

  /////////////////
  // Print transponder mode
  //
  lcd.setCursor(12, 0);
  switch (tunerDataRef[XP_MODE]) {

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
  switch(tunerMode) {
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

#endif
