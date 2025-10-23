// ============================================================================
// time_client.cpp
// ============================================================================
// Purpose: Network Time Protocol (NTP) client implementation
// Features: Pacific Time (PST/PDT) with DST support, lazy NTP initialization
// Servers: pool.ntp.org, time.nist.gov, time.google.com
// Output: ISO 8601 format timestamps (e.g., 2025-10-17T22:30:45-07:00)
// ============================================================================

#include "time_client.h"
#include "config.h"
#include "errors.h"
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
}

void timeClientBegin() {
  loadTZ();
  Serial.print("Time zone: ");
  Serial.print(tz);
  Serial.print(" (");
  Serial.print(tzPosix);
  Serial.println(")");
  // Don't configure NTP here - wait until WiFi is connected
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
  
  if (v == "America/Los_Angeles" || v == "America/San_Francisco") {
    tzPosix = "PST8PDT,M3.2.0,M11.1.0";
  } else if (v == "America/New_York") {
    tzPosix = "EST5EDT,M3.2.0,M11.1.0";
  } else if (v == "America/Chicago") {
    tzPosix = "CST6CDT,M3.2.0,M11.1.0";
  } else if (v == "America/Denver") {
    tzPosix = "MST7MDT,M3.2.0,M11.1.0";
  } else if (v == "UTC") {
    tzPosix = "UTC0";
  } else {
    tzPosix = "PST8PDT,M3.2.0,M11.1.0";
  }
  
  configTime(tzPosix, "pool.ntp.org", "time.nist.gov");
  ntpConfigured = true;
  
  Serial.print("Saved TZ: ");
  Serial.print(tz);
  Serial.print(" -> ");
  Serial.println(tzPosix);
  
  return true;
}

bool readTimeISO(String& iso8601) {
  // Ensure WiFi is connected
  ensureWiFi();
  if (!isWiFiUp()) {
    reportError("time/no_wifi", -1);
    return false;
  }
  
  // Configure NTP on first use (after WiFi is connected)
  if (!ntpConfigured) {
    Serial.println("[TIME] Configuring NTP...");
    configTime(tzPosix, "pool.ntp.org", "time.nist.gov", "time.google.com");
    ntpConfigured = true;
    
    // Wait for initial sync
    Serial.print("[TIME] Syncing");
    for (int i = 0; i < 30; i++) {
      delay(500);
      Serial.print(".");
      time_t now = time(nullptr);
      if (now > 1000000000) {
        Serial.println(" ✓");
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);
        Serial.print("[TIME] Synced: ");
        Serial.println(buffer);
        break;
      }
    }
  }
  
  time_t now = time(nullptr);
  
  // Check if time is synced
  if (now < 946684800) {
    Serial.println("[TIME] Not synced yet, waiting...");
    for (int i = 0; i < 20; i++) {
      delay(500);
      now = time(nullptr);
      if (now > 946684800) {
        break;
      }
    }
    
    if (now < 946684800) {
      reportError("time/ntp_sync", -1);
      return false;
    }
  }
  
  // Get local time
  struct tm timeinfo;
  if (!localtime_r(&now, &timeinfo)) {
    reportError("time/localtime", -2);
    return false;
  }
  
  // Format as ISO 8601
  char buffer[35];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  
  // Get timezone offset
  char tzOffset[10];
  strftime(tzOffset, sizeof(tzOffset), "%z", &timeinfo);
  
  // Format offset from -0700 to -07:00
  if (strlen(tzOffset) == 5) {
    char formatted[10];
    snprintf(formatted, sizeof(formatted), "%c%c%c:%c%c", 
             tzOffset[0], tzOffset[1], tzOffset[2], tzOffset[3], tzOffset[4]);
    iso8601 = String(buffer) + String(formatted);
  } else {
    iso8601 = String(buffer) + String(tzOffset);
  }
  
  Serial.print("[TIME] ✓ ");
  Serial.println(iso8601);
  
  return true;
}