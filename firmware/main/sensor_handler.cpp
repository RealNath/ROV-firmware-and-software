#include "sensor_handler.h"


SensorHandler::SensorHandler(): depthConstant(DEFAULT_DEPTH_CONSTANT), initialized(false) {
    Serial.println("[SENSOR] Initializing BNO055");
    bno = new Adafruit_BNO055(BNO_HARDWARE_ID, BNO_I2C_ADDR, &Wire);
    if(!bno->begin()){
        Serial.println("[SENSOR] Failed to initialize BNO055");
        return;
    }

    delay(1000);
    bno->setExtCrystalUse(true);
    Serial.println("[SENSOR] IMU (BNO055) initialized successfully.");


    Serial.println("[SENSOR] Initializing ADC for depth sensor");
    ads = new Adafruit_ADS1115();
    if(!ads->begin()){
        Serial.println("[SENSOR] Failed to initialize ADC for depth sensor");
    }
    Serial.println("[SENSOR] ADC for depth sensor initialized successfully.");

    initialized = true;
}


sensors_vec_t SensorHandler::getRotation(int64_t *timestampPtr){
    sensors_event_t event;
    bno->getEvent(&event, Adafruit_BNO055::VECTOR_EULER);
    if(timestampPtr) *timestampPtr = event.timestamp;
    return event.orientation;
}

sensors_vec_t SensorHandler::getRotationVelocity(int64_t *timestampPtr){
    sensors_event_t event;
    bno->getEvent(&event, Adafruit_BNO055::VECTOR_GYROSCOPE);
    if(timestampPtr) *timestampPtr = event.timestamp;
    return event.gyro;
}

sensors_vec_t SensorHandler::getLinearAcceleration(int64_t *timestampPtr){
    sensors_event_t event;
    bno->getEvent(&event, Adafruit_BNO055::VECTOR_LINEARACCEL);
    if(timestampPtr) *timestampPtr = event.timestamp;
    return event.acceleration;
}

sensors_vec_t SensorHandler::getGravity(int64_t *timestampPtr){
    sensors_event_t event;
    bno->getEvent(&event, Adafruit_BNO055::VECTOR_GRAVITY);
    if(timestampPtr) *timestampPtr = event.timestamp;
    return event.orientation;
}

sensors_vec_t SensorHandler::getMagnetism(int64_t *timestampPtr){
    sensors_event_t event;
    bno->getEvent(&event, Adafruit_BNO055::VECTOR_MAGNETOMETER);
    if(timestampPtr) *timestampPtr = event.timestamp;
    return event.magnetic;
}

int8_t SensorHandler::getTemperature(){
    return bno->getTemp();
}


float SensorHandler::getApproxDepth(){
    int16_t adc0 = ads->readADC_SingleEnded(0);
    float voltage = ads->computeVolts(adc0);
    return voltage * depthConstant;
}


void SensorHandler::correctDepthConstant(float recordedDepth){
    int16_t adc0 = ads->readADC_SingleEnded(0);
    float voltage = ads->computeVolts(adc0);
    depthConstant = voltage * recordedDepth; 
}
