#include <Wire.h>
#include <Arduino.h>

#include "sensor_handler.h"
#include "ethernet_handler.h"
#include "thruster_control.h"
#include "rov_controller.h"


EthernetHandler *eth;
SensorHandler *sensor;
ThrusterHandler *thrusters;
RovController rovControl;

const IPAddress STATIC_IP(192, 168, 42, 177);
const IPAddress GATEWAY(192, 168, 42, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress REMOTE_IP(192, 168, 42, 99);

uint64_t last_pid_time = 0;
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
    
    // Initialize Thrusters
    thrusters = new ThrusterHandler(ESC_LOW, ESC_HIGH, ESC_NEUTRAL); // Need to re-instantiate it since it was removed
    rovControl.setThrusterHandler(thrusters);

    // Initialize Ethernet connection
    eth = new EthernetHandler(STATIC_IP, GATEWAY, SUBNET);
    if(!eth->isInitialized()){
        Serial.println("[SETUP] Ethernet failed to initialize");
        while(true);
    }
    
    last_pid_time = millis();
}



void loop() {
    unsigned long now = millis();
    
    // 1. Receive Commands
    RovCommand cmd;
    eth->receiveCommand(cmd);
    
    // 2. Execute 50Hz PID Control Loop 
    float dt = (now - last_pid_time) / 1000.0f;
    if(dt >= 0.02f) {
        // Read IMU data
        sensors_vec_t orientationData = sensor->getRotation();
        sensors_vec_t accData = sensor->getLinearAcceleration();
        sensors_vec_t rotationVel = sensor->getRotationVelocity();
        
        currTelemetry.accelerationData.x = accData.x;         // Acceleration on X-axis
        currTelemetry.accelerationData.y = accData.y;         // Acceleration on Y-axis
        currTelemetry.accelerationData.z = accData.z;         // Acceleration on Z-axis
        currTelemetry.rotationData.yaw = orientationData.x;   // Heading
        currTelemetry.rotationData.roll = orientationData.y;  // Roll
        currTelemetry.rotationData.pitch = orientationData.z; // Pitch
        currTelemetry.depth = sensor->getApproxDepth();       // Depth
        currTelemetry.temperature = sensor->getTemperature(); // Temperature

        // Run PID for stabilization and write to thrusters
        rovControl.update(rotationVel, dt);

        // 3. Send Telemetry to Surface at 10Hz 
        if(now - last_telemetry_time >= 100){
            eth->sendTelemetry(currTelemetry);
            last_telemetry_time = now;
        }

        last_pid_time = now;
    }
}