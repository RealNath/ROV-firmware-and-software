#include "thruster_control.h"

bool ThrusterControl::init() {
  int thruster_pins[NUM_THRUSTERS] = {
    PIN_THRUSTER_1, PIN_THRUSTER_2, PIN_THRUSTER_3, 
    PIN_THRUSTER_4, PIN_THRUSTER_5, PIN_THRUSTER_6
  };
  
  Serial.println("Attaching and arming ESCs....");
  
  for (int i = 0; i < NUM_THRUSTERS; i++) {
    escArray[i].setPeriodHertz(PWM_FREQ);
    escArray[i].attach(thruster_pins[i], 1000, 2000); 

    // Send neutral signal (1500us) so that the ESC can arm
    escArray[i].writeMicroseconds(1500);
  }
  
  Serial.println("ESCs armed");

  return true;
}

void ThrusterControl::set(int index, int microseconds) {
  // Safety check: don't write outside the bounds of the array
  if (index < 0 || index >= NUM_THRUSTERS) return; 

  // Constrain the PWM value safely
  if (microseconds > 2000) {
    microseconds = 2000;
  } else if (microseconds < 1000) {
    microseconds = 1000;
  }
  
  // Write directly to the target hardware instance
  escArray[index].writeMicroseconds(microseconds);
}