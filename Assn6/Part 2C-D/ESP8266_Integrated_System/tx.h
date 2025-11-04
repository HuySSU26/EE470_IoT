// ============================================================================
// tx.h
// ============================================================================
// Purpose: Data transmission interface declarations
// Function: transmit() - Send sensor data to backend via HTTPS
// Protocol: JSON payload over HTTPS POST request
// ============================================================================

#pragma once
#include <Arduino.h>

bool transmit(uint8_t node, const String& iso8601, float tC, float h,
              uint32_t activityCount);