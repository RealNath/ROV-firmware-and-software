#ifndef ETHERNET_HANDLER_H
#define ETHERNET_HANDLER_H

#include <stdint.h>
#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include "hardware_config.h"

// --- NETWORK CONFIGURATION ---
constexpr uint16_t LOCAL_PORT  = 8888;
constexpr uint16_t REMOTE_PORT = 8888;

const IPAddress STATIC_IP(192, 168, 42, 177);
const IPAddress GATEWAY(192, 168, 42, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress REMOTE_IP(192, 168, 42, 99);

#define ETH_CS   5   // CS / SS pin
#define ETH_IRQ  4   // Interrupt pin (-1 if not connected)
#define ETH_RST  16  // Reset pin (-1 if tied to 3.3V)
#define ETH_SCLK 18  // SPI SCK
#define ETH_MISO 19  // SPI MISO
#define ETH_MOSI 23  // SPI MOSI

/* Prevent alignment by compiler */
#pragma pack(push, 1)


// Data sent from surface to ROV
struct RovCommand {
    int16_t manual_surge;
    int16_t manual_sway;
    
    float setpoint_yaw;
    float setpoint_depth;

    float Kp_yaw;
    float Ki_yaw;
    float Kd_yaw;

    float Kp_depth;
    float Ki_depth;
    float Kd_depth;

    bool lights_on;
    int16_t thruster_pwm[NUM_THRUSTERS];
};


// Data sent FROM the ROV to the surface
struct RovTelemetry {
    float depth;
    float yaw;
    float roll;
    float pitch;
};

#pragma pack(pop)


class Ethernet {
  private:
    WiFiUDP udp;

  public:
    bool init(); 
    
    void sendTelemetry(RovTelemetry data);
    bool receiveCommand(RovCommand &commandOut);
};

#endif