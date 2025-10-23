// ============================================================================
// switches.cpp
// ============================================================================
// Purpose: Input device detection with software debouncing
// Features: Non-blocking polling, event flags, 50ms debounce period
// Manages: node_1 (button) and node_2 (tilt) activity counters
// ============================================================================

#include "switches.h"
#include "config.h"

static volatile bool btnEvt = false;
static volatile bool tiltEvt = false;
static uint32_t n1 = 0, n2 = 0;

void switchesBegin() {
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_TILT, INPUT);
}

void pollSwitches() {
  static uint8_t lastBtn = HIGH, lastTilt = HIGH;
  static uint32_t tBtn = 0, tTilt = 0;
  uint32_t now = millis();

  // Button debounce
  uint8_t btn = digitalRead(PIN_BUTTON);
  if (btn != lastBtn && (now - tBtn) > 50) {
    tBtn = now;
    lastBtn = btn;
    if (btn == LOW) {
      btnEvt = true;
      Serial.println("[SW] Button pressed -> node_1");
    }
  }

  // Tilt switch debounce
  uint8_t tilt = digitalRead(PIN_TILT);
  if (tilt != lastTilt && (now - tTilt) > 50) {
    tTilt = now;
    lastTilt = tilt;
    if (tilt == LOW) {
      tiltEvt = true;
      Serial.println("[SW] Tilt detected -> node_2");
    }
  }
}

bool takeButtonEvent() {
  bool e = btnEvt;
  btnEvt = false;
  return e;
}

bool takeTiltEvent() {
  bool e = tiltEvt;
  tiltEvt = false;
  return e;
}

uint32_t node1Count() { return n1; }
uint32_t node2Count() { return n2; }
void incNode1() { n1++; }
void incNode2() { n2++; }
