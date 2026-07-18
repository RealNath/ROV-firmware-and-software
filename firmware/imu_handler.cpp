#include "imu_handler.h"
#include "ethernet_handler.h"

// BNO055 shares the same I2C bus as the ADS1115 (see hardware_config.h: PIN_I2C_SDA / PIN_I2C_SCL)
// Default I2C address is 0x28 (0x29 if the ADR pin is pulled HIGH on your breakout board)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

void init_imu() {
  // Note: Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL) may already be called in init_sensor().
  // Calling it again here is safe, but make sure init_sensor() and init_imu()
  // are not fighting over conflicting pins.
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (!bno.begin()) {
    String message = "Failed to Initialize IMU (BNO055)";
    send_ethernet_data(message);
    while (1);
  }

  delay(1000);

  // Use the onboard 32.768kHz crystal if your BNO055 board has one (most breakout boards do).
  // This improves the accuracy of the internal sensor fusion.
  bno.setExtCrystalUse(true);

  Serial.println("IMU (BNO055) Initialized Successfully.");
}

ImuData read_imu_data() {
  ImuData data;

  // Request a fused orientation reading (VECTOR_EULER by default with getEvent)
  sensors_event_t event;
  bno.getEvent(&event);

  data.heading = event.orientation.x; // yaw
  data.roll    = event.orientation.y;
  data.pitch   = event.orientation.z;

  return data;
}
