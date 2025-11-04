/**
 * ESP8266 LED Remote Control — Adaptive Polling
 * ----------------------------------------------
 * - Polls fast (~1.0s) for a short window after any change,
 *   then backs off to slow (~30s) while idle.
 * - Robust JSON parsing (tolerates spaces/newlines/casing).
 * - HTTPS GET with cache-busting; reuses TLS connection when possible.
 *
 * Pins:
 *   LED1 -> GPIO12 (D6)
 *   LED2 -> GPIO13 (D7)
 *   Button -> GPIO0 (D3) with INPUT_PULLUP (LOW = pressed)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// ================== WIFI (edit to your network) ==================
const char* WIFI_SSID     = "TP-Link_C6AF";
const char* WIFI_PASSWORD = "Mistygiant753";

// ================== SERVER ==================
const char* SERVER_URL = "https://huynguyen.co/led_control.php";

// ================== GPIO ==================
const int LED1_PIN   = 12;   // D6
const int LED2_PIN   = 13;   // D7
const int BUTTON_PIN = 0;    // D3 (boot strap; don't hold LOW at boot)

// ================== ADAPTIVE POLLING ==================
const unsigned long FAST_INTERVAL = 1000;    // 1.0s after change/activity
const unsigned long SLOW_INTERVAL = 10000;   // 10s when idle
const unsigned long FAST_WINDOW_MS = 30000;  // stay fast for 30s after change

// ================== STATE ==================
bool led1State = false;
bool led2State = false;

unsigned long nextPollAt = 0;
unsigned long fastUntil  = 0;

String lastTimestamp = "";   // server "timestamp" to detect changes

// Button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ---------- Forward Declarations ----------
void handleButton();
void pollServer();

// ---------- Small helpers ----------
static String upperNoSpace(String s) {
  s.replace("\r", ""); s.replace("\n", ""); s.replace("\t", " ");
  while (s.indexOf("  ") >= 0) s.replace("  ", " ");
  s.replace(" ", "");
  s.toUpperCase();
  return s;
}

// Returns true if key's value is "ON"; false if "OFF"; ok=false if not found
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

  String val = s.substring(q1 + 1, q2); // ON/OFF (already upper/trimmed)
  ok = (val == "ON" || val == "OFF");
  return (val == "ON");
}

// Extracts a string value for a given lower-case key from *original* JSON.
// Returns empty string and ok=false if not found.
static String getJsonString(const String& json, const char* key, bool& ok) {
  ok = false;
  // Find key in original (expects lower-case key names as sent by PHP)
  String needle = String("\"") + key + String("\"");
  int pos = json.indexOf(needle);
  if (pos < 0) return "";

  int colon = json.indexOf(':', pos + needle.length());
  if (colon < 0) return "";

  // Skip spaces
  int i = colon + 1;
  while (i < (int)json.length() && isspace(json[i])) i++;

  if (i >= (int)json.length() || json[i] != '\"') return "";
  int q1 = i;
  int q2 = json.indexOf('\"', q1 + 1);
  if (q2 < 0) return "";

  ok = true;
  return json.substring(q1 + 1, q2);
}

// ================== WiFi connect ==================
static void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect failed.");
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(9600);
  WiFi.setSleep(false);  // reduce latency on ESP8266
  delay(300);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ensureWiFi();

  // visual "ready" blink
  for (int i = 0; i < 2; ++i) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, HIGH);
    delay(120);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    delay(120);
  }

  // first poll ASAP
  nextPollAt = millis();
}

// ================== LOOP ==================
void loop() {
  handleButton();

  // Choose interval based on "fast window"
  unsigned long interval = (millis() < fastUntil) ? FAST_INTERVAL : SLOW_INTERVAL;

  if ((long)(millis() - nextPollAt) >= 0) {
    pollServer();
    nextPollAt = millis() + interval;
  }

  delay(10);
}

// ================== BUTTON (immediate poll) ==================
void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == LOW && lastButtonState == HIGH) {
      Serial.println("[BUTTON] Immediate poll");
      pollServer();
      // Stay in fast mode for a bit after manual interaction
      fastUntil = millis() + FAST_WINDOW_MS;
      // Reset schedule to poll again quickly
      nextPollAt = millis() + FAST_INTERVAL;

      // Wait for release (basic de-bounce)
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
      delay(80);
    }
  }
  lastButtonState = reading;
}

// ================== SERVER POLL ==================
void pollServer() {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi; skipping poll.");
    return;
  }

  static WiFiClientSecure client; // static -> allow connection reuse
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(7000);
  http.setReuse(true);  // try to reuse TLS connection where possible

  // cache-buster to avoid stale intermediaries
  String url = String(SERVER_URL) + "?device=poll&t=" + String(millis());

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin() failed");
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    Serial.print("Response: ");
    Serial.println(body);

    // Parse LED states (tolerant)
    bool ok1 = false, ok2 = false;
    bool newLed1 = parseOnOff(body, "led1", ok1);
    bool newLed2 = parseOnOff(body, "led2", ok2);

    // Apply if present
    bool anyChange = false;

    if (ok1 && newLed1 != led1State) {
      led1State = newLed1;
      digitalWrite(LED1_PIN, led1State ? HIGH : LOW);
      anyChange = true;
      Serial.print("LED1 -> "); Serial.println(led1State ? "ON" : "OFF");
    }

    if (ok2 && newLed2 != led2State) {
      led2State = newLed2;
      digitalWrite(LED2_PIN, led2State ? HIGH : LOW);
      anyChange = true;
      Serial.print("LED2 -> "); Serial.println(led2State ? "ON" : "OFF");
    }

    // Detect server-side updates via timestamp (if provided)
    bool tsOk = false;
    String ts = getJsonString(body, "timestamp", tsOk);
    if (tsOk && ts.length() && ts != lastTimestamp) {
      lastTimestamp = ts;
      anyChange = true; // treat as activity
      Serial.print("New timestamp: ");
      Serial.println(ts);
    }

    if (anyChange) {
      // Stay in FAST mode for a short window after a change
      fastUntil = millis() + FAST_WINDOW_MS;
      // also schedule the next poll sooner to catch follow-up changes
      nextPollAt = millis() + FAST_INTERVAL;
    }

    Serial.print("State: LED1=");
    Serial.print(led1State ? "ON" : "OFF");
    Serial.print(", LED2=");
    Serial.println(led2State ? "ON" : "OFF");

  } else {
    Serial.print("HTTP error: ");
    Serial.println(code);
  }

  http.end();
}