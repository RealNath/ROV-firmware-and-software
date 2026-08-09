#include <Wire.h>
#include <Arduino.h>

#include "sensor_handler.h"
#include "ethernet_handler.h"
#include "thruster_control.h"
#include "rov_controller.h"

#define TELEMETRY_FREQUENCY 10
#define TELEMETRY_PERIOD (1000 / TELEMETRY_FREQUENCY)

EthernetHandler *eth;
SensorHandler *sensor;
ThrusterHandler *thrusters;
RovController *rovControl;

const IPAddress STATIC_IP(192, 168, 42, 177);
const IPAddress GATEWAY(192, 168, 42, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress REMOTE_IP(192, 168, 42, 99);

uint64_t last_telemetry_time = 0;


RovTelemetry currTelemetry = {
    .depth = 0.0f, 
    .accelerationData = {0.0f, 0.0f, 0.0f}, 
    .rotationData = {0.0f, 0.0f, 0.0f}, 
    .temperature = 0,
    .isGripperHold = false, 
    .isLightsOn = false
};



void setup() {
    Serial.begin(ESP32_BAUD_RATE);
    Wire.begin();

    delay(2000);
    
    // Initialize BNO055 and depth sensor through ADS115
    sensor = new SensorHandler();
    if(!sensor->isInitialized()){
        Serial.println("[SETUP] Sensor manager failed to initialize");
        while(true);
    }
    
    // Initialize Thrusters and controller (never returns NULL)
    thrusters = new ThrusterHandler(ESC_LOW, ESC_HIGH, ESC_NEUTRAL);
    rovControl = new RovController(thrusters);

    // Initialize Ethernet connection
    eth = new EthernetHandler(STATIC_IP, GATEWAY, SUBNET);
    if(!eth->isInitialized()){
        Serial.println("[SETUP] Ethernet failed to initialize");
        while(true);
    }
    
}



void loop() {
    uint64_t now = millis();
    
    // Receive Commands
    RovCommand cmd;
    if(eth->receiveCommand(cmd)){
        eth->sendCallback(cmd);
    }

    rovControl->update();

    // Send Telemetry to Surface at 10Hz 
    if(now - last_telemetry_time >= TELEMETRY_PERIOD){

        // Read IMU data
        sensors_vec_t orientationData = sensor->getRotation();
        sensors_vec_t accData = sensor->getLinearAcceleration();
        
        currTelemetry.accelerationData.x = accData.x;         // Acceleration on X-axis
        currTelemetry.accelerationData.y = accData.y;         // Acceleration on Y-axis
        currTelemetry.accelerationData.z = accData.z;         // Acceleration on Z-axis
        currTelemetry.rotationData.yaw = orientationData.x;   // Heading
        currTelemetry.rotationData.roll = orientationData.y;  // Roll
        currTelemetry.rotationData.pitch = orientationData.z; // Pitch
        currTelemetry.depth = sensor->getApproxDepth();       // Depth
        currTelemetry.temperature = sensor->getTemperature(); // Temperature

        eth->sendTelemetry(currTelemetry);
        last_telemetry_time = now;

    }

}