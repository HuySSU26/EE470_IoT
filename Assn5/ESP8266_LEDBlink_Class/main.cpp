// Title: ESP8266_Dual_LED_Blink_Class_Demo
//----------------------
// Program Detail:
//----------------------
// Purpose: Demonstrates the Blink class by controlling two LEDs
//          at different blinking rates independently
// Inputs: None (autonomous blinking)
// Outputs: Two LEDs blinking at different rates
//          - LED1 on GPIO12 (D6): 1000ms rate (slow)
//          - LED2 on GPIO13 (D7): 250ms rate (fast)
// Date: September 26, 2025
// Compiler: PlatformIO with Visual Studio Code
// Author: Huy Nguyen
// Version: 1.0
//
// Hardware Configuration:
// - ESP8266 NodeMCU
// - LED1: GPIO12 (D6) -> 270Ω resistor -> LED -> GND
// - LED2: GPIO13 (D7) -> 270Ω resistor -> LED -> GND
//----------------------

#include <Arduino.h>
#include "ledblink.h"

// Create two Blink objects with different GPIO pins
Blink LED(12);     // First LED connected to GPIO12 (D6 on NodeMCU)
Blink LED2(13);    // Second LED connected to GPIO13 (D7 on NodeMCU)

void setup() {
    Serial.begin(9600);
    delay(100);
}

void loop() {
    // Call blinkRate() for each LED object
    // Each maintains its own independent timing
    LED.blinkRate(1000);      // slow blink: 1000ms (1 Hz)
    LED2.blinkRate(250);     // fast blink: 250ms (4 Hz)

    delay(10); // Small delay for loop stability 
}
