/**
 * ESP8266 LED + RGB Control System
 * =================================
 * 
 * Features:
 * 1. LED1 & LED2 Control (ON/OFF) via led_control.php
 * 2. RGB LED Control via mySlider.php (3 sliders: R, G, B)
 * 
 * Hardware Connections:
 * ────────────────────
 * LED1:    GPIO12 (D6) → 220Ω → LED → GND
 * LED2:    GPIO13 (D7) → 220Ω → LED → GND
 * 
 * RGB LED (Common Cathode):
 * Red:     GPIO15 (D8) → 330Ω → R pin
 * Green:   GPIO4  (D2) → Resistor → G pin
 * Blue:    GPIO5  (D1) → Resistor → B pin
 * Cathode: GND
 * 
 * Button:  GPIO0  (D3) with INPUT_PULLUP
 * 
 * Data Sources:
 * ───────────
 * https://huynguyen.co/led_control.php  → LED ON/OFF status
 * https://huynguyen.co/rgb_value.txt    → RGB values (format: R,G,B)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// ==================== WIFI CONFIGURATION ====================
const char* WIFI_SSID     = "WIFI_SSID";        // Replace with  actual SSID
const char* WIFI_PASSWORD = "WIFI_SSID_PSK";    // Replace with  actual PSK

// ==================== SERVER URLS ====================
const char* LED_URL = "https://huynguyen.co/led_control.php";
const char* RGB_URL = "https://huynguyen.co/rgb_proxy.php";  // Back to HTTPS - server forces it

// ==================== GPIO PINS ====================
// Digital LEDs
const int LED1_PIN = 12;   // D6
const int LED2_PIN = 13;   // D7

// RGB LED (PWM capable pins)
const int RGB_RED_PIN   = 15;  // D8 (GPIO15) - with 330Ω resistor
const int RGB_GREEN_PIN = 4;   // D2 (GPIO4)
const int RGB_BLUE_PIN  = 5;   // D1 (GPIO5)

// Button
const int BUTTON_PIN = 0;  // D3 (boot button with INPUT_PULLUP)

// ==================== POLLING INTERVALS ====================
const unsigned long FAST_INTERVAL = 1000;    // 1 second when active
const unsigned long SLOW_INTERVAL = 10000;   // 10 seconds when idle
const unsigned long FAST_WINDOW_MS = 30000;  // Stay fast for 30s after change

// ==================== STATE VARIABLES ====================
// LED states
bool led1State = false;
bool led2State = false;

// RGB values (0-255)
int rgbRed = 0;
int rgbGreen = 0;
int rgbBlue = 0;

// Timing
unsigned long nextPollAt = 0;
unsigned long fastUntil = 0;

// Timestamps to detect changes
String lastLedTimestamp = "";
String lastRgbData = "";

// Button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ==================== FORWARD DECLARATIONS ====================
void handleButton();
void pollAllSources();
void pollLEDStatus();
void pollRGBValues();
void setRGBColor(int r, int g, int b);
void ensureWiFi();

// ==================== HELPER FUNCTIONS ====================

/** Remove spaces/newlines and convert to uppercase */
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

/** Ensure WiFi connection */
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("\n[WiFi] Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connection failed!");
  }
}

// ==================== RGB LED CONTROL ====================

/** 
 * Set RGB LED color using PWM (0-255 per channel)
 */
void setRGBColor(int r, int g, int b) {
  // Constrain values
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  
  // Set PWM values (ESP8266 uses 0-1023 range)
  analogWrite(RGB_RED_PIN,   map(r, 0, 255, 0, 1023));
  analogWrite(RGB_GREEN_PIN, map(g, 0, 255, 0, 1023));
  analogWrite(RGB_BLUE_PIN,  map(b, 0, 255, 0, 1023));
  
  Serial.print("[RGB] Color set to R=");
  Serial.print(r);
  Serial.print(" G=");
  Serial.print(g);
  Serial.print(" B=");
  Serial.println(b);
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);
  WiFi.setSleep(false);
  delay(500);

  Serial.println("\n\n╔════════════════════════════════════════════╗");
  Serial.println("║  ESP8266 LED + RGB CONTROL SYSTEM         ║");
  Serial.println("║                                            ║");
  Serial.println("║  LED1: GPIO12 (D6) | LED2: GPIO13 (D7)    ║");
  Serial.println("║  RGB: R=GPIO15, G=GPIO4, B=GPIO5          ║");
  Serial.println("║  Control via mySlider.php (3 channels)    ║");
  Serial.println("║                                            ║");
  Serial.println("║  Sources:                                  ║");
  Serial.println("║  • led_control.php (LED ON/OFF)           ║");
  Serial.println("║  • rgb_value.txt (R,G,B values)           ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  // Setup digital LED pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  Serial.println("[INIT] Digital LEDs initialized (GPIO12, GPIO13)");

  // Setup RGB LED pins (PWM)
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setRGBColor(0, 0, 0);
  Serial.println("[INIT] RGB LED initialized (GPIO15=R, GPIO4=G, GPIO5=B)");

  // Setup button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("[INIT] Button initialized (GPIO0)");

  // Connect to WiFi
  ensureWiFi();

  // Visual startup test
  Serial.println("\n[TEST] Running startup sequence...");
  
  // Test digital LEDs
  Serial.println("[TEST] Testing LED1...");
  digitalWrite(LED1_PIN, HIGH);
  delay(300);
  digitalWrite(LED1_PIN, LOW);
  
  Serial.println("[TEST] Testing LED2...");
  digitalWrite(LED2_PIN, HIGH);
  delay(300);
  digitalWrite(LED2_PIN, LOW);
  
  // Test RGB (quick color cycle)
  Serial.println("[TEST] Testing RGB - Red");
  setRGBColor(255, 0, 0);
  delay(300);
  
  Serial.println("[TEST] Testing RGB - Green");
  setRGBColor(0, 255, 0);
  delay(300);
  
  Serial.println("[TEST] Testing RGB - Blue");
  setRGBColor(0, 0, 255);
  delay(300);
  
  setRGBColor(0, 0, 0);
  
  Serial.println("\n[READY] ✓ All systems initialized!");
  Serial.println("[READY] Waiting for commands...\n");

  // First poll immediately
  nextPollAt = millis();
}

// ==================== MAIN LOOP ====================
void loop() {
  handleButton();

  // Adaptive polling interval
  unsigned long interval = (millis() < fastUntil) ? FAST_INTERVAL : SLOW_INTERVAL;

  if ((long)(millis() - nextPollAt) >= 0) {
    pollAllSources();
    nextPollAt = millis() + interval;
  }

  delay(10);
}

// ==================== BUTTON HANDLER ====================
void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == LOW && lastButtonState == HIGH) {
      Serial.println("\n[BUTTON] ⚡ Manual poll triggered!");
      pollAllSources();
      
      // Stay in fast mode after button press
      fastUntil = millis() + FAST_WINDOW_MS;
      nextPollAt = millis() + FAST_INTERVAL;

      // Wait for release
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
      delay(80);
    }
  }
  lastButtonState = reading;
}

// ==================== POLL ALL SOURCES ====================
void pollAllSources() {
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  POLLING ALL SOURCES");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  pollLEDStatus();
  delay(100);
  
  pollRGBValues();
  
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  POLL COMPLETE");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ==================== POLL LED STATUS ====================
void pollLEDStatus() {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[LED] ✗ No WiFi - skipping");
    return;
  }

  Serial.println("[LED] Checking led_control.php...");

  static WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(7000);
  http.setReuse(true);

  String url = String(LED_URL) + "?t=" + String(millis());

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

    bool anyChange = false;

    if (ok1 && newLed1 != led1State) {
      led1State = newLed1;
      digitalWrite(LED1_PIN, led1State ? HIGH : LOW);
      anyChange = true;
      Serial.print("[LED] LED1 → ");
      Serial.println(led1State ? "ON ✓" : "OFF");
    }

    if (ok2 && newLed2 != led2State) {
      led2State = newLed2;
      digitalWrite(LED2_PIN, led2State ? HIGH : LOW);
      anyChange = true;
      Serial.print("[LED] LED2 → ");
      Serial.println(led2State ? "ON ✓" : "OFF");
    }

    // Check timestamp
    bool tsOk = false;
    String ts = getJsonString(body, "timestamp", tsOk);
    if (tsOk && ts.length() && ts != lastLedTimestamp) {
      lastLedTimestamp = ts;
      anyChange = true;
    }

    if (anyChange) {
      fastUntil = millis() + FAST_WINDOW_MS;
      nextPollAt = millis() + FAST_INTERVAL;
      Serial.println("[LED] ✓ Status updated!");
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

// ==================== POLL RGB VALUES ====================
void pollRGBValues() {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[RGB] ✗ No WiFi - skipping");
    return;
  }

  Serial.println("[RGB] Checking rgb_proxy.php...");

  // Use WiFiClientSecure with setInsecure() - ignore certificate validation
  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 512);  // Reduce buffer size for stability

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setReuse(false);

  String url = String(RGB_URL) + "?t=" + String(millis());

  if (!http.begin(client, url)) {
    Serial.println("[RGB] ✗ HTTP begin failed");
    return;
  }

  // Add headers to prevent HTML response
  http.addHeader("Accept", "text/plain");
  http.addHeader("User-Agent", "ESP8266");

  Serial.println("[RGB] Sending GET request...");
  int code = http.GET();
  
  Serial.print("[RGB] Response code: ");
  Serial.println(code);

  if (code == HTTP_CODE_OK || code == 200) {
    String body = http.getString();
    
    // Debug: show first 100 chars
    Serial.print("[RGB] Response preview: ");
    Serial.println(body.substring(0, min(100, (int)body.length())));
    
    body.trim();
    
    // Check if response is HTML (redirect page)
    if (body.indexOf("<html") >= 0 || body.indexOf("<!DOCTYPE") >= 0) {
      Serial.println("[RGB] ✗ Got HTML instead of data (redirect issue)");
      Serial.println("[RGB] → Server is still forcing redirect");
      http.end();
      return;
    }

    // Check if data changed
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
          
          Serial.print("[RGB] ✓ Values updated: R=");
          Serial.print(rgbRed);
          Serial.print(", G=");
          Serial.print(rgbGreen);
          Serial.print(", B=");
          Serial.println(rgbBlue);
          
          setRGBColor(rgbRed, rgbGreen, rgbBlue);
          
          fastUntil = millis() + FAST_WINDOW_MS;
          nextPollAt = millis() + FAST_INTERVAL;
        } else {
          Serial.println("[RGB] ✓ No changes");
        }
      } else {
        Serial.print("[RGB] ✗ Invalid format: ");
        Serial.println(body);
      }
    } else if (body.length() >= 50) {
      Serial.println("[RGB] ✗ Response too long (probably HTML error page)");
    } else {
      Serial.println("[RGB] ✓ No changes");
    }
  } else {
    Serial.print("[RGB] ✗ HTTP error: ");
    Serial.println(code);
  }

  http.end();

}
