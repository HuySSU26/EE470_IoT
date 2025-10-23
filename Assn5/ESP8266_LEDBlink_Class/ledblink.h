//----------------------
// File: ledblink.h
// Purpose: Header file for Blink class
// Description: Allows independent LED blinking on different GPIO pins
//              with individual timing control using non-blocking approach
//----------------------

#ifndef LEDBLINK_H
#define LEDBLINK_H

#include <Arduino.h>

class Blink {
public:
    // Constructor: Initialize LED on specified pin
    Blink(int pin);
    
    // Method: Set and execute blinking at specified rate (milliseconds)
    void blinkRate(int rate);

private:
    int _pin;		// GPIO pin number for this LED
    unsigned long _lastToggle;
    uint8_t _state;    
};

#endif // LEDBLINK_H