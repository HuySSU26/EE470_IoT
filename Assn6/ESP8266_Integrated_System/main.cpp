/*
 * ============================================================================
 * ESP8266 Integrated Control System - AUTO-RESTART VERSION
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
 * ============================================================================
 * Latest Version:
 * ============================================================================
 * Issue: Memory drops from 42KB to 14KB (not enough for SSL)
 * Solution: Offer to restart ESP8266 to free memory before Button 1
 * 
 * Features:
 * - Memory check before Button 1
 * - Auto-restart if memory < 20KB
 * - Manual restart command
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
const char* IFTTT_WEBHOOK_KEY = "8RnOIAiHnVAoyOFXIZg2D";
const char* IFTTT_EVENT_NAME = "sensor_alert";

// Memory management
const int MIN_MEMORY_FOR_SSL = 20000;  // 20KB minimum
static bool autoRestartEnabled = true;

// ============================================================================
// Check and Restart if Low Memory
// ============================================================================
bool checkMemoryAndRestart() {
  int freeHeap = ESP.getFreeHeap();
  
  Serial.print("\n[MEM] Current free heap: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");
  
  if (freeHeap < MIN_MEMORY_FOR_SSL) {
    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.println("║  ⚠ LOW MEMORY DETECTED!                        ║");
    Serial.println("╠════════════════════════════════════════════════╣");
    Serial.print("║  Free: ");
    Serial.print(freeHeap);
    Serial.println(" bytes (need 20,000+)          ║");
    Serial.println("║                                                ║");
    
    if (autoRestartEnabled) {
      Serial.println("║  → AUTO-RESTARTING in 3 seconds...            ║");
      Serial.println("║     (This will free memory for SSL)           ║");
      Serial.println("║     LEDs/RGB/Sensors will be preserved        ║");
      Serial.println("╚════════════════════════════════════════════════╝\n");
      
      delay(1000);
      Serial.println("[RESTART] 3...");
      delay(1000);
      Serial.println("[RESTART] 2...");
      delay(1000);
      Serial.println("[RESTART] 1...");
      delay(500);
      Serial.println("[RESTART] Rebooting now...\n");
      
      ESP.restart();
      // Never returns
    } else {
      Serial.println("║  → Auto-restart DISABLED                      ║");
      Serial.println("║     Button 1 will likely fail                 ║");
      Serial.println("║     Type 'R' to restart manually              ║");
      Serial.println("╚════════════════════════════════════════════════╝\n");
      return false;
    }
  }
  
  Serial.println("[MEM] ✓ Memory sufficient for SSL operations");
  return true;
}

// ============================================================================
// Transmit to Database
// ============================================================================
bool transmitToDatabase(const String& timestamp, float temp, float humidity, uint32_t count) {
  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║        TRANSMITTING TO DATABASE                ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  if (!ensureWiFi()) return false;
  
  StaticJsonDocument<512> doc;
  doc["node"] = 1;
  doc["temperature_C"] = temp;
  doc["humidity_pct"] = humidity;
  doc["timestamp"] = timestamp;
  doc["activity_count"] = count;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.print("[TX] Payload: ");
  Serial.println(jsonPayload);

  WiFiClientSecure* client = new WiFiClientSecure();
  client->setInsecure();
  client->setTimeout(15000);

  HTTPClient https;
  https.setTimeout(15000);
  https.setReuse(false);
  
  if (!https.begin(*client, SENSOR_DASHBOARD_URL)) {
    delete client;
    return false;
  }
  
  https.addHeader("Content-Type", "application/json");
  
  int httpCode = https.POST(jsonPayload);
  
  Serial.print("[TX] HTTP Code: ");
  Serial.println(httpCode);
  
  if (httpCode > 0) {
    Serial.println(https.getString());
  }

  https.end();
  delete client;
  
  return (httpCode == 200);
}

// ============================================================================
// IFTTT Notification
// ============================================================================
bool sendIFTTTNotification(const String& nodeName, float temp, float humidity) {
  if (!ensureWiFi()) return false;

  Serial.println("\n[IFTTT] Sending webhook...");
  
  String url = "https://maker.ifttt.com/trigger/";
  url += IFTTT_EVENT_NAME;
  url += "/with/key/";
  url += IFTTT_WEBHOOK_KEY;

  StaticJsonDocument<256> doc;
  doc["value1"] = nodeName;
  doc["value2"] = String(temp, 1);
  doc["value3"] = String(humidity, 1);
  
  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure* client = new WiFiClientSecure();
  client->setInsecure();
  client->setTimeout(10000);

  HTTPClient https;
  https.setTimeout(10000);
  https.setReuse(false);
  
  if (!https.begin(*client, url)) {
    delete client;
    return false;
  }

  https.addHeader("Content-Type", "application/json");
  int code = https.POST(payload);
  
  Serial.print("[IFTTT] Code: ");
  Serial.println(code);
  
  if (code > 0) {
    Serial.println(https.getString());
  }

  https.end();
  delete client;
  
  return (code == 200);
}

// ============================================================================
// Menu & Auto-Poll
// ============================================================================
static void serialMenu() {
  if (!Serial.available()) return;
  char c = Serial.read();
  
  if (c == 'M' || c == 'm') {
    Serial.println("\n╔════════════════════════╗");
    Serial.println("║  MEMORY STATUS         ║");
    Serial.println("╚════════════════════════╝");
    Serial.print("Free: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("Frag: ");
    Serial.print(ESP.getHeapFragmentation());
    Serial.println("%");
    Serial.print("Need: ");
    Serial.print(MIN_MEMORY_FOR_SSL);
    Serial.println(" bytes for SSL");
  } else if (c == 'R' || c == 'r') {
    Serial.println("\n[RESTART] Manual restart requested...");
    delay(1000);
    ESP.restart();
  } else if (c == 'A' || c == 'a') {
    autoRestartEnabled = !autoRestartEnabled;
    Serial.print("\n[AUTO-RESTART] ");
    Serial.println(autoRestartEnabled ? "ENABLED" : "DISABLED");
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
  
  Serial.println("\n\n\n");
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║   ESP8266 AUTO-RESTART SYSTEM                  ║");
  Serial.println("║   Restarts automatically when memory low       ║");
  Serial.println("╚════════════════════════════════════════════════╝\n");
  
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  ensureWiFi();
  
  timeClientBegin();
  switchesBegin();
  sensorsBegin();
  ledsBegin();
  controlBegin();
  
  Serial.print("\n[INIT] Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  
  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║              SYSTEM READY                      ║");
  Serial.println("╠════════════════════════════════════════════════╣");
  Serial.println("║  Button 1: Log + Notify (auto-restart if low) ║");
  Serial.println("║  Button 2: Check LED/RGB Status               ║");
  Serial.println("║                                                ║");
  Serial.println("║  Commands:                                     ║");
  Serial.println("║  Type 'M': Memory status                      ║");
  Serial.println("║  Type 'R': Manual restart                     ║");
  Serial.println("║  Type 'A': Toggle auto-restart                ║");
  Serial.println("║                                                ║");
  Serial.println("║  Auto-Poll: Every 10 seconds ✓                ║");
  Serial.println("╚════════════════════════════════════════════════╝\n");
  
  blinkAsync(PIN_LED1, 100, 300);
  delay(400);
  blinkAsync(PIN_LED2, 100, 300);
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
  // BUTTON 1 with Memory Check & Auto-Restart
  // ══════════════════════════════════════════════════════════════
  if (takeSwitch1Event()) {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════╗");
    Serial.println("║      BUTTON 1: SENSOR LOGGING EVENT            ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
    
    // Check memory FIRST - restart if needed
    if (!checkMemoryAndRestart()) {
      Serial.println("⚠ Continuing with low memory (likely to fail)\n");
    }
    
    String timestamp;
    float temperature = 0.0;
    float humidity = 0.0;
    bool sensorsOk = true;
    
    Serial.println("═══ [1/5] TIMESTAMP ═══");
    if (!readTimeISO(timestamp)) {
      timestamp = "2025-11-04 00:00:00";
      sensorsOk = false;
    } else {
      Serial.print("✓ ");
      Serial.println(timestamp);
    }
    
    Serial.println("\n═══ [2/5] DHT11 ═══");
    if (!readDHT(temperature, humidity)) {
      sensorsOk = false;
    } else {
      Serial.print("✓ ");
      Serial.print(temperature, 1);
      Serial.print("°C, ");
      Serial.print(humidity, 1);
      Serial.println("%");
    }
    
    Serial.println("\n═══ [3/5] DATABASE ═══");
    bool dbSuccess = false;
    if (sensorsOk) {
      uint32_t cnt = switch1Count() + 1;
      dbSuccess = transmitToDatabase(timestamp, temperature, humidity, cnt);
      if (dbSuccess) incSwitch1();
    }
    
    delay(500);
    
    Serial.println("\n═══ [4/5] IFTTT ═══");
    bool notifySuccess = false;
    if (sensorsOk) {
      notifySuccess = sendIFTTTNotification("node_1", temperature, humidity);
    }
    
    Serial.println("\n═══ [5/5] VISUAL ═══");
    blinkAsync(PIN_LED1, 250, 2000);
    
    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.println("║               SUMMARY                          ║");
    Serial.println("╠════════════════════════════════════════════════╣");
    Serial.print("║  Sensors:  ");
    Serial.println(sensorsOk ? "✓ OK    ║" : "✗ FAIL  ║");
    Serial.print("║  Database: ");
    Serial.println(dbSuccess ? "✓ OK    ║" : "✗ FAIL  ║");
    Serial.print("║  IFTTT:    ");
    Serial.println(notifySuccess ? "✓ OK    ║" : "✗ FAIL  ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
    
    if (!dbSuccess || !notifySuccess) {
      Serial.println("💡 System will auto-restart before next Button 1");
      Serial.println("   to ensure enough memory for SSL\n");
    }
  }
  
  // ══════════════════════════════════════════════════════════════
  // BUTTON 2
  // ══════════════════════════════════════════════════════════════
  if (takeSwitch2Event()) {
    Serial.println("\n[BUTTON 2] Status check...");
    pollLEDControl();
    pollRGBControl();
    Serial.println(getLEDStatusString());
    Serial.println(getRGBStatusString());
    blinkAsync(PIN_LED2, 250, 2000);
    incSwitch2();
  }
  
  delay(10);
}
