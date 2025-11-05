// ============================================================================
// switches.cpp - Dual Switch Implementation - IMPROVED RESPONSIVENESS
// ============================================================================

#include "switches.h"
#include "config.h"

// Event flags (set by pollSwitches, cleared by takeXxxEvent)
static volatile bool sw1Evt = false;
static volatile bool sw2Evt = false;

// Activity counters
static uint32_t count1 = 0;
static uint32_t count2 = 0;

// Debouncing state
static uint8_t sw1State = HIGH;
static uint8_t sw2State = HIGH;
static uint8_t sw1LastReading = HIGH;
static uint8_t sw2LastReading = HIGH;
static uint32_t sw1LastDebounceTime = 0;
static uint32_t sw2LastDebounceTime = 0;

/**
 * Initialize GPIO pins for switches
 */
void switchesBegin() {
  pinMode(PIN_SWITCH_1, INPUT_PULLUP);  // GPIO0 has internal pull-up
  pinMode(PIN_SWITCH_2, INPUT_PULLUP);  // GPIO16 needs explicit pull-up
  
  // Read initial state
  sw1State = digitalRead(PIN_SWITCH_1);
  sw2State = digitalRead(PIN_SWITCH_2);
  sw1LastReading = sw1State;
  sw2LastReading = sw2State;
  
  Serial.println("[SWITCHES] Initialized:");
  Serial.println("  Switch 1 (GPIO0)  -> Sensor logging");
  Serial.println("  Switch 2 (GPIO16) -> LED/RGB status");
}

/**
 * Poll both switches with improved debouncing
 * Call this frequently in the main loop
 */
void pollSwitches() {
  uint32_t now = millis();
  
  // ====== SWITCH 1 (GPIO0) ======
  uint8_t reading1 = digitalRead(PIN_SWITCH_1);
  
  // Check if reading changed (potential bounce or real change)
  if (reading1 != sw1LastReading) {
    sw1LastDebounceTime = now;  // Reset debounce timer
  }
  
  // If reading has been stable for debounce period
  if ((now - sw1LastDebounceTime) > DEBOUNCE_DELAY_MS) {
    // If the reading is different from the current state, update state
    if (reading1 != sw1State) {
      sw1State = reading1;
      
      // Trigger event on press (falling edge, LOW = pressed)
      if (sw1State == LOW) {
        sw1Evt = true;
        Serial.println("\n[SWITCH 1] ✓ Pressed -> Sensor Logging");
      }
    }
  }
  
  sw1LastReading = reading1;
  
  // ====== SWITCH 2 (GPIO16) ======
  uint8_t reading2 = digitalRead(PIN_SWITCH_2);
  
  if (reading2 != sw2LastReading) {
    sw2LastDebounceTime = now;
  }
  
  if ((now - sw2LastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading2 != sw2State) {
      sw2State = reading2;
      
      if (sw2State == LOW) {
        sw2Evt = true;
        Serial.println("\n[SWITCH 2] ✓ Pressed -> Status Check");
      }
    }
  }
  
  sw2LastReading = reading2;
}

/**
 * Take and clear switch 1 event
 */
bool takeSwitch1Event() {
  if (sw1Evt) {
    sw1Evt = false;
    return true;
  }
  return false;
}

/**
 * Take and clear switch 2 event
 */
bool takeSwitch2Event() {
  if (sw2Evt) {
    sw2Evt = false;
    return true;
  }
  return false;
}

/**
 * Check if button is currently being held down
 */
bool isSwitch1Pressed() {
  return (sw1State == LOW);
}

bool isSwitch2Pressed() {
  return (sw2State == LOW);
}

/**
 * Activity counters
 */
uint32_t switch1Count() { return count1; }
uint32_t switch2Count() { return count2; }
void incSwitch1() { count1++; }
void incSwitch2() { count2++; }
