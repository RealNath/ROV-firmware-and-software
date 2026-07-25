#include "ethernet_handler.h"

bool Ethernet::init() {
  Serial.println("Initializing Ethernet (W5500) with Static IP...");

  // 1. Configure Static IP FIRST
  if (!ETH.config(STATIC_IP, GATEWAY, SUBNET, GATEWAY)) {
    Serial.println("Static IP Configuration Failed!");
  }

  // 2. Initialize SPI Bus for Ethernet
  SPI.begin(ETH_SCLK, ETH_MISO, ETH_MOSI, ETH_CS);

  // 3. Initialize W5500 specifically
  if (!ETH.begin(ETH_PHY_W5500, 1, ETH_CS, ETH_IRQ, ETH_RST, SPI)) {
    Serial.println("Ethernet hardware failed to initialize.");
    return false;
  }

  udp.begin(LOCAL_PORT);
  Serial.print("Ethernet initialized. ROV IP: ");
  Serial.println(ETH.localIP());
  return true;
}

void Ethernet::sendTelemetry(RovTelemetry data) {
  // Use the constants directly to target the surface computer
  udp.beginPacket(REMOTE_IP, REMOTE_PORT);
  udp.write((uint8_t*)&data, sizeof(data));
  udp.endPacket();
}

bool Ethernet::receiveCommand(RovCommand &commandOut) {
  int packetSize = udp.parsePacket();
  
  if (packetSize == sizeof(RovCommand)) {
    udp.read((uint8_t*)&commandOut, sizeof(RovCommand));
    return true; 
    
  } else if (packetSize > 0) {
    Serial.println("Received mismatched UDP packet size.");
    while(udp.available()) { udp.read(); } 
  }
  
  return false; 
}