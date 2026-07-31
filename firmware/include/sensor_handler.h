#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#define DEFAULT_DEPTH_CONSTANT 100

#include "hardware_config.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>


class SensorHandler {

private:
    Adafruit_BNO055 *bno;
    Adafruit_ADS1115 *ads;
    float depthConstant;
    bool initialized;

public:
    SensorHandler();
    ~SensorHandler() {};

    bool isInitialized(){ return initialized; }

    sensors_vec_t getRotation(int64_t *timestampPtr = nullptr);
    sensors_vec_t getRotationVelocity(int64_t *timestampPtr = nullptr);
    sensors_vec_t getLinearAcceleration(int64_t *timestampPtr = nullptr);
    sensors_vec_t getGravity(int64_t *timestampPtr = nullptr);
    sensors_vec_t getMagnetism(int64_t *timestampPtr = nullptr);
    int8_t getTemperature();
    float getApproxDepth();

    void correctDepthConstant(float recordedDepth);
};
#endif