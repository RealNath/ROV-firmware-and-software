#include "rov_controller.h"

RovController::RovController(ThrusterHandler *thrusters) : 
    manual_surge(0), manual_sway(0), manual_heave(0),
    manual_yaw(0), manual_roll(0),
    thrusters(thrusters) {}


float RovController::mapToPWM(float val) {
    if(val < -1.0f) val = -1.0f;
    if(val > 1.0f) val = 1.0f;
    return 1500.0f + (val * 400.0f);
}

void RovController::handleTranslate(float x, float y, float z) {
    manual_surge = x;
    manual_sway = y;
    manual_heave = z;
}

void RovController::handleRotate(float roll, float pitch, float yaw) {
    manual_roll = roll;
    manual_yaw = yaw;
}

void RovController::update() {
    if(!thrusters) return;

    float yawEffort = manual_yaw;
    float rollEffort = manual_roll;

    // Force mixing
    float fl = manual_surge + manual_sway + yawEffort;
    float fr = -(manual_surge - manual_sway - yawEffort);
    float bl = -(manual_surge - manual_sway + yawEffort);
    float br = manual_surge + manual_sway - yawEffort;
    
    float ml = manual_heave + rollEffort;
    float mr = manual_heave - rollEffort;
    
    // Normalize to [-1, 1] if exceeding
    float max_horiz = max(max(abs(fl), abs(fr)), max(abs(bl), abs(br)));
    if(max_horiz > 1.0f) {
        fl /= max_horiz;
        fr /= max_horiz;
        bl /= max_horiz;
        br /= max_horiz;
    }
    
    float max_vert = max(abs(ml), abs(mr));
    if(max_vert > 1.0f) {
        ml /= max_vert;
        mr /= max_vert;
    }

    // Set thrusters
    thrusters->setFrontLeft(mapToPWM(fl));
    thrusters->setFrontRight(mapToPWM(fr));
    thrusters->setBackLeft(mapToPWM(bl));
    thrusters->setBackRight(mapToPWM(br));
    thrusters->setMiddleLeft(mapToPWM(ml));
    thrusters->setMiddleRight(mapToPWM(mr));
}