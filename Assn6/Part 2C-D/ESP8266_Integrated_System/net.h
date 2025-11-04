// ============================================================================
// net.h
// ============================================================================
// Purpose: WiFi connection management declarations
// Functions: ensureWiFi() - Connect/reconnect to WiFi (returns success status)
//            isWiFiUp() - Check connection status
// ============================================================================

#pragma once
#include <Arduino.h>

/**
 * Ensure WiFi is connected, attempt connection if needed
 * Returns true if WiFi connected successfully, false otherwise
 */
bool ensureWiFi();

/**
 * Check if WiFi is currently connected
 * Returns true if connected, false otherwise
 */
bool isWiFiUp();
