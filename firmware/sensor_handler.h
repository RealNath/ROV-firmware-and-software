#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#define SENSOR_CONSTANT 234

#include "hardware_config.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>

struct ImuData {
  float yaw;
  float roll;
  float pitch;
};

class DepthSensor{
  private:
    float depth_constant = SENSOR_CONSTANT;
    Adafruit_ADS1115 ads;
  
  public:
    bool init();
    float read();
};


class ImuSensor{
  private:
    // THIS is where you initialize the object so the whole class can use it.
    // 55 is a sensor ID, 0x28 is the default I2C address, and &Wire tells it 
    // to use the I2C bus you started in your main sketch.
    Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
  public:
    bool init();
    ImuData read();
};
#endif