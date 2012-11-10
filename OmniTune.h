#ifndef OMNITUNE_H
#define OMNITUNE_H

///////////////////
// Output hardware
//
enum LCD_PINS {
  RS = 27,
  RW = 0, EN, D4, D5, D6, D7,
  BACKLIGHT = 24
};

LiquidCrystalFast lcd(RS, RW, EN, D4, D5, D6, D7);

void setupOutput() {
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

void setupInput () {
  pinMode (PIN_LEFT_ENC_A, INPUT_PULLUP);
  pinMode (PIN_LEFT_ENC_B, INPUT_PULLUP);
  pinMode (PIN_LEFT_IN, INPUT_PULLUP);
  pinMode (PIN_RIGHT_ENC_A, INPUT_PULLUP);
  pinMode (PIN_RIGHT_ENC_B, INPUT_PULLUP);
  pinMode (PIN_RIGHT_IN, INPUT_PULLUP);
}

///////////////////////////////////////////////////////////////////////////////
// X-Plane objects
//
enum DATAREF_NAMES {
  NAV1, NAV2, COM1, COM2, ADF1, ADF2, XP_MODE, XP_CODE,
  DATAREF_COUNT
};

enum XP_MODES {
  XP_OFF, XP_STBY, XP_ON, XP_ALT, XP_TEST,
  XP_MODE_COUNT
};

FlightSimInteger dataref[DATAREF_COUNT];

void setupDataref() {
  dataref[NAV1] = XPlaneRef("sim/cockpit2/radios/actuators/nav1_frequency_hz");
  dataref[NAV2] = XPlaneRef("sim/cockpit2/radios/actuators/nav2_frequency_hz");
  dataref[COM1] = XPlaneRef("sim/cockpit2/radios/actuators/com1_frequency_hz");
  dataref[COM2] = XPlaneRef("sim/cockpit2/radios/actuators/com2_frequency_hz");
  dataref[ADF1] = XPlaneRef("sim/cockpit2/radios/actuators/adf1_frequency_hz");
  dataref[ADF2] = XPlaneRef("sim/cockpit2/radios/actuators/adf2_frequency_hz");
  dataref[XP_CODE] = XPlaneRef("sim/cockpit2/radios/actuators/transponder_code");
  dataref[XP_MODE] = XPlaneRef("sim/cockpit/radios/transponder_mode");
}

///////////////////////////////////////////////////////////////////////////////
// Local objects
//
short channel = NAV1; // indicates selected channel

void radioDisplayUpdate();
void radioInputUpdate();

elapsedMillis dispTimer = 0; //to avoid updating display too frequently
unsigned int dispPeriod = 40;

#endif
