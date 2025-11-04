// ============================================================================
// net.cpp
// ============================================================================
// Purpose: WiFi connection management implementation
// Features: Automatic connection with 15-second timeout, reconnection support
// Used by: control.cpp, messaging.cpp, time_client.cpp, tx.cpp
// ============================================================================

#include "net.h"
#include "config.h"
#include <ESP8266WiFi.h>

/**
 * Check if WiFi is currently connected
 */
bool isWiFiUp() {
  return WiFi.status() == WL_CONNECTED;
}

/**
 * Ensure WiFi connection is established
 * Returns true if connected, false if failed
 */
bool ensureWiFi() {
  // Already connected
  if (isWiFiUp()) {
    return true;
  }
  
  // Attempt connection
  Serial.print("[WiFi] Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.print("...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  uint32_t start = millis();
  while (!isWiFiUp() && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  
  if (isWiFiUp()) {
    Serial.print("[WiFi] Connected! IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("[WiFi] Connection failed (timeout 15s)");
    return false;
  }
}
