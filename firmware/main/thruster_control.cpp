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
    if(microseconds > ESC_HIGH){
        Serial.printf("[Thruster] Warning: microsecond too high (%d)\n", microseconds);
        microseconds = ESC_HIGH;
    } 
    else if(microseconds < ESC_LOW){
        Serial.printf("[Thruster] Warning: microsecond too low (%d)\n", microseconds);
        microseconds = ESC_LOW;
    }

    servosEscArray[index].writeMicroseconds(microseconds);
}


// Handle the case of inverted esc hardware at compile time

void ThrusterHandler::setFrontLeft(int microseconds){
#ifdef THRUSTER_FL_INV
    set(0, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(0, microseconds);
#endif
}

void ThrusterHandler::setFrontRight(int microseconds){
#ifdef THRUSTER_FR_INV
    set(1, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(1, microseconds);
#endif
}

void ThrusterHandler::setMiddleLeft(int microseconds){
#ifdef THRUSTER_ML_INV
    set(2, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(2, microseconds);
#endif
}

void ThrusterHandler::setMiddleRight(int microseconds){
#ifdef THRUSTER_MR_INV
    set(3, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(3, microseconds);
#endif
}

void ThrusterHandler::setBackLeft(int microseconds){
#ifdef THRUSTER_BL_INV
    set(4, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(4, microseconds);
#endif
}

void ThrusterHandler::setBackRight(int microseconds){
#ifdef THRUSTER_BR_INV
    set(5, (2 * ESC_NEUTRAL) - microseconds);
#else
    set(5, microseconds);
#endif
}


void ThrusterHandler::setAll(int microseconds){
    this->setFrontLeft(microseconds);
    this->setFrontRight(microseconds);
    this->setMiddleLeft(microseconds);
    this->setMiddleRight(microseconds);
    this->setBackLeft(microseconds);
    this->setBackRight(microseconds);
}