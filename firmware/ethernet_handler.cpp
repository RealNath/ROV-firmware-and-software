#include "ethernet_handler.h"
#include <SPI.h>

bool Ethernet::init() {
  Serial.println("Initializing Ethernet (W5500)...");

  // 1. Manual Hardware Reset (The Magic Bullet)
  // This forces the W5500 to wake up and clears the "read PHY register failed" error
  if (PIN_W5500_RST != -1) {
    pinMode(PIN_W5500_RST, OUTPUT);
    digitalWrite(PIN_W5500_RST, LOW);
    delay(10);
    digitalWrite(PIN_W5500_RST, HIGH);
    delay(150); // Give it time to boot up
  }

  // 2. Initialize the ESP32-S3 SPI bus (Without CS pin, exactly like your working code)
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

  // 3. Start the ETH library using the global SPI object
  if (!ETH.begin(ETH_PHY_W5500, 1, PIN_W5500_CS, -1, PIN_W5500_RST, SPI)) {
    Serial.println("❌ ERROR: ETH.begin failed. Check SPI wiring.");
    return false;
  }
  
  Serial.println("✅ ETH Driver Started Successfully.");

  // 4. Force the Static IP configuration
  ETH.config(STATIC_IP, GATEWAY, SUBNET, GATEWAY, GATEWAY);
  
  // Give the router/switch time to negotiate the link
  delay(3000);
  
  if (ETH.linkUp()) {
    Serial.print("✅ Ethernet Link UP. ROV IP: ");
    Serial.println(ETH.localIP());
  } else {
    Serial.println("⚠️ Ethernet Link DOWN (Is the tether unplugged?)");
  }

  // 5. Start listening for binary UDP packets
  udp.begin(LOCAL_PORT);
  
  return true;
}

// --- Binary Struct Communication (Scalable & Fast) ---

void Ethernet::sendTelemetry(RovTelemetry data) {
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
    while(udp.available()) { udp.read(); } // Clear the buffer
  }
  
  return false; 
}