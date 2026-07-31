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

unsigned long last_pid_time = 0;
unsigned long last_telemetry_time = 0;


RovCommand current_cmd = {
    RovCommandType::Translate,
    { .translationData = {0.0f, 0.0f, 0.0f} }
};


RovTelemetry current_telem = {
    0.0f, 
    {0.0f, 0.0f, 0.0f}, 
    {0.0f, 0.0f, 0.0f}, 
    false, 
    false
};



void setup() {
    Serial.begin(ESP32_BAUD_RATE);
    Wire.begin();
    
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
        sensors_vec_t rotationVel = sensor->getRotationVelocity();
        
        current_telem.rotationData.yaw = orientationData.x;   // Heading
        current_telem.rotationData.roll = orientationData.y;  // Roll
        current_telem.rotationData.pitch = orientationData.z; // Pitch
        current_telem.depth = sensor->getApproxDepth();       // Depth

        // Run PID for stabilization and write to thrusters
        rovControl.update(rotationVel, dt);

        // 3. Send Telemetry to Surface at 10Hz 
        if(now - last_telemetry_time >= 100){
            eth->sendTelemetry(current_telem);
            last_telemetry_time = now;
        }

        last_pid_time = now;
    }
}