#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#include "ethernet_handler.h"
#include "thruster_control.h"
#include "rov_controller.h"

/* Hardware & Logic Class Instances */
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
Ethernet eth;
ThrusterControl thrusters;
RovController rovControl;

/* State Variables */
uint8_t pressure_sensor_addr = 0x00;
unsigned long last_pid_time = 0;
unsigned long last_telemetry_time = 0;

RovCommand current_cmd = {
    0, 0,           // manual_surge, manual_sway
    0.0, 0.5,       // setpoint_yaw, setpoint_depth
    2.0, 0.0, 0.5,  // Kp, Ki, Kd Yaw
    5.0, 0.1, 1.0,  // Kp, Ki, Kd Depth
    false,          // lights_on
    {0, 0, 0, 0, 0, 0} // thruster_pwm overrides
};

RovTelemetry current_telem = {0.0f, 0.0f, 0.0f, 0.0f};

/* Mock function to read depth from I2C sensor */
float readDepthSensor() {
    if(pressure_sensor_addr == 0x00) return 0.0;
    Wire.requestFrom(pressure_sensor_addr, (uint8_t)2);
    if(Wire.available() >= 2) {
        uint16_t raw = (Wire.read() << 8) | Wire.read();
        return (float)raw / 1000.0; 
    }
    return 0.0;
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    /* Sensor Discovery */
    for(uint8_t i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        if(Wire.endTransmission() == 0 && i != 0x28 && i != 0x29) {
            pressure_sensor_addr = i;
        }
    }

    if(bno.begin()) {
        bno.setExtCrystalUse(true);
    }
    
    // Initialize OO Thrusters
    if (!thrusters.init()) {
        Serial.println("Warning: Thruster initialization returned false!");
    }

    // Initialize Network
    eth.init();
    
    last_pid_time = millis();
}

void loop() {
    unsigned long now = millis();
    
    /* 1. Receive Commands from Surface Computer */
    RovCommand incoming_cmd;
    if (eth.receiveCommand(incoming_cmd)) {
        current_cmd = incoming_cmd;
        rovControl.updateTunings(current_cmd);
    }
    
    /* 2. Execute 50Hz PID Control Loop */
    float dt = (now - last_pid_time) / 1000.0;
    if(dt >= 0.02) {
        
        // Read BNO055 Euler Angles (Yaw, Roll, Pitch)
        sensors_event_t orientationData;
        bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
        
        current_telem.yaw = orientationData.orientation.x;   // Heading
        current_telem.roll = orientationData.orientation.y;  // Roll
        current_telem.pitch = orientationData.orientation.z; // Pitch
        current_telem.depth = readDepthSensor();      // Depth

        // Run PID and send signals to thrusters instance
        rovControl.computeAndMix(
            current_cmd, 
            current_telem.yaw, 
            current_telem.depth, 
            dt,
            thrusters
        );

        /* 3. Send Telemetry to Surface at 10Hz */
        if (now - last_telemetry_time >= 100) {
            eth.sendTelemetry(current_telem);
            last_telemetry_time = now;
        }

        last_pid_time = now;
    }
}