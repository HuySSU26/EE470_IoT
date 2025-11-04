// ============================================================================
// config.h - Integrated System Configuration
// ============================================================================
// Purpose: Central configuration for ESP8266 Integrated Control System
// Features: Part 1 (Sensor Logging), Part 2A (LED Control), Part 2B (RGB Control)
// ============================================================================

#pragma once
#include <Arduino.h>

// ==== Wi-Fi Configuration ====
#define WIFI_SSID "TP-Link_C6AF"
#define WIFI_PASS "Mistygiant753"

// ==== GPIO Pin Assignments ====
// Input Switches
#define PIN_SWITCH_1    0    // GPIO0  (D3) - Switch 1: Controls Part 1 (Sensor logging to Slack/Sheets)
#define PIN_SWITCH_2   16    // GPIO16 (D0) - Switch 2: Controls Part 2A/2B (LED/RGB status check)

// Sensor Input
#define PIN_DHT        14    // GPIO14 (D5) - DHT11 temperature/humidity sensor

// Digital LED Outputs (Part 2A)
#define PIN_LED1       12    // GPIO12 (D6) - LED1 for visual feedback
#define PIN_LED2       13    // GPIO13 (D7) - LED2 for visual feedback

// RGB LED Outputs (Part 2B) - PWM capable pins
#define RGB_RED_PIN    15    // GPIO15 (D8) - RGB Red channel
#define RGB_GREEN_PIN   4    // GPIO4  (D2) - RGB Green channel
#define RGB_BLUE_PIN    5    // GPIO5  (D1) - RGB Blue channel

// ==== DHT Sensor ====
#define DHTTYPE        11    // DHT11 sensor type

// ==== Time / EEPROM ====
#define TZ_EEPROM_ADDR  0
#define TZ_EEPROM_SIZE  64

// ==== Backend URLs ====
// Part 1: Sensor data logging
#define DB_BASE_URL "https://huynguyen.co/Chartjs/sensor_dashboard.php"

// Part 2A & 2B: LED and RGB control
#define LED_CONTROL_URL "https://huynguyen.co/led_control.php"
#define RGB_CONTROL_URL "https://huynguyen.co/rgb_proxy.php"

// ==== Timing Constants ====
#define DEBOUNCE_DELAY_MS  50    // Switch debounce time
#define LED_BLINK_DURATION 2000  // Visual feedback duration (ms)

// ==== Message Buffer Settings ====
#define MAX_MESSAGE_QUEUE  10    // Maximum queued messages for transmission
