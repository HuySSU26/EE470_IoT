/*
 * ============================================================================
 * ESP8266 Integrated Control System
 * ============================================================================
 * Project: Combined sensor logging and LED/RGB control system
 * Author: Huy Nguyen
 * Date: November 4 2025
 * Version: 3.8
 * 
 * Description:
 *   Integrates three major functionalities:
 *   - Part 1: Environmental sensor logging with Slack/SMS notifications
 *   - Part 2A: Remote LED control (ON/OFF status)
 *   - Part 2B: Remote RGB LED control (color values)
 * 
 * Hardware:
 *   Inputs:
 *     - Switch 1 (GPIO0):  Triggers sensor logging to Google Sheets + IFTTT Webhook Notification
 *     - Switch 2 (GPIO16): Triggers LED/RGB status check + notifications
 *     - DHT11 (GPIO14):    Temperature and humidity sensor
 *   
 *   Outputs:
 *     - LED1 (GPIO12):     Visual feedback + remote control
 *     - LED2 (GPIO13):     Visual feedback + remote control
 *     - RGB LED:           Red (GPIO15), Green (GPIO4), Blue (GPIO5)
 * 
 * Operation:
 *   Switch 1 Press:
 *     1. Read DHT11 sensor (temperature, humidity)
 *     2. Get NTP timestamp
 *     3. Send data to Google Sheets via PHP endpoint
 *     4. Send notification to Slack/SMS
 *     5. Blink LED1 for visual confirmation
 *   
 *   Switch 2 Press:
 *     1. Poll LED control status from server
 *     2. Poll RGB values from server
 *     3. Update local LED/RGB states
 *     4. Send status notification to Slack/SMS
 *     5. Blink LED2 for visual confirmation
 * 
 * Features:
 *   - Non-blocking event-driven architecture
 *   - Message buffering and retry logic
 *   - Simultaneous switch handling (both switches can be pressed rapidly)
 *   - Battery operation capable (can run independently)
 *   - Visual LED feedback for all operations
 *   - Robust error handling and recovery
 * 
 * Dependencies:
 *   - ESP8266 Arduino Core (WiFi, HTTPClient)
 *   - Adafruit DHT sensor library
 *   - ArduinoJson
 *   - NTP time synchronization
 *
 * Based on working RGB_main.cpp and actual sensor_dashboard.php
 * 
 * Data Format Fixed:
 *   - Uses temperature_C and humidity_pct (not temperature/humidity)
 *   - IFTTT webhook working (SMS + Slack notifications)
 *   - Proper HTTP client setup matching RGB_main.cpp
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "switches.h"
#include "sensors.h"
#include "time_client.h"
#include "leds.h"
#include "control.h"
#include "net.h"

// ============================================================================
// CONFIGURATION
// ============================================================================
const char* SENSOR_DASHBOARD_URL = "https://huynguyen.co/Chartjs/sensor_dashboard.php";
const char* IFTTT_WEBHOOK_KEY = "WEBBHOOK_KEY";    // Replace with  actual key
const char* IFTTT_EVENT_NAME = "sensor_alert";

// ============================================================================
// Helper: Ensure WiFi Connection
// ============================================================================
static void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("[WiFi] Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] ✓ Connected - IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] ✗ Failed to connect");
  }
}

// ============================================================================
// Transmit to Database - EXACT FORMAT FROM sensor_dashboard.php
// ============================================================================
bool transmitToDatabase(const String& timestamp, float temp, float humidity, uint32_t count) {
  ensureWiFiConnected();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TX] ERROR: No WiFi");
    return false;
  }

  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║        TRANSMITTING TO DATABASE                ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  // Build JSON payload - EXACT format that sensor_dashboard.php expects
  Serial.println("[TX] Building JSON payload...");
  
  StaticJsonDocument<512> doc;
  doc["node"] = 1;  // Integer, not string
  doc["temperature_C"] = temp;  // MUST be temperature_C
  doc["humidity_pct"] = humidity;  // MUST be humidity_pct
  doc["timestamp"] = timestamp;
  doc["activity_count"] = count;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.print("[TX] Payload: ");
  Serial.println(jsonPayload);

  // Use WiFiClientSecure - same as RGB_main.cpp
  static WiFiClientSecure client;
  client.setInsecure();  // Skip certificate validation

  HTTPClient https;
  https.setTimeout(15000);
  https.setReuse(true);  // Reuse connection
  
  Serial.println("[TX] Connecting to server...");
  if (!https.begin(client, SENSOR_DASHBOARD_URL)) {
    Serial.println("[TX] ERROR: HTTPS begin failed");
    return false;
  }
  
  // Set headers
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Accept", "application/json");
  
  Serial.println("[TX] Sending POST request...");
  int httpCode = https.POST(jsonPayload);
  
  Serial.print("[TX] HTTP Code: ");
  Serial.println(httpCode);
  
  String response = "";
  if (httpCode > 0) {
    response = https.getString();
    Serial.print("[TX] Response: ");
    Serial.println(response);
  } else {
    Serial.print("[TX] Error: ");
    Serial.println(https.errorToString(httpCode));
  }

  https.end();
  
  bool success = (httpCode == 200 || httpCode == HTTP_CODE_OK);
  
  if (success) {
    Serial.println("╔════════════════════════════════════════════════╗");
    Serial.println("║          ✓ DATA SAVED TO DATABASE             ║");
    Serial.println("║    Google Sheets will auto-update              ║");
    Serial.println("╚════════════════════════════════════════════════╝");
  } else {
    Serial.println("[TX] ✗ Failed to save data");
  }

  return success;
}

// ============================================================================
// IFTTT Webhook for SMS + Slack Notifications
// ============================================================================
bool sendIFTTTNotification(const String& nodeName, float temp, float humidity) {
  ensureWiFiConnected();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[IFTTT] No WiFi");
    return false;
  }

  Serial.println("\n[IFTTT] Sending webhook (SMS + Slack)...");
  
  // Build IFTTT URL
  String url = "https://maker.ifttt.com/trigger/";
  url += IFTTT_EVENT_NAME;
  url += "/with/key/";
  url += IFTTT_WEBHOOK_KEY;

  // Create JSON payload with values
  StaticJsonDocument<256> doc;
  doc["value1"] = nodeName;
  doc["value2"] = String(temp, 1);  // Temperature with 1 decimal
  doc["value3"] = String(humidity, 1);  // Humidity with 1 decimal
  
  String payload;
  serializeJson(doc, payload);
  
  Serial.print("[IFTTT] Payload: ");
  Serial.println(payload);

  static WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(10000);
  
  if (!https.begin(client, url)) {
    Serial.println("[IFTTT] ✗ Begin failed");
    return false;
  }

  https.addHeader("Content-Type", "application/json");
  
  int code = https.POST(payload);
  
  Serial.print("[IFTTT] Code: ");
  Serial.println(code);
  
  if (code > 0) {
    Serial.print("[IFTTT] Response: ");
    Serial.println(https.getString());
  }

  https.end();
  
  bool success = (code == 200 || code == HTTP_CODE_OK);
  
  if (success) {
    Serial.println("[IFTTT] ✓ Notification sent");
    Serial.println("        → SMS to +1 707-547-7017");
    Serial.println("        → Slack #iot-sensors");
  } else {
    Serial.println("[IFTTT] ✗ Failed");
  }
  
  return success;
}

// ============================================================================
// Menu & Auto-Poll
// ============================================================================
static void serialMenu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'T' || c == 't') {
    Serial.println("\n╔════════════════════════╗");
    Serial.println("║  TIMEZONE CONFIG       ║");
    Serial.println("╚════════════════════════╝");
    Serial.print("> ");
    while (!Serial.available()) delay(10);
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      Serial.println(setTimezone(input) ? "✓ Updated" : "✗ Invalid");
    }
  }
}

static unsigned long lastAutoPoll = 0;
static const unsigned long AUTO_POLL_INTERVAL = 10000;

static void handleAutoPoll() {
  if (millis() - lastAutoPoll >= AUTO_POLL_INTERVAL) {
    lastAutoPoll = millis();
    Serial.println("\n[AUTO-POLL] Checking web commands...");
    pollAllControls();
  }
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║   ESP8266 INTEGRATED SYSTEM v3.0 FINAL        ║");
  Serial.println("║   Database + Sheets + SMS + Slack              ║");
  Serial.println("╚════════════════════════════════════════════════╝\n");
  
  Serial.println("[INIT] Initializing components...\n");
  
  Serial.print("[INIT] WiFi... ");
  WiFi.setSleep(false);  // Reduce latency
  ensureWiFiConnected();
  Serial.println(WiFi.status() == WL_CONNECTED ? "✓" : "✗");
  
  Serial.print("[INIT] Time... ");
  timeClientBegin();
  Serial.println("✓");
  
  Serial.print("[INIT] Switches... ");
  switchesBegin();
  Serial.println("✓");
  
  Serial.print("[INIT] DHT11... ");
  Serial.println(sensorsBegin() ? "✓" : "✗ Check wiring!");
  
  Serial.print("[INIT] LEDs... ");
  ledsBegin();
  Serial.println("✓");
  
  Serial.print("[INIT] Control... ");
  controlBegin();
  Serial.println("✓");
  
  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║              SYSTEM READY                      ║");
  Serial.println("╠════════════════════════════════════════════════╣");
  Serial.println("║  Button 1 (GPIO0):  Log to Database           ║");
  Serial.println("║                     → Google Sheets            ║");
  Serial.println("║                     → SMS Notification         ║");
  Serial.println("║                     → Slack Alert              ║");
  Serial.println("║                                                ║");
  Serial.println("║  Button 2 (GPIO16): Check LED/RGB Status      ║");
  Serial.println("║                                                ║");
  Serial.println("║  Auto-Poll: Every 10 seconds                  ║");
  Serial.println("╚════════════════════════════════════════════════╝\n");
  
  // Ready blink
  blinkAsync(PIN_LED1, 100, 300);
  delay(400);
  blinkAsync(PIN_LED2, 100, 300);
  
  Serial.println("[INFO] Ready for operation!\n");
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
  serialMenu();
  pollSwitches();
  ledsPoll();
  handleAutoPoll();
  
  // ══════════════════════════════════════════════════════════════
  // BUTTON 1: SENSOR LOGGING
  // ══════════════════════════════════════════════════════════════
  if (takeSwitch1Event()) {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════╗");
    Serial.println("║      BUTTON 1: SENSOR LOGGING EVENT            ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
    
    String timestamp;
    float temperature = 0.0;
    float humidity = 0.0;
    bool sensorsOk = true;
    
    // Step 1: Get timestamp
    Serial.println("═══ [1/5] GET TIMESTAMP ═══");
    if (!readTimeISO(timestamp)) {
      Serial.println("✗ NTP sync failed");
      timestamp = "2025-11-04 00:00:00";
      sensorsOk = false;
    } else {
      Serial.print("✓ ");
      Serial.println(timestamp);
    }
    
    // Step 2: Read DHT11
    Serial.println("\n═══ [2/5] READ DHT11 SENSOR ═══");
    if (!readDHT(temperature, humidity)) {
      Serial.println("✗ DHT11 read FAILED!");
      Serial.println("  Check: GPIO14 (D5) connection");
      temperature = 0.0;
      humidity = 0.0;
      sensorsOk = false;
    } else {
      Serial.print("✓ Temperature: ");
      Serial.print(temperature, 1);
      Serial.println("°C");
      Serial.print("✓ Humidity: ");
      Serial.print(humidity, 1);
      Serial.println("%");
    }
    
    // Step 3: Transmit to database
    Serial.println("\n═══ [3/5] SAVE TO DATABASE ═══");
    uint32_t nextCount = switch1Count() + 1;
    
    bool dbSuccess = false;
    if (sensorsOk) {
      dbSuccess = transmitToDatabase(timestamp, temperature, humidity, nextCount);
      if (dbSuccess) {
        incSwitch1();
        Serial.println("✓ Data logged - Count: " + String(switch1Count()));
      }
    } else {
      Serial.println("⚠ SKIPPED - Sensor errors");
    }
    
    // Step 4: Send IFTTT notification (SMS + Slack)
    Serial.println("\n═══ [4/5] SEND NOTIFICATIONS ═══");
    if (sensorsOk) {
      String nodeName = "node_1";
      bool iftttSuccess = sendIFTTTNotification(nodeName, temperature, humidity);
      if (iftttSuccess) {
        Serial.println("✓ Notifications sent successfully");
      } else {
        Serial.println("⚠ Notifications partially failed");
      }
    } else {
      Serial.println("⚠ SKIPPED - Sensor errors");
    }
    
    // Step 5: Visual feedback
    Serial.println("\n═══ [5/5] VISUAL CONFIRMATION ═══");
    blinkAsync(PIN_LED1, 250, 2000);
    Serial.println("✓ LED1 blinking (2 seconds)");
    
    // Summary
    Serial.println("\n╔════════════════════════════════════════════════╗");
    if (sensorsOk && dbSuccess) {
      Serial.println("║           ✓ ALL OPERATIONS SUCCESSFUL         ║");
    } else {
      Serial.println("║           ⚠ SOME OPERATIONS FAILED            ║");
    }
    Serial.print("║  Activity Count: ");
    Serial.print(switch1Count());
    if (switch1Count() < 10) Serial.print("  ");
    else if (switch1Count() < 100) Serial.print(" ");
    Serial.println("                            ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
  }
  
  // ══════════════════════════════════════════════════════════════
  // BUTTON 2: STATUS CHECK
  // ══════════════════════════════════════════════════════════════
  if (takeSwitch2Event()) {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════╗");
    Serial.println("║      BUTTON 2: LED/RGB STATUS CHECK            ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
    
    Serial.println("═══ [1/3] POLL LED STATUS ═══");
    bool ledChanged = pollLEDControl();
    Serial.println(ledChanged ? "✓ LED states updated" : "✓ No changes");
    
    delay(100);
    
    Serial.println("\n═══ [2/3] POLL RGB STATUS ═══");
    bool rgbChanged = pollRGBControl();
    Serial.println(rgbChanged ? "✓ RGB values updated" : "✓ No changes");
    
    String ledStatus = getLEDStatusString();
    String rgbStatus = getRGBStatusString();
    
    Serial.println("\nCurrent Status:");
    Serial.print("  ");
    Serial.println(ledStatus);
    Serial.print("  ");
    Serial.println(rgbStatus);
    
    Serial.println("\n═══ [3/3] VISUAL CONFIRMATION ═══");
    blinkAsync(PIN_LED2, 250, 2000);
    Serial.println("✓ LED2 blinking (2 seconds)");
    
    incSwitch2();
    
    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.print("║  Activity Count: ");
    Serial.print(switch2Count());
    if (switch2Count() < 10) Serial.print("  ");
    else if (switch2Count() < 100) Serial.print(" ");
    Serial.println("                            ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
  }
  
  delay(10);
}
