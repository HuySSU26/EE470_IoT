// ============================================================================
// control.cpp - Remote LED/RGB Control Implementation - MEMORY LEAK FIXED
// ============================================================================

#include "control.h"
#include "config.h"
#include "leds.h"
#include "net.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Last known timestamps to detect changes
static String lastLedTimestamp = "";
static String lastRgbData = "";

/**
 * Helper: Remove spaces/newlines and convert to uppercase
 */
static String upperNoSpace(String s) {
  s.replace("\r", "");
  s.replace("\n", "");
  s.replace("\t", " ");
  while (s.indexOf("  ") >= 0) s.replace("  ", " ");
  s.replace(" ", "");
  s.toUpperCase();
  return s;
}

/**
 * Helper: Parse ON/OFF value from JSON
 */
static bool parseOnOff(const String& json, const char* key, bool& ok) {
  ok = false;
  String s = upperNoSpace(json);
  String k = "\"";
  k += key;
  k += "\"";
  k.toUpperCase();

  int pos = s.indexOf(k);
  if (pos < 0) return false;

  pos = s.indexOf(":", pos);
  if (pos < 0) return false;

  int q1 = s.indexOf("\"", pos + 1);
  if (q1 < 0) return false;
  int q2 = s.indexOf("\"", q1 + 1);
  if (q2 < 0) return false;

  String val = s.substring(q1 + 1, q2);
  ok = (val == "ON" || val == "OFF");
  return (val == "ON");
}

/**
 * Helper: Get string value from JSON
 */
static String getJsonString(const String& json, const char* key, bool& ok) {
  ok = false;

  String needle = String("\"") + key + String("\"");
  int pos = json.indexOf(needle);
  if (pos < 0) return "";

  int colon = json.indexOf(':', pos + needle.length());
  if (colon < 0) return "";

  int i = colon + 1;
  while (i < (int)json.length() && isspace(json[i])) i++;

  if (i >= (int)json.length() || json[i] != '"') return "";
  int q1 = i;
  int q2 = json.indexOf('"', q1 + 1);
  if (q2 < 0) return "";

  ok = true;
  return json.substring(q1 + 1, q2);
}

/**
 * Initialize control module
 */
void controlBegin() {
  Serial.println("[CONTROL] Remote control module initialized");
}

/**
 * Poll LED control status from server - MEMORY LEAK FIXED
 */
bool pollLEDControl() {
  if (!ensureWiFi()) {
    Serial.println("[CONTROL] No WiFi - skipping LED poll");
    return false;
  }

  Serial.println("[CONTROL] Polling LED status...");

  // FIX: Create new client each time (not static!)
  WiFiClientSecure* client = new WiFiClientSecure();
  client->setInsecure();

  HTTPClient http;
  http.setTimeout(7000);
  http.setReuse(false);  // Don't reuse connections

  // Add cache-busting parameter
  String url = String(LED_CONTROL_URL) + "?t=" + String(millis());

  if (!http.begin(*client, url)) {
    Serial.println("[CONTROL] LED HTTP begin failed");
    delete client;  // Clean up
    return false;
  }

  int code = http.GET();
  bool changed = false;

  if (code == HTTP_CODE_OK) {
    String body = http.getString();

    // Parse LED states
    bool ok1 = false, ok2 = false;
    bool newLed1 = parseOnOff(body, "led1", ok1);
    bool newLed2 = parseOnOff(body, "led2", ok2);

    if (ok1) {
      bool currentLed1 = getLED(PIN_LED1);
      if (newLed1 != currentLed1) {
        setLED(PIN_LED1, newLed1);
        Serial.print("[CONTROL] LED1 -> ");
        Serial.println(newLed1 ? "ON" : "OFF");
        changed = true;
      }
    }

    if (ok2) {
      bool currentLed2 = getLED(PIN_LED2);
      if (newLed2 != currentLed2) {
        setLED(PIN_LED2, newLed2);
        Serial.print("[CONTROL] LED2 -> ");
        Serial.println(newLed2 ? "ON" : "OFF");
        changed = true;
      }
    }

    // Check timestamp for server-side updates
    bool tsOk = false;
    String ts = getJsonString(body, "timestamp", tsOk);
    if (tsOk && ts.length() && ts != lastLedTimestamp) {
      lastLedTimestamp = ts;
      if (!changed) {
        Serial.println("[CONTROL] LED timestamp updated (no state change)");
      }
    }

  } else {
    Serial.print("[CONTROL] LED HTTP error: ");
    Serial.println(code);
  }

  http.end();
  delete client;  // FIX: Clean up memory!
  
  return changed;
}

/**
 * Poll RGB values from server - Already correct (creates new client)
 */
bool pollRGBControl() {
  if (!ensureWiFi()) {
    Serial.println("[CONTROL] No WiFi - skipping RGB poll");
    return false;
  }

  Serial.println("[CONTROL] Polling RGB values...");

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 512);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setReuse(false);

  String url = String(RGB_CONTROL_URL) + "?t=" + String(millis());

  if (!http.begin(client, url)) {
    Serial.println("[CONTROL] RGB HTTP begin failed");
    return false;
  }

  http.addHeader("Accept", "text/plain");
  http.addHeader("User-Agent", "ESP8266");

  int code = http.GET();
  bool changed = false;

  if (code == HTTP_CODE_OK || code == 200) {
    String body = http.getString();
    body.trim();

    // Check if response is HTML (error page)
    if (body.indexOf("<html") >= 0 || body.indexOf("<!DOCTYPE") >= 0) {
      Serial.println("[CONTROL] RGB got HTML redirect (server issue)");
      http.end();
      return false;
    }

    // Parse RGB values (format: R,G,B)
    if (body != lastRgbData && body.length() > 0 && body.length() < 50) {
      lastRgbData = body;

      int comma1 = body.indexOf(',');
      int comma2 = body.indexOf(',', comma1 + 1);

      if (comma1 > 0 && comma2 > comma1) {
        int newR = body.substring(0, comma1).toInt();
        int newG = body.substring(comma1 + 1, comma2).toInt();
        int newB = body.substring(comma2 + 1).toInt();

        // Constrain values
        newR = constrain(newR, 0, 255);
        newG = constrain(newG, 0, 255);
        newB = constrain(newB, 0, 255);

        // Check if values changed
        int currR, currG, currB;
        getRGBColor(currR, currG, currB);

        if (newR != currR || newG != currG || newB != currB) {
          setRGBColor(newR, newG, newB);
          Serial.print("[CONTROL] RGB updated: R=");
          Serial.print(newR);
          Serial.print(", G=");
          Serial.print(newG);
          Serial.print(", B=");
          Serial.println(newB);
          changed = true;
        }
      } else {
        Serial.print("[CONTROL] Invalid RGB format: ");
        Serial.println(body);
      }
    }

  } else {
    Serial.print("[CONTROL] RGB HTTP error: ");
    Serial.println(code);
  }

  http.end();
  return changed;
}

/**
 * Poll both LED and RGB controls
 */
bool pollAllControls() {
  bool ledChanged = pollLEDControl();
  delay(100);  // Small delay between requests
  bool rgbChanged = pollRGBControl();
  return ledChanged || rgbChanged;
}

/**
 * Get LED status as formatted string
 */
String getLEDStatusString() {
  String status = "LED1:";
  status += getLED(PIN_LED1) ? "ON" : "OFF";
  status += ", LED2:";
  status += getLED(PIN_LED2) ? "ON" : "OFF";
  return status;
}

/**
 * Get RGB status as formatted string
 */
String getRGBStatusString() {
  int r, g, b;
  getRGBColor(r, g, b);
  String status = "RGB(";
  status += String(r);
  status += ",";
  status += String(g);
  status += ",";
  status += String(b);
  status += ")";
  return status;
}
