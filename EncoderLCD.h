#ifndef ENCODERINPUT_H
#define ENCODERINPUT_H



////////////////////
// Interface prototypes
//
void setupOmniTuneDataref();
void updateOmniTuneDisplay();
void updateOmniTuneInput(const int &modeDelta,
                      const int &leftDelta,
                      const int &rightDelta);

void setupDialDataref();
void updateDialDisplay();
void updateDialInput(const int &modeDelta,
                     const int &leftDelta,
                     const int &rightDelta);



////////////////////
// Internal state
//
enum META_MODE {
  OMNITUNE, DIAL,
  META_MODE_COUNT
};

int metaMode = DIAL;



////////////////////
// Encoders (including integrated buttons)
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



////////////////////
// LCD
//
enum LCD_PINS {
  RS = 27,
  RW = 0, EN, D4, D5, D6, D7,
  BACKLIGHT = 24
};

LiquidCrystalFast lcd(RS, RW, EN, D4, D5, D6, D7);

elapsedMicros lcdTimer = 0; //to avoid updating display too frequently
unsigned int lcdPeriod = 35525; //magic number determined by experimentation



////////////////////
// Hardware setup
//
void setupEncoderLCD () {
  pinMode (PIN_LEFT_IN, INPUT_PULLUP);
  pinMode (PIN_RIGHT_IN, INPUT_PULLUP);

  pinMode (BACKLIGHT, OUTPUT);
  analogWrite (BACKLIGHT, 128);

  lcd.begin (16, 2);

  setupOmniTuneDataref();
  setupDialDataref();
}



////////////////////
// loop function
// read hardware & send results to mode-specific function
// per LCD timer, call mode-specific display function
//
void loopEncoderLCD() {

  // decide whether display should be updated
  bool showDisplay = false;

  if (lcdTimer > lcdPeriod) {
    lcdTimer = 0;
    if (FlightSim.isEnabled()) {
      showDisplay = true;
      analogWrite(BACKLIGHT, 128);
    } else {
      lcd.clear();
      lcd.print("OmniStuff");
      analogWrite(BACKLIGHT, 0);
    }
  }


  // parse input from integrated pushbuttons on encoders
  leftIn.update();
  rightIn.update();

  // pressing both buttons at once will change metamodes
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

  // pressing one button will change the mode within the current metamode
  short modeDelta = 0;
  if (leftIn.risingEdge()) {
    modeDelta = -1;
  }
  if (rightIn.risingEdge()) {
    modeDelta = 1;
  }


  // parse input from encoders
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


  // pass input to input function, and call display function, of
  // selected metamode
  switch(metaMode) {

    case OMNITUNE:
      updateOmniTuneInput(modeDelta, leftDelta, rightDelta);
      if(showDisplay)
        updateOmniTuneDisplay();
      break;

    case DIAL:
      updateDialInput(modeDelta, leftDelta, rightDelta);
      if(showDisplay)
        updateDialDisplay();
      break;
  }

} //encoderLCDUpdate


#endif // ENCODERINPUT_H
