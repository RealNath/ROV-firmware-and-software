#include "ethernet_handler.h"
#include "sensor_handler.h"
#include "rov_controller.h"

extern const IPAddress STATIC_IP;
extern const IPAddress GATEWAY;
extern const IPAddress SUBNET;
extern const IPAddress REMOTE_IP;
extern SensorHandler *sensor;
extern RovController rovControl;

EthernetHandler::EthernetHandler(IPAddress staticIP, IPAddress gatewayIP, IPAddress subnet): 
    lastRemotePort(0), 
    hasRemoteClient(false),
    initialized(false)
{
    Serial.printf("[ETH] Initializing Ethernet (W5500) with Static IP (%s)\n", staticIP.toString().c_str());

    if(PIN_W5500_RST != -1){
        pinMode(PIN_W5500_RST, OUTPUT);
        digitalWrite(PIN_W5500_RST, LOW);
        delay(10);
        digitalWrite(PIN_W5500_RST, HIGH);
        delay(150);
    }

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

    if(!ETH.begin(ETH_PHY_W5500, 1, PIN_W5500_CS, -1, PIN_W5500_RST, SPI)) {
        Serial.println("[ETH] Ethernet hardware failed to initialize.");
        return;
    }

    if(!ETH.config(staticIP, gatewayIP, subnet, gatewayIP, gatewayIP)){
        Serial.println("[ETH] IP configuration failed");
    }

    if(!ETH.linkUp()){
        Serial.println("[ETH] Ethernet Link DOWN (check if plugged correctly)");
    }

    delay(3000);

    udp = new NetworkUDP();
    udp->begin(LOCAL_PORT);

    initialized = true;
    Serial.printf("[ETH] Ethernet initialized. ROV IP: %s\n", ETH.localIP().toString().c_str());
}

void EthernetHandler::sendTelemetry(const RovTelemetry& data) {
    if(hasRemoteClient){
        udp->beginPacket(lastRemoteIP, lastRemotePort);
    } 
    else{
        udp->beginPacket(REMOTE_IP, REMOTE_PORT);
    }

    udp->write((uint8_t*) &data, sizeof(data));
    udp->endPacket();
}

void EthernetHandler::sendCallback(RovCommand &commandOut) {
    if(hasRemoteClient){
        udp->beginPacket(lastRemoteIP, lastRemotePort);
    } 
    else{
        udp->beginPacket(REMOTE_IP, REMOTE_PORT);
    }

    commandOut.command = RovCommandType::RovCallback;
    udp->write((uint8_t*) &commandOut, sizeof(commandOut));
    udp->endPacket();
}

bool EthernetHandler::receiveCommand(RovCommand &commandOut) {
    int packetSize = udp->parsePacket();

    if(packetSize == sizeof(RovCommand)) {
        lastRemoteIP = udp->remoteIP();
        lastRemotePort = udp->remotePort();
        hasRemoteClient = true;

        udp->read((uint8_t*)&commandOut, sizeof(RovCommand));

        switch(commandOut.command){
            case RovCommandType::Translate:
                rovControl.handleTranslate(
                    commandOut.translationData.x,
                    commandOut.translationData.y,
                    commandOut.translationData.z
                );
                sendCallback(commandOut);
                break;

            case RovCommandType::Rotate:
                rovControl.handleRotate(
                    commandOut.rotationData.roll,
                    commandOut.rotationData.pitch,
                    commandOut.rotationData.yaw
                );
                sendCallback(commandOut);
                break;

            case RovCommandType::SetLightOn:
                break;

            case RovCommandType::SetGripperHold:
                break;

            case RovCommandType::CorrectDepth:
                sensor->correctDepthConstant(commandOut.depthCorrection);
                sendCallback(commandOut);
                break;

            default:
                break;
        }

        return true; 
    } 
    else if(packetSize > 0){
        Serial.printf("[ETH] Received mismatched UDP packet size. (size = %d)\n", packetSize);
        while(udp->available()){ 
            udp->read();
        } 
    }

    return false; 
}