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
    thrusters->setAll(ESC_LOW); delay(2000);
    thrusters->setFrontLeft(ESC_NEUTRAL); delay(2000);
    thrusters->setFrontRight(ESC_NEUTRAL); delay(2000);
    thrusters->setMiddleLeft(ESC_NEUTRAL); delay(2000);
    thrusters->setMiddleRight(ESC_NEUTRAL); delay(2000);
    thrusters->setBackLeft(ESC_NEUTRAL); delay(2000);
    thrusters->setBackRight(ESC_NEUTRAL); delay(2000);
}