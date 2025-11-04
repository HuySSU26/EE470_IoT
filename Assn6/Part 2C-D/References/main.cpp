/**
 * ========================================================================
 * UNIFIED ESP8266 IoT SYSTEM - Battery Powered with Two Buttons
 * ========================================================================
 * 
 * PlatformIO Project - Revised Version with secrets.h
 * Platform: ESP8266
 * Board: NodeMCU v2
 * Framework: Arduino
 * 
 * Integrates:
 * - Part 1: Sensor data reporting to Google Sheets/Slack via IFTTT
 * - Part 2A: LED1 & LED2 control via led_control.php
 * - Part 2B: RGB LED control via rgb_value.txt
 * 
 * Hardware Connections:
 * ────────────────────────────────────────────────────────────────────
 * BUTTONS:
 *   Button 1:  GPIO0  (D3) - Sensor Report to Slack/Google (with pull-up)
 *   Button 2:  GPIO16 (D0) - Check LED/RGB status (with pull-up)
 * 
 * LEDS (Digital):
 *   LED1:      GPIO12 (D6) → 220Ω → LED → GND
 *   LED2:      GPIO13 (D7) → 220Ω → LED → GND
 * 
 * RGB LED (Common Cathode):
 *   Red:       GPIO15 (D8) → 330Ω → R pin
 *   Green:     GPIO4  (D2) → Resistor → G pin
 *   Blue:      GPIO5  (D1) → Resistor → B pin
 *   Cathode:   GND
 * 
 * SENSORS (DHT11):
 *   Data:      GPIO14 (D5)
 *   VCC:       3.3V
 *   GND:       GND
 * 
 * Configuration:
 * ────────────────────────────────────────────────────────────────────
 * All credentials and settings are in include/secrets.h
 * To change WiFi or other settings, edit that file only!
 * 
 * ========================================================================
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// ==================== INCLUDE SECRETS ====================
// All sensitive credentials are in this file
// Edit include/secrets.h to change WiFi, IFTTT, etc.
#include "secrets.h"

// ==================== BUILD IFTTT URL ====================
const String IFTTT_URL = String("https://maker.ifttt.com/trigger/") + 
                         String(IFTTT_EVENT) + "/with/key/" + String(IFTTT_KEY);

// ==================== GPIO PINS ====================
// Buttons
const int BUTTON1_PIN = 0;   // D3 - Sensor Report (boot button)
const int BUTTON2_PIN = 16;  // D0 - LED/RGB Status Check

// Digital LEDs
const int LED1_PIN = 12;     // D6
const int LED2_PIN = 13;     // D7

// RGB LED
const int RGB_RED_PIN   = 15;  // D8
const int RGB_GREEN_PIN = 4;   // D2
const int RGB_BLUE_PIN  = 5;   // D1

// DHT Sensor
const int DHT_PIN = 14;      // D5
const int DHT_TYPE = DHT11;

// ==================== SENSOR & DHT ====================
DHT dht(DHT_PIN, DHT_TYPE);
int activityCounter = 0;

// ==================== TIMING CONSTANTS ====================
#ifndef AUTO_CHECK_INTERVAL
  const unsigned long AUTO_CHECK_INTERVAL = 120000;  // 2 minutes
#endif

#ifndef FAST_POLLING_INTERVAL
  const unsigned long FAST_POLLING_INTERVAL = 1000;  // 1 second
#endif

#ifndef SLOW_POLLING_INTERVAL
  const unsigned long SLOW_POLLING_INTERVAL = 10000;  // 10 seconds
#endif

#ifndef HTTP_TIMEOUT
  const unsigned long HTTP_TIMEOUT = 10000;  // 10 seconds
#endif

#ifndef MESSAGE_QUEUE_SIZE
  const int MESSAGE_QUEUE_SIZE = 10;
#endif

const unsigned long FAST_WINDOW_MS = 30000;  // 30 seconds
const unsigned long DEBOUNCE_DELAY = 50;     // 50 milliseconds

// ==================== STATE VARIABLES ====================
// LED states
bool led1State = false;
bool led2State = false;

// RGB values
int rgbRed = 0;
int rgbGreen = 0;
int rgbBlue = 0;

// Timing
unsigned long lastAutoCheck = 0;
unsigned long fastUntil = 0;

// Change detection
String lastLedTimestamp = "";
String lastRgbData = "";

// Button debounce
bool lastButton1State = HIGH;
bool lastButton2State = HIGH;
unsigned long lastDebounce1 = 0;
unsigned long lastDebounce2 = 0;

// Message queue for simultaneous button presses
struct Message {
  String content;
  String url;
  bool isJson;
};

Message messageQueue[MESSAGE_QUEUE_SIZE];
int queueFront = 0;
int queueRear = 0;
int queueCount = 0;

// ==================== FORWARD DECLARATIONS ====================
void handleButtons();
void button1Action();
void button2Action();
void pollLEDStatus();
void pollRGBValues();
void readAndSendSensorData();
void sendIFTTT(String nodeName, float temp, float humidity);
void setRGBColor(int r, int g, int b);
void ensureWiFi();
bool enqueueMessage(String content, String url, bool isJson);
void processMessageQueue();

// ==================== HELPER FUNCTIONS ====================

/** Ensure WiFi connection */
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("\n[WiFi] Connecting...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] ✓ Connected! IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] MAC: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("[WiFi] ✗ Connection failed!");
    Serial.println("[WiFi] Check SSID/password in include/secrets.h");
  }
}

/** Remove spaces and convert to uppercase */
static String upperNoSpace(String s) {
  s.replace("\r", "");
  s.replace("\n", "");
  s.replace("\t", " ");
  while (s.indexOf("  ") >= 0) s.replace("  ", " ");
  s.replace(" ", "");
  s.toUpperCase();
  return s;
}

/** Parse boolean ON/OFF from JSON */
static bool parseOnOff(const String& json, const char* key, bool& ok) {
  ok = false;
  String s = upperNoSpace(json);
  String k = "\""; k += key; k += "\""; k.toUpperCase();

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

/** Get string value from JSON */
static String getJsonString(const String& json, const char* key, bool& ok) {
  ok = false;
  
  String needle = String("\"") + key + String("\"");
  int pos = json.indexOf(needle);
  if (pos < 0) return "";

  int colon = json.indexOf(':', pos + needle.length());
  if (colon < 0) return "";

  int i = colon + 1;
  while (i < (int)json.length() && isspace(json[i])) i++;

  if (i >= (int)json.length() || json[i] != '\"') return "";
  int q1 = i;
  int q2 = json.indexOf('\"', q1 + 1);
  if (q2 < 0) return "";

  ok = true;
  return json.substring(q1 + 1, q2);
}

/** Set RGB LED color using PWM */
void setRGBColor(int r, int g, int b) {
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  
  analogWrite(RGB_RED_PIN,   map(r, 0, 255, 0, 1023));
  analogWrite(RGB_GREEN_PIN, map(g, 0, 255, 0, 1023));
  analogWrite(RGB_BLUE_PIN,  map(b, 0, 255, 0, 1023));
  
  Serial.print("[RGB] Color: R=");
  Serial.print(r);
  Serial.print(" G=");
  Serial.print(g);
  Serial.print(" B=");
  Serial.println(b);
}

// ==================== MESSAGE QUEUE FUNCTIONS ====================

bool enqueueMessage(String content, String url, bool isJson) {
  if (queueCount >= MESSAGE_QUEUE_SIZE) {
    Serial.println("[QUEUE] Queue full! Message dropped.");
    return false;
  }
  
  messageQueue[queueRear].content = content;
  messageQueue[queueRear].url = url;
  messageQueue[queueRear].isJson = isJson;
  
  queueRear = (queueRear + 1) % MESSAGE_QUEUE_SIZE;
  queueCount++;
  
  Serial.print("[QUEUE] Message queued. Queue size: ");
  Serial.println(queueCount);
  return true;
}

void processMessageQueue() {
  if (queueCount == 0) return;
  
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[QUEUE] No WiFi - cannot process queue");
    return;
  }
  
  Serial.println("\n[QUEUE] Processing messages...");
  
  while (queueCount > 0) {
    Message msg = messageQueue[queueFront];
    queueFront = (queueFront + 1) % MESSAGE_QUEUE_SIZE;
    queueCount--;
    
    Serial.print("[QUEUE] Sending to: ");
    Serial.println(msg.url);
    
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    
    if (!http.begin(client, msg.url)) {
      Serial.println("[QUEUE] HTTP begin failed");
      continue;
    }
    
    int httpCode = -1;
    
    if (msg.isJson) {
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(msg.content);
    } else {
      httpCode = http.GET();
    }
    
    if (httpCode > 0) {
      Serial.print("[QUEUE] Response: ");
      Serial.println(httpCode);
      if (httpCode == HTTP_CODE_OK) {
        Serial.println("[QUEUE] ✓ Message sent successfully");
      }
    } else {
      Serial.print("[QUEUE] ✗ Error: ");
      Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
    delay(300);
  }
  
  Serial.println("[QUEUE] All messages processed\n");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);
  WiFi.setSleep(false);
  delay(500);

  Serial.println("\n\n╔════════════════════════════════════════════════════╗");
  Serial.println("║  UNIFIED ESP8266 IoT SYSTEM                       ║");
  Serial.println("║  With Secrets.h Configuration                     ║");
  Serial.println("║                                                    ║");
  Serial.println("║  Button 1 (GPIO0):  Send Sensor Data to Slack    ║");
  Serial.println("║  Button 2 (GPIO16): Check LED/RGB Status         ║");
  Serial.println("║                                                    ║");
  Serial.println("║  LED1: GPIO12 | LED2: GPIO13                      ║");
  Serial.println("║  RGB: R=GPIO15, G=GPIO4, B=GPIO5                  ║");
  Serial.println("║  DHT11: GPIO14                                    ║");
  Serial.println("╚════════════════════════════════════════════════════╝\n");

  // Print configuration from secrets.h
  Serial.println("[CONFIG] Configuration loaded from secrets.h:");
  Serial.print("[CONFIG] Node Number: ");
  Serial.println(NODE_NUMBER);
  Serial.print("[CONFIG] IFTTT Event: ");
  Serial.println(IFTTT_EVENT);
  Serial.print("[CONFIG] LED Control URL: ");
  Serial.println(LED_CONTROL_URL);
  Serial.print("[CONFIG] RGB Proxy URL: ");
  Serial.println(RGB_PROXY_URL);
  Serial.print("[CONFIG] Sensor Dashboard URL: ");
  Serial.println(SENSOR_DASHBOARD_URL);
  Serial.println();

  // Setup buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  Serial.println("[INIT] Buttons initialized");

  // Setup digital LED pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  Serial.println("[INIT] Digital LEDs initialized");

  // Setup RGB LED pins
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setRGBColor(0, 0, 0);
  Serial.println("[INIT] RGB LED initialized");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("[INIT] DHT11 sensor initialized");

  // Connect to WiFi
  ensureWiFi();

  // Startup test sequence
  Serial.println("\n[TEST] Running startup sequence...");
  
  // Test LED1
  digitalWrite(LED1_PIN, HIGH);
  delay(300);
  digitalWrite(LED1_PIN, LOW);
  
  // Test LED2
  digitalWrite(LED2_PIN, HIGH);
  delay(300);
  digitalWrite(LED2_PIN, LOW);
  
  // Test RGB
  setRGBColor(255, 0, 0);  // Red
  delay(300);
  setRGBColor(0, 255, 0);  // Green
  delay(300);
  setRGBColor(0, 0, 255);  // Blue
  delay(300);
  setRGBColor(0, 0, 0);    // Off
  
  Serial.println("\n[READY] ✓ System initialized and ready!");
  Serial.println("[INFO] Button 1: Send sensor data");
  Serial.println("[INFO] Button 2: Check LED/RGB status");
  Serial.println("[INFO] To change WiFi: Edit include/secrets.h\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  handleButtons();
  processMessageQueue();
  
  if (millis() - lastAutoCheck >= AUTO_CHECK_INTERVAL) {
    Serial.println("[AUTO] Scheduled LED/RGB check");
    pollLEDStatus();
    delay(100);
    pollRGBValues();
    lastAutoCheck = millis();
  }
  
  delay(10);
}

// ==================== BUTTON HANDLERS ====================

void handleButtons() {
  // Button 1 (GPIO0) - Sensor Report
  bool reading1 = digitalRead(BUTTON1_PIN);
  if (reading1 != lastButton1State) {
    lastDebounce1 = millis();
  }
  
  if ((millis() - lastDebounce1) > DEBOUNCE_DELAY) {
    if (reading1 == LOW && lastButton1State == HIGH) {
      Serial.println("\n[BUTTON1] ⚡ Sensor report triggered!");
      button1Action();
      
      while (digitalRead(BUTTON1_PIN) == LOW) delay(10);
      delay(100);
    }
  }
  lastButton1State = reading1;
  
  // Button 2 (GPIO16) - LED/RGB Status Check
  bool reading2 = digitalRead(BUTTON2_PIN);
  if (reading2 != lastButton2State) {
    lastDebounce2 = millis();
  }
  
  if ((millis() - lastDebounce2) > DEBOUNCE_DELAY) {
    if (reading2 == LOW && lastButton2State == HIGH) {
      Serial.println("\n[BUTTON2] ⚡ LED/RGB status check triggered!");
      button2Action();
      
      while (digitalRead(BUTTON2_PIN) == LOW) delay(10);
      delay(100);
    }
  }
  lastButton2State = reading2;
}

void button1Action() {
  readAndSendSensorData();
}

void button2Action() {
  pollLEDStatus();
  delay(100);
  pollRGBValues();
}

// ==================== SENSOR FUNCTIONS ====================

void readAndSendSensorData() {
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  READING SENSOR DATA");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[SENSOR] ✗ Failed to read from DHT sensor!");
    return;
  }
  
  Serial.print("[SENSOR] Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("[SENSOR] Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  activityCounter++;
  
  String timestamp = String(millis() / 1000);
  
  String jsonPayload = "{";
  jsonPayload += "\"node\":" + String(NODE_NUMBER) + ",";
  jsonPayload += "\"temperature_C\":" + String(temperature, 2) + ",";
  jsonPayload += "\"humidity_pct\":" + String(humidity, 2) + ",";
  jsonPayload += "\"timestamp\":\"" + timestamp + "\",";
  jsonPayload += "\"activity_count\":" + String(activityCounter);
  jsonPayload += "}";
  
  Serial.println("[SENSOR] JSON prepared:");
  Serial.println(jsonPayload);
  
  enqueueMessage(jsonPayload, SENSOR_DASHBOARD_URL, true);
  
  String nodeName = "node_" + String(NODE_NUMBER);
  sendIFTTT(nodeName, temperature, humidity);
  
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  SENSOR DATA QUEUED");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void sendIFTTT(String nodeName, float temp, float humidity) {
  Serial.println("[IFTTT] Preparing notification...");
  
  String iftttJson = "{";
  iftttJson += "\"value1\":\"" + nodeName + "\",";
  iftttJson += "\"value2\":\"" + String(temp, 1) + "\",";
  iftttJson += "\"value3\":\"" + String(humidity, 1) + "\"";
  iftttJson += "}";
  
  Serial.println("[IFTTT] Payload:");
  Serial.println(iftttJson);
  
  enqueueMessage(iftttJson, IFTTT_URL, true);
}

// ==================== LED STATUS POLLING ====================

void pollLEDStatus() {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[LED] ✗ No WiFi");
    return;
  }

  Serial.println("[LED] Checking led_control.php...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(7000);

  String url = String(LED_CONTROL_URL) + "?t=" + String(millis());

  if (!http.begin(client, url)) {
    Serial.println("[LED] ✗ HTTP begin failed");
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    
    bool ok1 = false, ok2 = false;
    bool newLed1 = parseOnOff(body, "led1", ok1);
    bool newLed2 = parseOnOff(body, "led2", ok2);

    bool changed = false;

    if (ok1 && newLed1 != led1State) {
      led1State = newLed1;
      digitalWrite(LED1_PIN, led1State ? HIGH : LOW);
      changed = true;
      Serial.print("[LED] LED1 → ");
      Serial.println(led1State ? "ON ✓" : "OFF");
    }

    if (ok2 && newLed2 != led2State) {
      led2State = newLed2;
      digitalWrite(LED2_PIN, led2State ? HIGH : LOW);
      changed = true;
      Serial.print("[LED] LED2 → ");
      Serial.println(led2State ? "ON ✓" : "OFF");
    }

    bool tsOk = false;
    String ts = getJsonString(body, "timestamp", tsOk);
    if (tsOk && ts.length() && ts != lastLedTimestamp) {
      lastLedTimestamp = ts;
      changed = true;
    }

    if (changed) {
      fastUntil = millis() + FAST_WINDOW_MS;
    } else {
      Serial.print("[LED] ✓ No changes (LED1:");
      Serial.print(led1State ? "ON" : "OFF");
      Serial.print(", LED2:");
      Serial.print(led2State ? "ON" : "OFF");
      Serial.println(")");
    }

  } else {
    Serial.print("[LED] ✗ HTTP error: ");
    Serial.println(code);
  }

  http.end();
}

// ==================== RGB STATUS POLLING ====================

void pollRGBValues() {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[RGB] ✗ No WiFi");
    return;
  }

  Serial.println("[RGB] Checking rgb_proxy.php...");

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 512);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  http.setReuse(false);

  String url = String(RGB_PROXY_URL) + "?t=" + String(millis());

  if (!http.begin(client, url)) {
    Serial.println("[RGB] ✗ HTTP begin failed");
    return;
  }

  http.addHeader("Accept", "text/plain");
  http.addHeader("User-Agent", "ESP8266");

  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    body.trim();
    
    if (body.indexOf("<html") >= 0 || body.indexOf("<!DOCTYPE") >= 0) {
      Serial.println("[RGB] ✗ Got HTML (redirect)");
      http.end();
      return;
    }

    if (body != lastRgbData && body.length() > 0 && body.length() < 50) {
      lastRgbData = body;
      
      int comma1 = body.indexOf(',');
      int comma2 = body.indexOf(',', comma1 + 1);
      
      if (comma1 > 0 && comma2 > comma1) {
        int newR = body.substring(0, comma1).toInt();
        int newG = body.substring(comma1 + 1, comma2).toInt();
        int newB = body.substring(comma2 + 1).toInt();
        
        newR = constrain(newR, 0, 255);
        newG = constrain(newG, 0, 255);
        newB = constrain(newB, 0, 255);
        
        if (newR != rgbRed || newG != rgbGreen || newB != rgbBlue) {
          rgbRed = newR;
          rgbGreen = newG;
          rgbBlue = newB;
          
          Serial.print("[RGB] ✓ Updated: R=");
          Serial.print(rgbRed);
          Serial.print(", G=");
          Serial.print(rgbGreen);
          Serial.print(", B=");
          Serial.println(rgbBlue);
          
          setRGBColor(rgbRed, rgbGreen, rgbBlue);
          
          fastUntil = millis() + FAST_WINDOW_MS;
        } else {
          Serial.println("[RGB] ✓ No changes");
        }
      } else {
        Serial.print("[RGB] ✗ Invalid format: ");
        Serial.println(body);
      }
    } else {
      Serial.println("[RGB] ✓ No changes");
    }
  } else {
    Serial.print("[RGB] ✗ HTTP error: ");
    Serial.println(code);
  }

  http.end();
}
