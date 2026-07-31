#include "thruster_control.h"

ThrusterHandler::ThrusterHandler(int low, int high, int neutralMicrosecond) {
    int thruster_pins[NUM_THRUSTERS] = {
        PIN_THRUSTER_FL, PIN_THRUSTER_FR,
        PIN_THRUSTER_ML, PIN_THRUSTER_MR,
        PIN_THRUSTER_BL, PIN_THRUSTER_BR
    };

    Serial.println("[Thruster] Attaching and arming ESCs");

    for(int i = 0; i < NUM_THRUSTERS; i++){
        servosEscArray[i].setPeriodHertz(PWM_FREQ);
        servosEscArray[i].attach(thruster_pins[i], low, high); 
        servosEscArray[i].writeMicroseconds(neutralMicrosecond); // Send neutral signal (1500us) so that the ESC can arm
    }

    Serial.println("[Thruster] ESCs armed");
}


void ThrusterHandler::set(int index, int microseconds) {
    if(index < 0 || index >= NUM_THRUSTERS){
        Serial.printf("[Thruster] Warning: unbounded index (%d)\n", index);
        return;
    }

    // Constrain the PWM value safely
    if(microseconds > 2000){
        Serial.printf("[Thruster] Warning: microsecond too high (%d)\n", microseconds);
        microseconds = 2000;
    } 
    else if(microseconds < 1000){
        Serial.printf("[Thruster] Warning: microsecond too low (%d)\n", microseconds);
        microseconds = 1000;
    }

    servosEscArray[index].writeMicroseconds(microseconds);
}


void ThrusterHandler::setFrontLeft(int microseconds){
    set(0, microseconds);
}

void ThrusterHandler::setFrontRight(int microseconds){
    set(1, microseconds);
}

void ThrusterHandler::setMiddleLeft(int microseconds){
    if(microseconds > ESC_NEUTRAL){
        set(2, ESC_NEUTRAL - (microseconds - ESC_NEUTRAL));
    }
    else{
        set(2, ESC_NEUTRAL + (ESC_NEUTRAL - microseconds));
    }
}

void ThrusterHandler::setMiddleRight(int microseconds){
    if(microseconds > ESC_NEUTRAL){
        set(3, ESC_NEUTRAL - (microseconds - ESC_NEUTRAL));
    }
    else{
        set(3, ESC_NEUTRAL + (ESC_NEUTRAL - microseconds));
    }
}


void ThrusterHandler::setBackLeft(int microseconds){
    if(microseconds > ESC_NEUTRAL){
        set(4, ESC_NEUTRAL - (microseconds - ESC_NEUTRAL));
    }
    else{
        set(4, ESC_NEUTRAL + (ESC_NEUTRAL - microseconds));
    }
}


void ThrusterHandler::setBackRight(int microseconds){
    if(microseconds > ESC_NEUTRAL){
        set(5, ESC_NEUTRAL - (microseconds - ESC_NEUTRAL));
    }
    else{
        set(5, ESC_NEUTRAL + (ESC_NEUTRAL - microseconds));
    }
}


void ThrusterHandler::setAll(int microseconds){
    for(int i = 0; i < NUM_THRUSTERS; i++){
        set(i, microseconds);
    }
}