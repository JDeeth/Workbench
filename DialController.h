#ifndef DIALCONTROLLER_H
#define DIALCONTROLLER_H


// Stores mechanics of mapping a pair of encoders to a single cockpit
// dial.
/// \todo refine this class, make bits private etc
class Dial {
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
  Dial(float low, float high, float sc, float ratio, bool lap = false) {
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



////////////////////
// dial modes
//
enum DIAL_MODES {
  HEADING_P1, NAV1_OBS, VSI_BUG,
  DIAL_MODE_COUNT
};

int dialMode = HEADING_P1;

/// \todo put Dials into an array
Dial headingP1 (0.0, 360.0, 0.25, 20, true);
Dial nav1Obs (0.0, 360.0, 0.25, 20, true);
Dial vsiBug (-6000, 6000, 100, 5);

void setupDialDataref() {
  headingP1.dr = XPlaneRef("sim/cockpit2/autopilot/heading_dial_deg_mag_pilot");
  nav1Obs.dr = XPlaneRef("sim/cockpit2/radios/actuators/nav1_obs_deg_mag_pilot");
  vsiBug.dr = XPlaneRef("sim/cockpit2/autopilot/vvi_dial_fpm");
}



////////////////////
// Input handling function
//
void updateDialInput(const int &modeDelta,
                     const int &leftDelta,
                     const int &rightDelta) {
  // update dial mode
  if (modeDelta) {
    dialMode += modeDelta;
    if (dialMode >= DIAL_MODE_COUNT)
      dialMode -= DIAL_MODE_COUNT;
    if(dialMode < 0)
      dialMode += DIAL_MODE_COUNT;
  }

  // update selected dial
  if (leftDelta || rightDelta) {
    switch(dialMode) {
      case HEADING_P1:
        headingP1.addDelta(rightDelta, leftDelta);
        break;
      case NAV1_OBS:
        nav1Obs.addDelta(rightDelta, leftDelta);
        break;
      case VSI_BUG:
        vsiBug.addDelta(rightDelta, leftDelta);
        break;
    }
  } // if enc input
}



////////////////////
// update dial display
//
void updateDialDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);

  switch (dialMode) {
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
    case VSI_BUG:
      lcd.print("VSI Bug");
      lcd.setCursor(0,1);
      lcd.print(vsiBug.dr);
      break;
    default:
      lcd.print("Undefined");
  }
}


#endif // DIALCONTROLLER_H
