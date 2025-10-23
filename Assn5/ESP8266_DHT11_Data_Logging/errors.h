// ============================================================================
// errors.h
// ============================================================================
// Purpose: Error reporting function declarations
// Provides: Centralized error logging interface for all modules
// Format: [ERR] <location> code=<error_code>
// ============================================================================

#pragma once
#include <Arduino.h>
void reportError(const String& where, int code);