// ============================================================================
// switches.h - Dual Switch Input Handler
// ============================================================================
// Purpose: Handle two independent switches with debouncing
// Switch 1 (GPIO0):  Triggers Part 1 - Sensor logging to Slack/Google Sheets
// Switch 2 (GPIO16): Triggers Part 2 - LED/RGB status check and messaging
// ============================================================================

#pragma once
#include <Arduino.h>

/**
 * Initialize switch GPIO pins as inputs with pull-ups
 */
void switchesBegin();

/**
 * Poll both switches for state changes (call every loop iteration)
 * Handles debouncing internally (50ms)
 */
void pollSwitches();

/**
 * Check and consume switch 1 event
 * Returns true if switch 1 was pressed since last check
 */
bool takeSwitch1Event();

/**
 * Check and consume switch 2 event
 * Returns true if switch 2 was pressed since last check
 */
bool takeSwitch2Event();

/**
 * Get activity counters (number of times each switch was pressed)
 */
uint32_t switch1Count();
uint32_t switch2Count();

/**
 * Increment activity counters (called after successful actions)
 */
void incSwitch1();
void incSwitch2();
