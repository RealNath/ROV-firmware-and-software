#pragma once

class PID {
private:
    float _kp, _ki, _kd;
    float _integral;
    float _prev_error;
    bool _is_heading;

public:
    PID(float kp, float ki, float kd, bool is_heading = false);
    
    void setTunings(float kp, float ki, float kd);
    
    void reset();
    
    float compute(float setpoint, float current, float dt);
};