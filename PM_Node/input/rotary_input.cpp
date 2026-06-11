#include "rotary_input.h"
#include "../config.h"

volatile int8_t RotaryInput::encAccum = 0;
volatile bool RotaryInput::nextPagePending = false;
volatile bool RotaryInput::prevPagePending = false;

uint8_t IRAM_ATTR RotaryInput::readAB() {
  uint8_t a = digitalRead(ENC_CLK_PIN) ? 1 : 0;
  uint8_t b = digitalRead(ENC_DT_PIN) ? 1 : 0;
  return (a << 1) | b;
}

void IRAM_ATTR RotaryInput::isrEncoder() {
  static uint8_t lastAB = 0b11;

  static const int8_t transitionTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  uint8_t ab = readAB();
  uint8_t idx = (lastAB << 2) | ab;
  int8_t step = transitionTable[idx];

  if (step != 0) {
    encAccum += step;

    if (encAccum >= 4) {
      nextPagePending = true;
      encAccum = 0;
    } else if (encAccum <= -4) {
      prevPagePending = true;
      encAccum = 0;
    }
  }

  lastAB = ab;
}

bool RotaryInput::begin() {
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT_PIN), isrEncoder, CHANGE);

  lastBtnState = digitalRead(ENC_SW_PIN);
  pressPending = false;

  return true;
}

void RotaryInput::update() {
  int btnState = digitalRead(ENC_SW_PIN);
  unsigned long now = millis();

  if (btnState != lastBtnState &&
      (now - lastBtnChangeMs) > ENC_BTN_DEBOUNCE_MS) {
    lastBtnChangeMs = now;
    lastBtnState = btnState;

    if (btnState == LOW) {
      pressPending = true;
    }
  }
}

bool RotaryInput::consumeNextPage() {
  noInterrupts();
  bool v = nextPagePending;
  nextPagePending = false;
  interrupts();
  return v;
}

bool RotaryInput::consumePrevPage() {
  noInterrupts();
  bool v = prevPagePending;
  prevPagePending = false;
  interrupts();
  return v;
}

bool RotaryInput::consumePress() {
  bool v = pressPending;
  pressPending = false;
  return v;
}
