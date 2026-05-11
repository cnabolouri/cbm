#include "button.h"

ButtonInput::ButtonInput(int pin, unsigned long debounceMs, unsigned long shortPressMs, unsigned long longPressMs)
  : pin(pin),
    debounceMs(debounceMs),
    shortPressMs(shortPressMs),
    longPressMs(longPressMs) {}

void ButtonInput::begin() {
  pinMode(pin, INPUT_PULLUP);
  stableState = digitalRead(pin);
  lastReading = stableState;
}

ButtonInput::Event ButtonInput::update() {
  Event evt = NONE;
  bool reading = digitalRead(pin);

  if (reading != lastReading) {
    lastDebounceAt = millis();
  }

  if ((millis() - lastDebounceAt) > debounceMs) {
    if (reading != stableState) {
      stableState = reading;

      if (stableState == LOW) {
        pressed = true;
        pressStartAt = millis();
      } else {
        if (pressed) {
          unsigned long held = millis() - pressStartAt;
          if (held >= longPressMs) evt = LONG_PRESS;
          else if (held >= shortPressMs) evt = SHORT_PRESS;
        }
        pressed = false;
      }
    }
  }

  lastReading = reading;
  return evt;
}

bool ButtonInput::isPressed() const {
  return stableState == LOW;
}