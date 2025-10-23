// ============================================================================
// errors.cpp
// ============================================================================
// Purpose: Implementation of error reporting system
// Function: reportError() - Prints formatted error messages to Serial Monitor
// Used by: All modules when errors occur (WiFi, NTP, sensors, transmission)
// ============================================================================

#include "errors.h"

void reportError(const String& where, int code) {
  Serial.print("[ERR] ");
  Serial.print(where);
  Serial.print(" code=");
  Serial.println(code);
}