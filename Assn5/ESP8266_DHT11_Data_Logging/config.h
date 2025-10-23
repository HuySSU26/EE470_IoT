// ============================================================================
// config.h
// ============================================================================
// Purpose: Central configuration file for ESP8266 Sensor Logger
// Contains: WiFi credentials, GPIO pin assignments, sensor settings,
//           timezone EEPROM configuration, and backend database URL
// Used by: All modules requiring hardware pins or system constants
// ============================================================================

#pragma once
#include <Arduino.h>

// ==== Wi-Fi ====
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// ==== Pin assignments ====
#define PIN_BUTTON  0    // GPIO0 (D3) - push button (external pull-up)
#define PIN_TILT    4    // GPIO4 (D2) - tilt switch (external pull-up)
#define PIN_DHT    14    // GPIO14 (D5)- DHT11 data pin

// Output indicators
#define PIN_LED1   12    // GPIO12 (D6)- LED1 (blinks on button send)
#define PIN_LED2   13    // GPIO13 (D7)- LED2 (blinks on tilt send)

// ==== DHT ====
#define DHTTYPE     11   // DHT11

// ==== Time / EEPROM ====
#define TZ_EEPROM_ADDR  0
#define TZ_EEPROM_SIZE  64

// ==== Backend ====
#define DB_BASE_URL "https://huynguyen.co/Chartjs/sensor_dashboard.php"  // Actual endpoint
