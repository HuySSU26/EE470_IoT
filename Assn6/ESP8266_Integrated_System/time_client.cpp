// ============================================================================
// time_client.cpp
// ============================================================================
// Purpose: Network Time Protocol (NTP) client implementation
// Features: Timezone support with DST, lazy NTP initialization, EEPROM persistence
// Servers: pool.ntp.org, time.nist.gov, time.google.com
// Output: ISO 8601 format timestamps (e.g., 2025-10-17T22:30:45-07:00)
// ============================================================================

#include "time_client.h"
#include "config.h"
#include "net.h"

#include <EEPROM.h>
#include <time.h>

static String tz = "America/Los_Angeles";
static const char* tzPosix = "PST8PDT,M3.2.0,M11.1.0";
static bool ntpConfigured = false;

static void loadTZ() {
  EEPROM.begin(TZ_EEPROM_SIZE);
  char buf[TZ_EEPROM_SIZE] = {0};
  for (int i = 0; i < TZ_EEPROM_SIZE - 1; i++) {
    buf[i] = EEPROM.read(TZ_EEPROM_ADDR + i);
  }
  if (buf[0] != (char)0xFF && strlen(buf) > 0) {
    tz = String(buf);
    if (tz == "America/Los_Angeles" || tz == "America/San_Francisco") {
      tzPosix = "PST8PDT,M3.2.0,M11.1.0";
    } else if (tz == "America/New_York") {
      tzPosix = "EST5EDT,M3.2.0,M11.1.0";
    } else if (tz == "America/Chicago") {
      tzPosix = "CST6CDT,M3.2.0,M11.1.0";
    } else if (tz == "America/Denver") {
      tzPosix = "MST7MDT,M3.2.0,M11.1.0";
    } else if (tz == "UTC") {
      tzPosix = "UTC0";
    } else {
      tzPosix = "PST8PDT,M3.2.0,M11.1.0";
    }
  }
}

static void saveTZ(const String& s) {
  int n = min((int)s.length(), TZ_EEPROM_SIZE - 1);
  for (int i = 0; i < TZ_EEPROM_SIZE; i++) {
    EEPROM.write(TZ_EEPROM_ADDR + i, (i < n) ? s[i] : 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

void timeClientBegin() {
  loadTZ();
  Serial.print("[TIME] Timezone loaded: ");
  Serial.println(tz);
}

bool setTimezone(const String& ianaString) {
  if (ianaString.length() == 0 || ianaString.length() >= TZ_EEPROM_SIZE) {
    return false;
  }
  
  tz = ianaString;
  
  if (tz == "America/Los_Angeles" || tz == "America/San_Francisco") {
    tzPosix = "PST8PDT,M3.2.0,M11.1.0";
  } else if (tz == "America/New_York") {
    tzPosix = "EST5EDT,M3.2.0,M11.1.0";
  } else if (tz == "America/Chicago") {
    tzPosix = "CST6CDT,M3.2.0,M11.1.0";
  } else if (tz == "America/Denver") {
    tzPosix = "MST7MDT,M3.2.0,M11.1.0";
  } else if (tz == "UTC") {
    tzPosix = "UTC0";
  } else {
    tzPosix = "PST8PDT,M3.2.0,M11.1.0";
  }
  
  saveTZ(tz);
  ntpConfigured = false;
  
  return true;
}

String getTimezone() {
  return tz;
}

static bool syncNTP() {
  if (!ensureWiFi()) {
    Serial.println("[TIME] Error: No WiFi for NTP sync (-1)");
    return false;
  }
  
  Serial.print("[TIME] Syncing NTP (TZ: ");
  Serial.print(tz);
  Serial.println(")...");
  
  configTime(tzPosix, "pool.ntp.org", "time.nist.gov", "time.google.com");
  
  time_t now = time(nullptr);
  uint32_t start = millis();
  while (now < 1000000000 && millis() - start < 30000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  
  if (now >= 1000000000) {
    Serial.print("[TIME] NTP sync OK: ");
    Serial.println(ctime(&now));
    ntpConfigured = true;
    return true;
  }
  
  Serial.println("[TIME] Error: NTP sync timeout (-1)");
  return false;
}

bool readTimeISO(String& out) {
  if (!ntpConfigured) {
    if (!syncNTP()) {
      return false;
    }
  }
  
  time_t now = time(nullptr);
  if (now < 1000000000) {
    Serial.println("[TIME] Error: NTP not synced (-1)");
    return false;
  }
  
  struct tm timeinfo;
  if (!localtime_r(&now, &timeinfo)) {
    Serial.println("[TIME] Error: localtime_r failed (-2)");
    return false;
  }
  
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
  out = String(buf);
  
  if (out.length() > 19) {
    String offset = out.substring(19);
    if (offset.length() == 5) {
      offset = offset.substring(0, 3) + ":" + offset.substring(3);
      out = out.substring(0, 19) + offset;
    }
  }
  
  return true;
}
