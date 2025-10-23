// ============================================================================
// leds.cpp
// ============================================================================
// Purpose: Non-blocking LED blink controller implementation
// Features: Independent state management, millis()-based timing
// Behavior: LED1 blinks on node_1 success, LED2 blinks on node_2 success
// Duration: 2-second visual confirmation per transmission
// ============================================================================

#include "leds.h"
#include "config.h"

struct LedState {
  uint8_t pin;
  uint32_t startTime;
  uint16_t periodMs;      // How long one on/off cycle takes
  uint16_t onTimeMs;      // How long LED stays ON per cycle
  uint16_t totalTimeMs;   // Total duration of blinking
  uint8_t blinkCount;     // How many times to blink
  uint8_t currentBlink;   // Current blink number
  bool active;
  bool ledOn;
};

static LedState led1State = {PIN_LED1, 0, 0, 0, 0, 0, 0, false, false};
static LedState led2State = {PIN_LED2, 0, 0, 0, 0, 0, 0, false, false};

void ledsBegin() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  
  Serial.println("[LED] Initialized GPIO12 and GPIO13");
}

static void updateLed(LedState& state) {
  if (!state.active) return;

  uint32_t elapsed = millis() - state.startTime;

  // Check if total time expired
  if (elapsed >= state.totalTimeMs) {
    digitalWrite(state.pin, LOW);
    state.active = false;
    state.ledOn = false;
    return;
  }

  // Calculate position within current blink cycle
  uint32_t cyclePosition = elapsed % state.periodMs;

  // Turn LED on/off based on cycle position
  if (cyclePosition < state.onTimeMs) {
    if (!state.ledOn) {
      digitalWrite(state.pin, HIGH);
      state.ledOn = true;
    }
  } else {
    if (state.ledOn) {
      digitalWrite(state.pin, LOW);
      state.ledOn = false;
    }
  }
}

void ledsPoll() {
  updateLed(led1State);
  updateLed(led2State);
}

void blinkAsync(uint8_t pin, uint16_t onTime, uint16_t totalTime) {
  LedState* state = nullptr;
  
  if (pin == PIN_LED1) {
    state = &led1State;
    Serial.print("[LED] Starting blink on GPIO12: ");
  } else if (pin == PIN_LED2) {
    state = &led2State;
    Serial.print("[LED] Starting blink on GPIO13: ");
  }
  
  if (state) {
    state->startTime = millis();
    state->onTimeMs = onTime;
    state->totalTimeMs = totalTime;
    state->periodMs = onTime * 2;  // On time + off time
    state->active = true;
    state->ledOn = false;
    
    Serial.print(onTime);
    Serial.print("ms ON, ");
    Serial.print(totalTime);
    Serial.println("ms total");
    
    // Immediately turn on the LED
    digitalWrite(pin, HIGH);
    state->ledOn = true;
  }
}