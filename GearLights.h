#ifndef GEARLIGHTS_H
#define GEARLIGHTS_H


////////////////////////////////////////
// Hardware input
//
const int gearSwitchPin = 45;
Bounce gearSwitch = Bounce (gearSwitchPin, 5);

////////////////////////////////////////
// Hardware output
//
const int greenLeftPin  = 15;
const int greenNosePin  = 16;
const int greenRightPin = 17;

const int redLeftPin    = 12;
const int redNosePin    = 13;
const int redRightPin   = 14;

////////////////////////////////////////
// X-Plane input and output
//
// Landing gear handle commands
FlightSimCommand gearUp;
FlightSimCommand gearDown;

// Landing gear actual position
FlightSimFloat gearDeployLeft;
FlightSimFloat gearDeployNose;
FlightSimFloat gearDeployRight;

// Landing gear handle position
FlightSimInteger gearHandleDown;

// Aircraft essential bus voltage
FlightSimFloat supplyVolts;



void setupGearLights() {
  //////////////////
  // Input
  //
  pinMode (gearSwitchPin, INPUT_PULLUP);

  //////////////////
  // Output
  //
  pinMode(greenLeftPin,  OUTPUT);
  pinMode(greenNosePin,  OUTPUT);
  pinMode(greenRightPin, OUTPUT);

  pinMode(redLeftPin,    OUTPUT);
  pinMode(redNosePin,    OUTPUT);
  pinMode(redRightPin,   OUTPUT);

  //////////////////
  // X-Plane
  //
  gearUp = XPlaneRef("sim/flight_controls/landing_gear_up");
  gearDown = XPlaneRef("sim/flight_controls/landing_gear_down");

  gearDeployLeft =  XPlaneRef("sim/flightmodel2/gear/deploy_ratio[1]");
  gearDeployNose =  XPlaneRef("sim/flightmodel2/gear/deploy_ratio[0]");
  gearDeployRight = XPlaneRef("sim/flightmodel2/gear/deploy_ratio[2]");

  gearHandleDown =  XPlaneRef("sim/cockpit2/controls/gear_handle_down");

  supplyVolts =     XPlaneRef("sim/cockpit2/electrical/bus_volts[0]");
}



void loopGearLights() {
  FlightSim.update();
  gearSwitch.update();

  //////////////////
  // Process input
  //

  // blocking input on gear handle position
  if(gearSwitch.read() == LOW) { // if the switch is closed
    if (gearHandleDown == 0) { // if gear handle is up
      gearDown.once(); // move it down
    }
  } else {
    if (gearHandleDown == 1) { // if gear handle is down
      gearUp.once(); // move it up
    }
  } // gearSwitch

  //////////////////
  // Process output
  //

  // we need 10V and the sim to be running to light the LEDs
  bool canLight = (supplyVolts > 10.0) && FlightSim.isEnabled();

  // light red LEDs if gear handle and position disagree and we have power
  digitalWrite(redLeftPin,  (gearHandleDown != gearDeployLeft)  && canLight);
  digitalWrite(redNosePin,  (gearHandleDown != gearDeployNose)  && canLight);
  digitalWrite(redRightPin, (gearHandleDown != gearDeployRight) && canLight);

  // light green LEDs if gear down and we have power
  digitalWrite(greenLeftPin,  (gearDeployLeft  == 1.0) && canLight);
  digitalWrite(greenNosePin,  (gearDeployNose  == 1.0) && canLight);
  digitalWrite(greenRightPin, (gearDeployRight == 1.0) && canLight);

}

#endif // GEARLIGHTS_H
