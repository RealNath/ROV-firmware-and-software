#ifndef THRUSTER_CONTROL_H
#define THRUSTER_CONTROL_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "hardware_config.h"
#include "ethernet_handler.h"


class ThrusterHandler {
private:
    Servo servosEscArray[NUM_THRUSTERS];

public:
    ThrusterHandler(int low, int high, int neutralMicrosecond);
    ~ThrusterHandler() {};

    void set(int index, int microseconds);
    void setFrontLeft(int microseconds);
    void setFrontRight(int microseconds);
    void setMiddleLeft(int microseconds);
    void setMiddleRight(int microseconds);
    void setBackLeft(int microseconds);
    void setBackRight(int microseconds);
    void setAll(int microseconds);
};

#endif