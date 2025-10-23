// ============================================================================
// net.cpp
// ============================================================================
// Purpose: WiFi connection management implementation
// Features: Automatic connection with 15-second timeout, reconnection support
// Used by: time_client.cpp and tx.cpp before network operations
// ============================================================================

#include "net.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include "errors.h"

bool isWiFiUp() {
  return WiFi.status() == WL_CONNECTED;
}

void ensureWiFi() {
  if (isWiFiUp()) return;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  
  uint32_t start = millis();
  while (!isWiFiUp() && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  
  if (isWiFiUp()) {
    Serial.print("WiFi OK: ");
    Serial.println(WiFi.localIP());
  } else {
    reportError("wifi/connect", -100);
  }
}