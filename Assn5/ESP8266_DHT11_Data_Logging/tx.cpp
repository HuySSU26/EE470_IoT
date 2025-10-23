// ============================================================================
// tx.cpp
// ============================================================================
// Purpose: HTTPS transmission to MySQL database via PHP endpoint
// Features: JSON serialization, URL encoding, duplicate detection (FNV-1a)
// Security: BearSSL (insecure mode), automatic WiFi reconnection
// Endpoint: https://huynguyen.co/Chartjs/sensor_dashboard.php
// ============================================================================

#include "tx.h"
#include "config.h"
#include "errors.h"
#include "net.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

// Simple URL encoding for timestamp
static String urlEncode(const String& str) {
  String encoded = "";
  char c;
  for (size_t i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      char hex[3];
      sprintf(hex, "%02X", c);
      encoded += hex;
    }
  }
  return encoded;
}

// Hash function to detect duplicate transmissions
static uint32_t simpleHash(const String& s) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h;
}

bool transmit(uint8_t node, const String& iso, float tC, float h,
              uint32_t activityCount) {
  ensureWiFi();
  if (!isWiFiUp()) {
    reportError("tx/no_wifi", -20);
    return false;
  }

  // Build JSON payload
  JsonDocument body;
  body["node"] = node;
  body["temperature_C"] = tC;
  body["humidity_pct"] = h;
  body["timestamp"] = iso;
  body["activity_count"] = activityCount;

  String payload;
  serializeJson(body, payload);

  // Check for duplicate transmission
  static uint32_t lastHash[3] = {0, 0, 0};
  uint32_t hsh = simpleHash(payload);
  if (node < 3 && lastHash[node] == hsh) {
    Serial.println("[TX] Duplicate payload -> skipped");
    return false;
  }

  // Send HTTPS POST
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();

  HTTPClient http;
  String url = String(DB_BASE_URL) + "?ts=" + urlEncode(iso) + "&node=" + String(node);
  
  if (!http.begin(*client, url)) {
    reportError("tx/begin", -21);
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String response = http.getString();
  http.end();

  if (code == HTTP_CODE_OK || code == HTTP_CODE_ACCEPTED || code == HTTP_CODE_CREATED) {
    Serial.print("[TX] Success: ");
    Serial.println(code);
    if (node < 3) lastHash[node] = hsh;
    return true;
  }

  reportError("tx/POST", code);
  Serial.print("[TX] Response: ");
  Serial.println(response);
  return false;
}
