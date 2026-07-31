#ifndef ETHERNET_HANDLER_H
#define ETHERNET_HANDLER_H

// Arduino.h MUST come first so IDF C headers are wrapped in extern "C" properly
#include <Arduino.h>
#include <ETH.h>
#include <NetworkUdp.h>
#include "hardware_config.h"

#define LOCAL_PORT   8888   // ESP32 listens for commands on this port
#define REMOTE_PORT  8889   // PC listens for telemetry/callbacks on this port

extern const IPAddress STATIC_IP;
extern const IPAddress GATEWAY;
extern const IPAddress SUBNET;
extern const IPAddress REMOTE_IP;

enum class RovCommandType : uint8_t {
    Translate,
    Rotate,
    SetLightOn,
    SetGripperHold,
    CorrectDepth,
    RovCallback
};

struct RovCommand {
    RovCommandType command;
    union {
      struct { float x, y, z; } translationData;
      struct { float roll, pitch, yaw; } rotationData;
      bool gripperHold;
      bool lightsOn;
      float depthCorrection;
    };
} __attribute__((packed));

struct RovTelemetry {
    float depth;
    struct { float x, y, z; } accelerationData;
    struct { float roll, pitch, yaw; } rotationData;
    int8_t temperature;
    bool isGripperHold;
    bool isLightsOn;
};


class EthernetHandler {
private:
    NetworkUDP *udp;
    IPAddress lastRemoteIP;
    uint16_t lastRemotePort;
    bool hasRemoteClient;
    bool initialized;

public:
    EthernetHandler(IPAddress staticIP, IPAddress gatewayIP, IPAddress subnet);
    ~EthernetHandler() {};

    bool isInitialized() const { return initialized; }

    void sendTelemetry(const RovTelemetry& data);
    void sendCallback(RovCommand &commandOut);
    bool receiveCommand(RovCommand &commandOut);
};

#endif