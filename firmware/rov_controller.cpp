#include "rov_controller.h"

RovController::RovController() 
    : yawPID(2.0, 0.0, 0.5, true),   
      depthPID(5.0, 0.1, 1.0, false) 
{}

void RovController::updateTunings(const RovCommand& cmd) {
    yawPID.setTunings(cmd.Kp_yaw, cmd.Ki_yaw, cmd.Kd_yaw);
    depthPID.setTunings(cmd.Kp_depth, cmd.Ki_depth, cmd.Kd_depth);
}

void RovController::computeAndMix(const RovCommand& cmd, float current_yaw, float current_depth, float dt, ThrusterControl& thrusters) {
    
    int pwm_outputs[NUM_THRUSTERS];

    // Check if surface computer sent direct manual thruster PWM overrides (> 0)
    bool direct_override = false;
    for (int i = 0; i < NUM_THRUSTERS; i++) {
        if (cmd.thruster_pwm[i] > 0) {
            direct_override = true;
            break;
        }
    }

    if (direct_override) {
        // Use direct manual thrust commands from surface
        for (int i = 0; i < NUM_THRUSTERS; i++) {
            pwm_outputs[i] = cmd.thruster_pwm[i];
        }
    } else {
        // Calculate Control Efforts using PID
        int pid_yaw_out = (int)yawPID.compute(cmd.setpoint_yaw, current_yaw, dt);
        int pid_depth_out = (int)depthPID.compute(cmd.setpoint_depth, current_depth, dt);

        // Control Allocation (Mixer)
        pwm_outputs[0] = 1500 + cmd.manual_surge - cmd.manual_sway + pid_yaw_out;
        pwm_outputs[1] = 1500 + cmd.manual_surge + cmd.manual_sway - pid_yaw_out;
        pwm_outputs[2] = 1500 - cmd.manual_surge - cmd.manual_sway - pid_yaw_out;
        pwm_outputs[3] = 1500 - cmd.manual_surge + cmd.manual_sway + pid_yaw_out;
        pwm_outputs[4] = 1500 + pid_depth_out;
        pwm_outputs[5] = 1500 + pid_depth_out;
    }

    // Write PWM signals using the ThrusterControl object
    for (int i = 0; i < NUM_THRUSTERS; i++) {
        thrusters.set(i, pwm_outputs[i]);
    }
}