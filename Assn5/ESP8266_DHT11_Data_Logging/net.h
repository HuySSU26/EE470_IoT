// ============================================================================
// net.h
// ============================================================================
// Purpose: WiFi connection management declarations
// Functions: ensureWiFi() - Connect/reconnect to WiFi
//            isWiFiUp() - Check connection status
// ============================================================================

#pragma once
#include <Arduino.h>

void ensureWiFi();
bool isWiFiUp();