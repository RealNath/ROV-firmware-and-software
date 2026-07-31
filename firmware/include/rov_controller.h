#pragma once
#include "ethernet_handler.h"
#include "thruster_control.h"
#include "pid_controller.h"
#include "sensor_handler.h"

class RovController {
private:
    ThrusterHandler* thrusters;
    
    // Target speeds/forces from joystick
    float manual_surge;
    float manual_sway;
    float manual_heave;
    float manual_yaw;
    float manual_roll;
    
    // PIDs for stabilization (zero speed when idle)
    PID yawRatePID;
    PID rollRatePID;
    
    float mapToPWM(float normalizedValue);

public:
    RovController();
    void setThrusterHandler(ThrusterHandler* tm);
    
    void handleTranslate(float x, float y, float z);
    void handleRotate(float roll, float pitch, float yaw);
    
    void update(const sensors_vec_t& rotationVelocity, float dt);
};