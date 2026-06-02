#include "joystick_input.h"

static float clampFloatJoystick(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool JoystickInput::begin() {
  pinMode(JOY_SW_PIN, INPUT_PULLUP);
  analogReadResolution(12);

  calibrateCenter();

  filteredX = (float)centerX;
  filteredY = (float)centerY;

  return true;
}

void JoystickInput::calibrateCenter() {
  long sx = 0;
  long sy = 0;
  const int samples = 60;

  for (int i = 0; i < samples; i++) {
    sx += analogRead(JOY_X_PIN);
    sy += analogRead(JOY_Y_PIN);
    delay(5);
  }

  centerX = (int)(sx / samples);
  centerY = (int)(sy / samples);
}

float JoystickInput::normalizeAxis(float raw, float center) {
  float spanPos = 4095.0f - center;
  float spanNeg = center;
  float out;

  if (raw >= center) {
    out = (raw - center) / (spanPos > 1.0f ? spanPos : 1.0f);
  } else {
    out = (raw - center) / (spanNeg > 1.0f ? spanNeg : 1.0f);
  }

  out = clampFloatJoystick(out, -1.0f, 1.0f);

  float absOut = fabs(out);
  if (absOut <= JOY_DEADZONE_NORM) {
    return 0.0f;
  }

  float sign = (out >= 0.0f) ? 1.0f : -1.0f;
  float mag = (absOut - JOY_DEADZONE_NORM) / (1.0f - JOY_DEADZONE_NORM);
  mag = clampFloatJoystick(mag, 0.0f, 1.0f);

  // softer curve for finer control
  mag = mag * mag;

  return sign * mag;
}

void JoystickInput::update(UIState& ui) {
  int rawX = analogRead(JOY_X_PIN);
  int rawY = analogRead(JOY_Y_PIN);
  bool btnPressed = (digitalRead(JOY_SW_PIN) == LOW);

  filteredX = filteredX * (1.0f - JOY_FILTER_ALPHA) + rawX * JOY_FILTER_ALPHA;
  filteredY = filteredY * (1.0f - JOY_FILTER_ALPHA) + rawY * JOY_FILTER_ALPHA;

  float nx = normalizeAxis(filteredX, centerX);
  float ny = normalizeAxis(filteredY, centerY);

  // current mapping chosen so pointer follows joystick direction
  float vx = nx * POINTER_MAX_SPEED;
  float vy = -ny * POINTER_MAX_SPEED;

  ui.pointer.x += vx;
  ui.pointer.y += vy;

  ui.pointer.x = clampFloatJoystick(ui.pointer.x, 0.0f, (float)(TFT_W - 1));
  ui.pointer.y = clampFloatJoystick(ui.pointer.y, 34.0f, (float)(TFT_H - 43));

  ui.pointer.xi = (int)(ui.pointer.x + 0.5f);
  ui.pointer.yi = (int)(ui.pointer.y + 0.5f);

  bool currentState = btnPressed ? LOW : HIGH;
  unsigned long now = millis();

  if (currentState != lastBtnState && (now - lastButtonChangeMs) > JOY_DEBOUNCE_MS) {
    lastButtonChangeMs = now;
    lastBtnState = currentState;

    if (currentState == LOW) {
      pressStartMs = now;
      longPressHandled = false;
    } else {
      if (!longPressHandled) {
        shortPressPending = true;
      }
    }
  }

  if (currentState == LOW && !longPressHandled) {
    if (now - pressStartMs >= JOY_LONG_PRESS_MS) {
      longPressHandled = true;
      longPressPending = true;
    }
  }
}

bool JoystickInput::consumeShortPress() {
  bool value = shortPressPending;
  shortPressPending = false;
  return value;
}

bool JoystickInput::consumeLongPress() {
  bool value = longPressPending;
  longPressPending = false;
  return value;
}
