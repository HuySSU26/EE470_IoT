//----------------------
// File: ledblink.cpp
// Purpose: Implementation of Blink class
// Description: Non-blocking LED blink control using millis() timing
//              Each LED maintains independent state using static arrays
//----------------------

#include "ledblink.h"

// Constructor: Initialize the LED pin
Blink::Blink(int pin) : _pin(pin), _lastToggle(0), _state(LOW) {
    pinMode(_pin, OUTPUT);           // Set pin as output
    digitalWrite(_pin, LOW);         // Start with LED off
}

// Method: Control LED blinking at specified rate
void Blink::blinkRate(int rate) {
    unsigned long currentTime = millis(); // Get current time
    // Check if enough time has elapsed since last toggle
    if (currentTime - _lastToggle >= (unsigned long)rate) {
        _state = !_state;    		   // Toggle LED state
        digitalWrite(_pin, _state ? HIGH : LOW);
        _lastToggle = currentTime;    // Update last toggle time
    }
}