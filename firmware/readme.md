# ROV Control System Firmware

This repository contains the modular ESP32 firmware for our ROV. The codebase is object-oriented and divided into three main hardware handlers: Sensors (I2C), Thrusters (PWM), and Ethernet (W5500/UDP). 

## 📦 Dependencies
Ensure you have the following libraries installed in your Arduino IDE:
*   `Adafruit BNO055` & `Adafruit Unified Sensor` (IMU)
*   `Adafruit ADS1X15` (Depth Sensor ADC)
*   `ESP32Servo` (ESC/Thruster control)
*   `ETH.h` (Built into ESP32 core)

---

## 📡 1. Ethernet Communication (`ethernet_handler.h`)
The Ethernet module handles UDP communication with the surface computer using scalable data structures. Static IPs and ports are predefined in the header file.

### Data Structures
We use packed `structs` to send and receive binary data over UDP. If you need to add a new sensor, simply add it to the struct here — the payload size scales automatically.

**`RovTelemetry` (Sent to Surface)**
| Variable | Type | Description |
| :--- | :--- | :--- |
| `depth` | float | Calculated depth from ADS1115 |
| `yaw` | float | Heading from BNO055 |
| `roll` | float | Roll angle from BNO055 |
| `pitch` | float | Pitch angle from BNO055 |

**`RovCommand` (Received from Surface)**
| Variable | Type | Description |
| :--- | :--- | :--- |
| `thruster_pwm` | int16_t array | PWM values (1000-2000) for all ESCs |
| `lights_on` | bool | Boolean toggle for ROV lights |

### Usage
```cpp
#include "ethernet_handler.h"

Ethernet eth; // Create the object

// Inside setup()
eth.init(); 

// To send data
RovTelemetry telemetry_data = {1.5, 45.0, 0.0, -5.0};
eth.sendTelemetry(telemetry_data);

// To receive data
RovCommand incoming_cmd;
if (eth.receiveCommand(incoming_cmd)) {
  // New command received successfully!
  int forward_pwm = incoming_cmd.thruster_pwm[0];
}
```

---

## 🧭 2. Sensor Handler (`sensor_handler.h`)
This module manages the I2C sensors. **Note:** The I2C bus (`Wire.begin()`) must be initialized in the main `setup()` function before calling the sensor `init()` functions, as both sensors share the same SDA/SCL pins.

### Usage
```cpp
#include <Wire.h>
#include "sensor_handler.h"
#include "hardware_config.h"

DepthSensor depthSensor;
ImuSensor imuSensor;

// Inside setup()
Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); // Start bus first
depthSensor.init();
imuSensor.init();

// Inside loop()
float current_depth = depthSensor.read();

ImuData current_imu = imuSensor.read();
float current_yaw = current_imu.yaw;
```

---

## 🚤 3. Thruster Control (`thruster_control.h`)
Manages the BLDC motors via ESCs. The initialization automatically sends a 1500µs neutral signal to arm the ESCs safely.

### Usage
```cpp
#include "thruster_control.h"

ThrusterControl thrusters;

// Inside setup()
thrusters.init();

// Inside loop() - set(index, microseconds)
// Hardware limits (1000 - 2000) are enforced automatically inside the class
thrusters.set(0, 1600); // Spin thruster 0 slightly forward
```

---

## 🔄 Main File Example (`main.ino`)
Here is how everything integrates into a clean, readable main file:

```cpp
#include <Wire.h>
#include "hardware_config.h"
#include "ethernet_handler.h"
#include "thruster_control.h"
#include "sensor_handler.h"

// 1. Instantiate Objects
Ethernet eth;
ThrusterControl thrusters;
DepthSensor depthSensor;
ImuSensor imuSensor;

void setup() {
  Serial.begin(115200);
  
  // Start shared I2C bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Initialize hardware with failsafes
  if (!eth.init() || !thrusters.init() || !depthSensor.init() || !imuSensor.init()) {
    Serial.println("System HALT: Hardware failure.");
    while (1); 
  }
}

void loop() {
  // --- 1. READ SENSORS ---
  float current_depth = depthSensor.read();
  ImuData current_imu = imuSensor.read();

  // --- 2. SEND TELEMETRY ---
  RovTelemetry telemetry;
  telemetry.depth = current_depth;
  telemetry.yaw = current_imu.yaw;
  telemetry.roll = current_imu.roll;
  telemetry.pitch = current_imu.pitch;
  eth.sendTelemetry(telemetry);

  // --- 3. RECEIVE & EXECUTE COMMANDS ---
  RovCommand command;
  if (eth.receiveCommand(command)) {
    // Apply new PWM values to all thrusters
    for (int i = 0; i < NUM_THRUSTERS; i++) {
      thrusters.set(i, command.thruster_pwm[i]);
    }
  }
  
  delay(50); // Small delay to prevent network flooding
}
```