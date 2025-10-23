// ============================================================================
// time_client.h
// ============================================================================
// Purpose: NTP time synchronization interface declarations
// Functions: NTP configuration, ISO 8601 timestamp retrieval, timezone mgmt
// Features: Lazy initialization, EEPROM timezone storage
// ============================================================================

#pragma once
#include <Arduino.h>

void timeClientBegin();
bool readTimeISO(String& iso8601);
String getTimezone();
bool setTimezone(const String& tz);