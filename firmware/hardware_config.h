#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// --- I2C BUS (IMU & ADS1115) ---
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9

// --- SPI BUS (W5500 ETHERNET) ---
#define PIN_SPI_MOSI        12
#define PIN_SPI_MISO        13
#define PIN_SPI_SCK         11
#define PIN_W5500_CS        10
#define PIN_W5500_RST       -1 // rst here isnt actually wired. so there is no rst on physical. its floating

// --- BLDC THRUSTER ESCs ---
#define NUM_THRUSTERS          6
#define PIN_THRUSTER_1         4
#define PIN_THRUSTER_2         5
#define PIN_THRUSTER_3         7
#define PIN_THRUSTER_4         16
#define PIN_THRUSTER_5         17
#define PIN_THRUSTER_6         15
#define PWM_FREQ               50

// --- SERVO PIN ---

#define SERVO_PIN              16

// --- ADC CHANNELS ---
#define ADS1115_PRESSURE_CH 0 // Connected to A0 on the ADS module


// PID Parameters

#define PID_KI                 1
#define PID_KD                 1
#define PID_KP                 1
#define INTEGRAL_MAX          100

#endif