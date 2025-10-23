// ============================================================================
// switches.h
// ============================================================================
// Purpose: Input device (button/tilt switch) interface declarations
// Functions: Switch polling, event detection, debouncing, activity counters
// Hardware: GPIO0 (button), GPIO4 (tilt switch) with external pull-ups
// ============================================================================

#pragma once
#include <Arduino.h>

void switchesBegin();
void pollSwitches();
bool takeButtonEvent();
bool takeTiltEvent();

uint32_t node1Count();
uint32_t node2Count();
void incNode1();
void incNode2();