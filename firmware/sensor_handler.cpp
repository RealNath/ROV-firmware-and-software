#include "sensor_handler.h"


bool DepthSensor::init(){ // changes into boolean. so if the init returns false, we can halt the program on the main file.
  if (!ads.begin()){
    Serial.println("depth sensor failed");
    return false;
  }
  return true;
}

float DepthSensor::read(){
  int16_t adc0 = ads.readADC_SingleEnded(0);
  
  // Convert the raw value to actual voltage
  float voltage = ads.computeVolts(adc0);

  float depth = voltage*depth_constant;

  return depth;
}

bool ImuSensor::init(){ // changes into boolean. so if the init returns false, we can halt the program on the main file.

  Serial.println("initializing IMU");
  if (!bno.begin()) {
    Serial.println("failed to initialize IMU");
    return false;
  }

  delay(1000);
  bno.setExtCrystalUse(true);
  Serial.println("IMU (BNO055) Initialized Successfully.");

  return true;
}

ImuData ImuSensor::read(){
  ImuData data; 

  // Request a fused orientation reading (VECTOR_EULER by default with getEvent)
  sensors_event_t event;
  bno.getEvent(&event);

  data.yaw = event.orientation.x; // yaw
  data.roll    = event.orientation.y;
  data.pitch   = event.orientation.z;

  return data; // so, this sends a struct? what is struct anyway?
}
