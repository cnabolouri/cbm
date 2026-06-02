#pragma once
#include <Arduino.h>
#include "../types.h"
#include "../config.h"

class JoystickInput {
public:
  bool begin();
  void update(UIState& ui);

  bool consumeShortPress();
  bool consumeLongPress();

  int getCenterX() const { return centerX; }
  int getCenterY() const { return centerY; }

private:
  int centerX = 2048;
  int centerY = 2048;

  float filteredX = 2048.0f;
  float filteredY = 2048.0f;

  bool lastBtnState = HIGH;
  unsigned long pressStartMs = 0;
  bool longPressHandled = false;
  unsigned long lastButtonChangeMs = 0;

  bool shortPressPending = false;
  bool longPressPending = false;

  void calibrateCenter();
  float normalizeAxis(float raw, float center);
};
