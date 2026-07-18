#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H

#include "hardware_config.h"
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>

// Struct to hold orientation data (Euler angles, in degrees)
// heading = yaw (0-360), roll and pitch are typically -180 to 180
struct ImuData {
  float heading;
  float roll;
  float pitch;
};

// Function declarations
void init_imu();
ImuData read_imu_data();

#endif
