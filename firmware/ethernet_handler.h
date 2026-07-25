#ifndef ETHERNET_HANDLER_H
#define ETHERNET_HANDLER_H

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include "hardware_config.h"

// --- NETWORK CONFIGURATION ---
constexpr uint16_t LOCAL_PORT  = 8888;
constexpr uint16_t REMOTE_PORT = 8888;

const IPAddress STATIC_IP(192, 168, 42, 177);
const IPAddress GATEWAY(192, 168, 42, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress REMOTE_IP(192, 168, 42, 99);

#pragma pack(push, 1)

// Data sent FROM the ROV to the surface
struct RovTelemetry {
  float depth;
  float yaw;
  float roll;
  float pitch;
};

// Data received BY the ROV from the surface
struct RovCommand {
  int16_t thruster_pwm[NUM_THRUSTERS];
  bool lights_on;
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