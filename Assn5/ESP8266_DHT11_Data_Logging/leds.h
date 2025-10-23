// ============================================================================
// leds.h
// ============================================================================
// Purpose: LED visual feedback interface declarations
// Functions: Non-blocking LED blink control for transmission confirmation
// Hardware: GPIO12 (LED1), GPIO13 (LED2)
// ============================================================================

#pragma once
#include <Arduino.h>

void ledsBegin();
void ledsPoll();
void blinkAsync(uint8_t pin, uint16_t onTime, uint16_t totalTime);