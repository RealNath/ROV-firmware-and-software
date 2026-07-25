#pragma once
#include "ethernet_handler.h"
#include "thruster_control.h"
#include "pid_controller.h"

class RovController {
private:
    PID yawPID;
    PID depthPID;

public:
    RovController();
    
    void updateTunings(const RovCommand& cmd);
    void computeAndMix(const RovCommand& cmd, float current_yaw, float current_depth, float dt, ThrusterControl& thrusters);
};