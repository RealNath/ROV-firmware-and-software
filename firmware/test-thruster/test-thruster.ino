#include <Wire.h>
#include <Arduino.h>

#include "thruster_control.h"
#include "hardware_config.h"

ThrusterHandler *thrusters;

void setup() {
    Serial.begin(ESP32_BAUD_RATE);
    Wire.begin();
    thrusters = new ThrusterHandler(ESC_LOW, ESC_HIGH, ESC_NEUTRAL); 
}


void loop() {
    thrusters->setAll(ESC_NEUTRAL); delay(3000);
    thrusters->setFrontLeft(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
    thrusters->setFrontRight(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
    thrusters->setMiddleLeft(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
    thrusters->setMiddleRight(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
    thrusters->setBackLeft(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
    thrusters->setBackRight(1600); delay(3000); thrusters->setAll(ESC_NEUTRAL);
}