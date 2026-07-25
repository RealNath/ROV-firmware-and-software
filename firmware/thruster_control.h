#ifndef THRUSTER_CONTROL_H
#define THRUSTER_CONTROL_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "hardware_config.h"
#include "ethernet_handler.h"

class ThrusterControl {
  private:
    Servo escArray[NUM_THRUSTERS]; 

  public:
    bool init();
    void set(int index, int microseconds);
};

#endif