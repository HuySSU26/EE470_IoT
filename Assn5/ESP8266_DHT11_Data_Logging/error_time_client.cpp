// ===============================================================
// ORIGINAL src/time_client.cpp - USING TIMEAPI.IO WEB SERVICE
// ===============================================================

#include "time_client.h"
#include "config.h"
#include "errors.h"
#include "net.h"

#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

static String tz = "UTC";

static void loadTZ() {
  EEPROM.begin(TZ_EEPROM_SIZE);
  char buf[TZ_EEPROM_SIZE] = {0};
  for (int i = 0; i < TZ_EEPROM_SIZE - 1; i++) {
    buf[i] = EEPROM.read(TZ_EEPROM_ADDR + i);
  }
  if (buf[0] != (char)0xFF && strlen(buf) > 0) {
    tz = String(buf);
  }
}

static void saveTZ(const String& s) {
  int n = min((int)s.length(), TZ_EEPROM_SIZE - 1);
  for (int i = 0; i < TZ_EEPROM_SIZE; i++) {
    EEPROM.write(TZ_EEPROM_ADDR + i, (i < n) ? s[i] : 0);
  }
  EEPROM.commit();
}

void timeClientBegin() {
  loadTZ();
  Serial.print("Time zone: ");
  Serial.println(tz);
}

String getTimezone() {
  return tz;
}

bool setTimezone(const String& v) {
  if (v.length() < 3 || v.length() >= TZ_EEPROM_SIZE - 1) {
    return false;
  }
  tz = v;
  saveTZ(tz);
  Serial.print("Saved TZ: ");
  Serial.println(tz);
  return true;
}

// THIS IS THE PROBLEMATIC FUNCTION THAT USED TIMEAPI.IO
bool readTimeISO(String& iso8601) {
  ensureWiFi();
  if (!isWiFiUp()) {
    reportError("time/no_wifi", -1);
    return false;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();

  HTTPClient http;
  
  // ⚠️ THIS LINE USES TIMEAPI.IO WEB SERVICE
  String url = "https://timeapi.io/api/Time/current/zone?timeZone=" + tz;
  
  if (!http.begin(*client, url)) {
    reportError("time/begin", -1);
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    reportError("time/GET", code);
    http.end();
    return false;
  }

  // Get the response as a String first
  String payload = http.getString();
  http.end();

  // Debug output
  Serial.print("[DEBUG] Response length: ");
  Serial.println(payload.length());
  Serial.print("[DEBUG] First 200 chars: ");
  Serial.println(payload.substring(0, 200));

  // Parse JSON from String - TIMEAPI.IO RETURNS JSON
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("[DEBUG] JSON parse error: ");
    Serial.println(err.c_str());
    reportError("time/json", -2);
    return false;
  }

  // ⚠️ TIMEAPI.IO USES "dateTime" FIELD (capital T)
  const char* iso = doc["dateTime"];
  if (!iso) {
    Serial.println("[DEBUG] dateTime field not found in JSON");
    reportError("time/no_dateTime", -3);
    return false;
  }

  iso8601 = String(iso);
  Serial.print("[TIME] ");
  Serial.println(iso8601);
  return true;
}



// ===============================================================
// ATTEMPTED ALTERNATIVE - USING WORLDTIMEAPI.ORG
// ===============================================================

bool readTimeISO(String& iso8601) {
  ensureWiFi();
  if (!isWiFiUp()) {
    reportError("time/no_wifi", -1);
    return false;
  }

  WiFiClient client;  // Plain HTTP - much more reliable than HTTPS
  HTTPClient http;
  http.setTimeout(15000);  // 15 second timeout
  
  // ⚠️ THIS USES WORLDTIMEAPI.ORG WEB SERVICE
  String url = "http://worldtimeapi.org/api/timezone/" + tz;
  
  if (!http.begin(client, url)) {
    reportError("time/begin", -1);
    return false;
  }

  int code = http.GET();
  
  if (code != HTTP_CODE_OK) {
    reportError("time/GET", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    reportError("time/json", -2);
    return false;
  }

  // ⚠️ WORLDTIMEAPI.ORG USES "datetime" FIELD (lowercase)
  const char* iso = doc["datetime"];
  if (!iso) {
    reportError("time/no_datetime", -3);
    return false;
  }

  iso8601 = String(iso);
  Serial.print("[TIME] ✓ ");
  Serial.println(iso8601);
  return true;
}
```

---



/* **Error Messages in Serial Monitor:**
```
[ERR] time/GET code=-5      ← HTTP connection timeout
[ERR] time/json code=-2     ← JSON parsing failed
[TIME] HTTP error code: -5  ← Connection issues

-------------------------------------------------*/