// ============================================================================
// switches.cpp - Dual Switch Implementation
// ============================================================================

#include "switches.h"
#include "config.h"

// Event flags (set by pollSwitches, cleared by takeXxxEvent)
static volatile bool sw1Evt = false;
static volatile bool sw2Evt = false;

// Activity counters
static uint32_t count1 = 0;
static uint32_t count2 = 0;

/**
 * Initialize GPIO pins for switches
 */
void switchesBegin() {
  pinMode(PIN_SWITCH_1, INPUT_PULLUP);  // GPIO0 has internal pull-up
  pinMode(PIN_SWITCH_2, INPUT_PULLUP);  // GPIO16 needs explicit pull-up
  
  Serial.println("[SWITCHES] Initialized:");
  Serial.println("  Switch 1 (GPIO0)  -> Part 1: Sensor logging");
  Serial.println("  Switch 2 (GPIO16) -> Part 2: LED/RGB status");
}

/**
 * Poll both switches with debouncing
 * Call this every loop iteration
 */
void pollSwitches() {
  static uint8_t lastSw1 = HIGH, lastSw2 = HIGH;
  static uint32_t timeSw1 = 0, timeSw2 = 0;
  uint32_t now = millis();

  // Poll Switch 1 (GPIO0)
  uint8_t sw1 = digitalRead(PIN_SWITCH_1);
  if (sw1 != lastSw1 && (now - timeSw1) > DEBOUNCE_DELAY_MS) {
    timeSw1 = now;
    lastSw1 = sw1;
    if (sw1 == LOW) {  // Button pressed (active low with pull-up)
      sw1Evt = true;
      Serial.println("\n[SWITCH 1] Pressed -> Part 1 Triggered");
    }
  }

  // Poll Switch 2 (GPIO16)
  uint8_t sw2 = digitalRead(PIN_SWITCH_2);
  if (sw2 != lastSw2 && (now - timeSw2) > DEBOUNCE_DELAY_MS) {
    timeSw2 = now;
    lastSw2 = sw2;
    if (sw2 == LOW) {  // Button pressed (active low with pull-up)
      sw2Evt = true;
      Serial.println("\n[SWITCH 2] Pressed -> Part 2 Triggered");
    }
  }
}

/**
 * Take and clear switch 1 event
 */
bool takeSwitch1Event() {
  bool evt = sw1Evt;
  sw1Evt = false;
  return evt;
}

/**
 * Take and clear switch 2 event
 */
bool takeSwitch2Event() {
  bool evt = sw2Evt;
  sw2Evt = false;
  return evt;
}

/**
 * Activity counters
 */
uint32_t switch1Count() { return count1; }
uint32_t switch2Count() { return count2; }
void incSwitch1() { count1++; }
void incSwitch2() { count2++; }
