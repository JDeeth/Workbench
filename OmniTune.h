#ifndef OMNITUNE_H
#define OMNITUNE_H

#define DataRefIdent PROGMEM const char



///////////////////
// Output hardware
//
enum LCD_PINS {
  RS = 27,
  RW = 0, EN, D4, D5, D6, D7,
  BACKLIGHT = 24
};

LiquidCrystalFast lcd(RS, RW, EN, D4, D5, D6, D7);

void setupOmnituneOutput() {
  pinMode (BACKLIGHT, OUTPUT);
  analogWrite (BACKLIGHT, 128);

  lcd.begin (16, 2);
}



///////////////////
// Input hardware
//
enum INPUT_PINS {
  PIN_LEFT_ENC_A = 7,
  PIN_LEFT_ENC_B = 8,
  PIN_LEFT_IN = 20,

  PIN_RIGHT_ENC_A = 10,
  PIN_RIGHT_ENC_B = 9,
  PIN_RIGHT_IN = 11
};

Bounce leftIn = Bounce (PIN_LEFT_IN, 5);
Bounce rightIn = Bounce (PIN_RIGHT_IN, 5);

Encoder leftEnc(PIN_LEFT_ENC_A, PIN_LEFT_ENC_B);
Encoder rightEnc(PIN_RIGHT_ENC_A, PIN_RIGHT_ENC_B);

const short ENC_CHANGE_PER_DETENT = 4; // may be 1, 2, or 4 depending on model
short leftEncPrev;  // position of encoders when last inspected
short rightEncPrev;

void setupOmnituneInput () {
  pinMode (PIN_LEFT_ENC_A, INPUT_PULLUP);
  pinMode (PIN_LEFT_ENC_B, INPUT_PULLUP);
  pinMode (PIN_LEFT_IN, INPUT_PULLUP);
  pinMode (PIN_RIGHT_ENC_A, INPUT_PULLUP);
  pinMode (PIN_RIGHT_ENC_B, INPUT_PULLUP);
  pinMode (PIN_RIGHT_IN, INPUT_PULLUP);
}



////////////////////////////////////////////////////////////////////////
// Radio tuner objects
//
enum TUNER_MODES {
  NAV1, NAV2, COM1, COM2, ADF1, ADF2, XP_MODE, XP_CODE,
  TUNER_MODE_COUNT
};

// transponder modes
enum XP_MODES {
  XP_OFF, XP_STBY, XP_ON, XP_ALT, XP_TEST,
  XP_MODE_COUNT
};

DataRefIdent tunerDataRefIdent[][64] = {
  {"sim/cockpit2/radios/actuators/nav1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/nav2_frequency_hz"},
  {"sim/cockpit2/radios/actuators/com1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/com2_frequency_hz"},
  {"sim/cockpit2/radios/actuators/adf1_frequency_hz"},
  {"sim/cockpit2/radios/actuators/adf2_frequency_hz"},
  {"sim/cockpit2/radios/actuators/transponder_code"},
  {"sim/cockpit/radios/transponder_mode"}
};

FlightSimInteger tunerDataRef[TUNER_MODE_COUNT];

void setupTunerDataref() {
  for (int i = 0; i < TUNER_MODE_COUNT; ++i) {
    tunerDataRef[i].assign((const _XpRefStr_ *) &tunerDataRefIdent[i][0]);
  }
}

int tunerMode = NAV1; // indicates selected channel

void tunerDisplayUpdate();
void tunerInputUpdate(const int &modeDelta,
                      const int &leftDelta,
                      const int &rightDelta);

////////////////////////////////////////////////////////////////////////
// Knob-adjusting objects
//
class Knob {
public:
  FlightSimFloat dr;
  float lowLimit;
  float highLimit;
  float scalar;
  int coarseToFineRatio;
  bool lapNotCrop;
  void set(float low, float high, float sc, float ratio, bool lap = false) {
    lowLimit = low;
    highLimit = high;
    scalar = sc;
    coarseToFineRatio = ratio;
    lapNotCrop = lap;
  }
  Knob(float low, float high, float sc, float ratio, bool lap = false) {
    set(low, high, sc, ratio, lap);
  }

  void addDelta(int fineDelta, int coarseDelta = 0) {
    float delta = fineDelta + coarseDelta * coarseToFineRatio;
    delta *= scalar;
    delta += dr;
    if(lapNotCrop) {
      while (delta >= highLimit)
        delta -= (highLimit - lowLimit);
      while (delta < lowLimit)
        delta += (highLimit - lowLimit);
    } else {
      if (delta > highLimit)
        delta = highLimit;
      if (delta < lowLimit)
        delta = lowLimit;
    }
    dr = delta;
  }
};

enum KNOB_MODES {
  HEADING_P1, NAV1_OBS,// NAV2_OBS,
  KNOB_MODE_COUNT
};

Knob headingP1(0.0, 360.0, 0.25, 20, true);
Knob nav1Obs(0.0, 360.0, 0.25, 20, true);

int knobMode = HEADING_P1;

void setupKnobDataref() {
  headingP1.dr = XPlaneRef("sim/cockpit2/autopilot/heading_dial_deg_mag_pilot");
  nav1Obs.dr = XPlaneRef("sim/cockpit2/radios/actuators/nav1_obs_deg_mag_pilot");
}

void knobDisplayUpdate();
void knobInputUpdate(const int &modeDelta,
                     const int &leftDelta,
                     const int &rightDelta);

////////////////////////////////////////////////////////////////////////
// Local objects
//
enum META_MODE {
  TUNER, KNOB,
  META_MODE_COUNT
};

int metaMode = 0;

elapsedMicros dispTimer = 0; //to avoid updating display too frequently
unsigned int dispPeriod = 35525; //magic number determined by experimentation



void setupOmniTune() {
  setupOmnituneInput();
  setupOmnituneOutput();
  setupTunerDataref();
  setupKnobDataref();
}



void loopOmniTune() {

  bool showDisplay = false;

  if (dispTimer > dispPeriod) {
    dispTimer = 0;
    if (FlightSim.isEnabled()) {
      showDisplay = true;
      analogWrite(BACKLIGHT, 128);
    } else {
      lcd.clear();
      lcd.print("OmniStuff");
      analogWrite(BACKLIGHT, 0);
    }
  }




  leftIn.update();
  rightIn.update();


  if ((leftIn.read() == LOW) && rightIn.fallingEdge()) {
    ++metaMode;
    if (metaMode >= META_MODE_COUNT)
      metaMode = 0;
  }

  if ((rightIn.read() == LOW) && leftIn.fallingEdge()) {
    --metaMode;
    if (metaMode < 0)
      metaMode = META_MODE_COUNT - 1;
  }

  short modeDelta = 0;
  if (leftIn.risingEdge()) {
    modeDelta = -1;
  }
  if (rightIn.risingEdge()) {
    modeDelta = 1;
  }

  short leftDelta = (leftEnc.read() - leftEncPrev) / ENC_CHANGE_PER_DETENT;
  if (leftDelta) {
    leftEncPrev = 0;
    leftEnc.write(0);
  }

  short rightDelta = (rightEnc.read() - rightEncPrev) / ENC_CHANGE_PER_DETENT;
  if (rightDelta) {
    rightEncPrev = 0;
    rightEnc.write(0);
  }

  switch(metaMode) {
    case TUNER:
      tunerInputUpdate(modeDelta, leftDelta, rightDelta);
      if(showDisplay)
        tunerDisplayUpdate();
      break;

    case KNOB:
      knobInputUpdate(modeDelta, leftDelta, rightDelta);
      if(showDisplay)
        knobDisplayUpdate();
      break;
  }
}




void knobInputUpdate(const int &modeDelta,
                     const int &leftDelta,
                     const int &rightDelta) {
  if (modeDelta) {
    knobMode += modeDelta;
    if (knobMode >= KNOB_MODE_COUNT)
      knobMode -= KNOB_MODE_COUNT;
    if(knobMode < 0)
      knobMode += KNOB_MODE_COUNT;
  }

  switch(knobMode) {
    case HEADING_P1:
      headingP1.addDelta(rightDelta, leftDelta);
      break;
    case NAV1_OBS:
      nav1Obs.addDelta(rightDelta, leftDelta);
      break;
  }
}





void knobDisplayUpdate() {
  lcd.clear();
  lcd.setCursor(0, 0);

  switch (knobMode) {
    case HEADING_P1:
      lcd.print("Heading P1");
      lcd.setCursor(0,1);
      lcd.print(headingP1.dr);
      break;
    case NAV1_OBS:
      lcd.print("NAV1 OBS");
      lcd.setCursor(0,1);
      lcd.print(nav1Obs.dr);
      break;
    default:
      lcd.print("Undefined");
  }
}






////////////////////////////////////////////////////////////////////////
// Tuner-mode input update
//
void tunerInputUpdate(const int &modeDelta,
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
void tunerDisplayUpdate() {

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
