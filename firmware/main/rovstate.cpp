#include "rovstate.h"


// The constructor
Rovstate::RovState() {
    rintegral = 0.0f;
    pintegral = 0.0f;
    yintegral = 0.0f;
    
    current.time = 0.0f;
    previous.time = 0.0f;
} 

ROCState::update() {
    previous = current; 
    
    current.orientation = sensor.getRotation(); 
    current.linear = sensor.getLinearAcceleration();
        
    current.time = millis() / 1000.0f; 
    }
