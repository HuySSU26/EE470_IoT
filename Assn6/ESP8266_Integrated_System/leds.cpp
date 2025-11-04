// ============================================================================
// leds.cpp - LED and RGB Control Implementation
// ============================================================================

#include "leds.h"
#include "config.h"

// Blink state for non-blocking animation
struct BlinkState {
  uint8_t pin;
  uint32_t startTime;
  uint16_t blinkPeriod;
  uint16_t duration;
  bool active;
};

static BlinkState blink1 = {PIN_LED1, 0, 0, 0, false};
static BlinkState blink2 = {PIN_LED2, 0, 0, 0, false};

// Current LED states
static bool led1State = false;
static bool led2State = false;

// Current RGB values
static int rgbR = 0, rgbG = 0, rgbB = 0;

/**
 * Initialize all LED pins
 */
void ledsBegin() {
  // Digital LEDs
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  
  // RGB LED (PWM pins)
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  setRGBColor(0, 0, 0);  // Start with RGB off
  
  Serial.println("[LEDS] Initialized:");
  Serial.println("  LED1 (GPIO12) | LED2 (GPIO13)");
  Serial.println("  RGB: R=GPIO15, G=GPIO4, B=GPIO5");
}

/**
 * Helper to handle one blink state
 */
static void handleBlink(BlinkState& bs) {
  if (!bs.active) return;
  
  uint32_t elapsed = millis() - bs.startTime;
  
  if (elapsed >= bs.duration) {
    // Blink sequence complete - restore original state
    bs.active = false;
    if (bs.pin == PIN_LED1) {
      digitalWrite(PIN_LED1, led1State ? HIGH : LOW);
    } else if (bs.pin == PIN_LED2) {
      digitalWrite(PIN_LED2, led2State ? HIGH : LOW);
    }
    return;
  }
  
  // Toggle based on blink period
  uint16_t phase = elapsed % (bs.blinkPeriod * 2);
  bool blinkOn = (phase < bs.blinkPeriod);
  digitalWrite(bs.pin, blinkOn ? HIGH : LOW);
}

/**
 * Poll all blink states (non-blocking)
 */
void ledsPoll() {
  handleBlink(blink1);
  handleBlink(blink2);
}

/**
 * Start async blink on specified pin
 */
void blinkAsync(uint8_t pin, uint16_t blink_ms, uint16_t duration_ms) {
  BlinkState* bs = nullptr;
  
  if (pin == PIN_LED1) {
    bs = &blink1;
  } else if (pin == PIN_LED2) {
    bs = &blink2;
  }
  
  if (bs) {
    bs->pin = pin;
    bs->startTime = millis();
    bs->blinkPeriod = blink_ms;
    bs->duration = duration_ms;
    bs->active = true;
  }
}

/**
 * Set RGB color using PWM
 */
void setRGBColor(int r, int g, int b) {
  // Constrain values to 0-255
  rgbR = constrain(r, 0, 255);
  rgbG = constrain(g, 0, 255);
  rgbB = constrain(b, 0, 255);
  
  // ESP8266 PWM range is 0-1023
  analogWrite(RGB_RED_PIN,   map(rgbR, 0, 255, 0, 1023));
  analogWrite(RGB_GREEN_PIN, map(rgbG, 0, 255, 0, 1023));
  analogWrite(RGB_BLUE_PIN,  map(rgbB, 0, 255, 0, 1023));
}

/**
 * Get current RGB values
 */
void getRGBColor(int& r, int& g, int& b) {
  r = rgbR;
  g = rgbG;
  b = rgbB;
}

/**
 * Set digital LED state
 */
void setLED(uint8_t pin, bool state) {
  if (pin == PIN_LED1) {
    led1State = state;
    if (!blink1.active) {  // Only apply if not blinking
      digitalWrite(PIN_LED1, state ? HIGH : LOW);
    }
  } else if (pin == PIN_LED2) {
    led2State = state;
    if (!blink2.active) {  // Only apply if not blinking
      digitalWrite(PIN_LED2, state ? HIGH : LOW);
    }
  }
}

/**
 * Get digital LED state
 */
bool getLED(uint8_t pin) {
  if (pin == PIN_LED1) return led1State;
  if (pin == PIN_LED2) return led2State;
  return false;
}
