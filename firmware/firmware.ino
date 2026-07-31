#include <Wire.h>
#include "hardware_config.h"
#include "thruster_control.h"
#include "sensor_handler.h"
#include "ethernet_handler.h"

// Instantiate globally
Ethernet eth;
ImuSensor rovImu; 
ThrusterControl thruster;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  
  Serial.println("Starting preliminary test...");
  
  eth.init(); 
  thruster.init();
  
  if (!rovImu.init()) {
    Serial.println("HALT: IMU Init failed!");
    while(1); 
  }

  Serial.println("System ready.");
}

void loop() {
  // 1. CHECK FOR COMMANDS FROM PC
  RovCommand cmd;
  if (eth.receiveCommand(cmd)) {
    // 2. EXECUTE COMMAND (Update all thrusters)
    for(int i = 0; i < NUM_THRUSTERS; i++) {
      thruster.set(i, cmd.thruster_pwm[i]);
    }
  }

  // 3. READ SENSORS
  ImuData current_imu = rovImu.read();
  
  // 4. SEND TELEMETRY TO PC
  RovTelemetry telemetry;
  telemetry.depth = 0.0; // Placeholder until you add the Depth sensor back
  telemetry.yaw = current_imu.yaw;
  telemetry.pitch = current_imu.pitch;
  telemetry.roll = current_imu.roll;
  
  eth.sendTelemetry(telemetry);

  // Run at roughly 20Hz (50ms delay) to keep UDP traffic stable
  delay(50); 
}