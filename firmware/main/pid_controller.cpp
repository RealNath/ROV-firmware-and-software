#include "pid_controller.h"

PID::PID(float kp, float ki, float kd, bool is_heading) 
    : _kp(kp), _ki(ki), _kd(kd), _is_heading(is_heading), _integral(0.0), _prev_error(0.0) {}


void PID::setTunings(float kp, float ki, float kd) {
    if(_kp != kp || _ki != ki) {
        reset(); 
    }

    _kp = kp; 
    _ki = ki; 
    _kd = kd;
}


void PID::reset() {
    _integral = 0.0;
    _prev_error = 0.0;
}


float PID::compute(float setpoint, float current, float dt) {
    if (dt <= 0.0) return 0.0;

    float error = setpoint - current;
    
    if(_is_heading){
        if (error > 180.0f) error -= 360.0f;
        if (error < -180.0f) error += 360.0f;
    }

    _integral += error * dt;
    float derivative = (error - _prev_error) / dt;
    _prev_error = error;

    return (_kp * error) + (_ki * _integral) + (_kd * derivative);
}