// ============================================================================
// leds.h - LED and RGB Control Interface
// ============================================================================
// Purpose: Control digital LEDs (Part 2A) and RGB LED (Part 2B)
// Features: Non-blocking blink, PWM RGB control, status polling
// ============================================================================

#pragma once
#include <Arduino.h>

/**
 * Initialize all LED GPIO pins (digital and RGB)
 */
void ledsBegin();

/**
 * Poll LED states for non-blocking blink timing
 * Call every loop iteration
 */
void ledsPoll();

/**
 * Start a non-blocking blink sequence on specified LED
 * @param pin LED pin number (PIN_LED1 or PIN_LED2)
 * @param duration_ms Total duration of blink (milliseconds)
 * @param blink_ms On/off period for blinking (milliseconds)
 */
void blinkAsync(uint8_t pin, uint16_t blink_ms, uint16_t duration_ms);

/**
 * Set RGB LED color
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 */
void setRGBColor(int r, int g, int b);

/**
 * Get current RGB LED values
 */
void getRGBColor(int& r, int& g, int& b);

/**
 * Set digital LED state
 * @param pin LED pin (PIN_LED1 or PIN_LED2)
 * @param state true=ON, false=OFF
 */
void setLED(uint8_t pin, bool state);

/**
 * Get digital LED state
 * @param pin LED pin (PIN_LED1 or PIN_LED2)
 * @return true if LED is ON, false if OFF
 */
bool getLED(uint8_t pin);
