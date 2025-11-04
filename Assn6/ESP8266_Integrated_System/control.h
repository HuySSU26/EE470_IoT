// ============================================================================
// control.h - Remote LED/RGB Control Interface
// ============================================================================
// Purpose: Poll and apply LED/RGB states from web server (Part 2A/2B)
// Features: HTTPS polling, JSON parsing, state management
// ============================================================================

#pragma once
#include <Arduino.h>

/**
 * Initialize control module
 */
void controlBegin();

/**
 * Poll LED control status from server and update local LEDs
 * Returns true if states were updated
 */
bool pollLEDControl();

/**
 * Poll RGB values from server and update local RGB LED
 * Returns true if values were updated
 */
bool pollRGBControl();

/**
 * Poll both LED and RGB control
 * Returns true if any state changed
 */
bool pollAllControls();

/**
 * Get current LED states as string for messaging
 * Returns formatted string like "LED1:ON, LED2:OFF"
 */
String getLEDStatusString();

/**
 * Get current RGB values as string for messaging
 * Returns formatted string like "RGB(255,128,0)"
 */
String getRGBStatusString();
