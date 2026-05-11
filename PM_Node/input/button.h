#pragma once
#include <Arduino.h>

class ButtonInput {
public:
  enum Event {
    NONE,
    SHORT_PRESS,
    LONG_PRESS
  };

  ButtonInput(int pin, unsigned long debounceMs, unsigned long shortPressMs, unsigned long longPressMs);

  void begin();
  Event update();
  bool isPressed() const;

private:
  int pin;
  unsigned long debounceMs;
  unsigned long shortPressMs;
  unsigned long longPressMs;

  bool lastReading = HIGH;
  bool stableState = HIGH;
  bool pressed = false;
  unsigned long lastDebounceAt = 0;
  unsigned long pressStartAt = 0;
};