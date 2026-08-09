#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- I2C BUS (IMU & ADS1115) ---
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9

// --- BNO055 ---
#define BNO_HARDWARE_ID     55
#define BNO_I2C_ADDR        0x28

// --- ESP32 ---
#define ESP32_BAUD_RATE 115200

// --- SPI BUS (W5500 ETHERNET) ---
#define PIN_SPI_MOSI        12
#define PIN_SPI_MISO        13
#define PIN_SPI_SCK         11
#define PIN_W5500_CS        10
#define PIN_W5500_RST       -1


// --- BLDC THRUSTER ESCs ---
#define NUM_THRUSTERS       6
#define PIN_THRUSTER_FL     4
#define PIN_THRUSTER_FR     15
#define PIN_THRUSTER_ML     7
#define PIN_THRUSTER_MR     17
#define PIN_THRUSTER_BL     5
#define PIN_THRUSTER_BR     16
#define PWM_FREQ            50
#define ESC_LOW             1000
#define ESC_HIGH            2000
#define ESC_NEUTRAL         1500

// --- ADC CHANNELS ---
#define ADS1115_PRESSURE_CH 0


// PID Parameters
#define PID_KI                 1
#define PID_KD                 1
#define PID_KP                 1
#define INTEGRAL_MAX          100

#endif