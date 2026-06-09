#include "joystick_input.h"

static float clampFloatJoystick(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool JoystickInput::begin() {
  pinMode(JOY_SW_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  delay(500);
  calibrateCenter();

  filteredX = (float)centerX;
  filteredY = (float)centerY;

  Serial.print("Joystick center calibrated: X=");
  Serial.print(centerX);
  Serial.print(" Y=");
  Serial.println(centerY);

  return true;
}

void JoystickInput::calibrateCenter() {
  long sx = 0;
  long sy = 0;
  const int samples = 100;

  for (int i = 0; i < samples; i++) {
    sx += analogRead(JOY_X_PIN);
    sy += analogRead(JOY_Y_PIN);
    delay(5);
  }

  int cx = sx / samples;
  int cy = sy / samples;

  // Accept normal centered readings, reject only near-rail failures.
  if (cx < 400 || cx > 3700 || cy < 400 || cy > 3700) {
    centerX = 2048;
    centerY = 2048;
  } else {
    centerX = cx;
    centerY = cy;
  }
}

float JoystickInput::normalizeAxis(float raw, float center) {
  float spanPos = 4095.0f - center;
  float spanNeg = center;

  float out = 0.0f;
  if (raw >= center) {
    out = (raw - center) / (spanPos > 1.0f ? spanPos : 1.0f);
  } else {
    out = (raw - center) / (spanNeg > 1.0f ? spanNeg : 1.0f);
  }

  out = clampFloatJoystick(out, -1.0f, 1.0f);

  if (fabs(out) < JOY_DEADZONE_NORM) {
    return 0.0f;
  }

  float sign = (out >= 0.0f) ? 1.0f : -1.0f;
  float mag = fabs(out);

  mag = (mag - JOY_DEADZONE_NORM) / (1.0f - JOY_DEADZONE_NORM);
  mag = clampFloatJoystick(mag, 0.0f, 1.0f);

  mag = mag * mag;

  return sign * mag;
}

void JoystickInput::update(UIState &ui) {
  unsigned long now = millis();
  float dt = 0.02f;
  if (lastUpdateMs != 0) {
    dt = (now - lastUpdateMs) / 1000.0f;
    dt = clampFloatJoystick(dt, 0.001f, 0.25f);
  }
  lastUpdateMs = now;

  int rawX = analogRead(JOY_X_PIN);
  int rawY = analogRead(JOY_Y_PIN);
  bool btnPressed = (digitalRead(JOY_SW_PIN) == LOW);

  if (rawX < 0) rawX = 0;
  if (rawX > 4095) rawX = 4095;
  if (rawY < 0) rawY = 0;
  if (rawY > 4095) rawY = 4095;

  filteredX = filteredX * (1.0f - JOY_FILTER_ALPHA) + rawX * JOY_FILTER_ALPHA;
  filteredY = filteredY * (1.0f - JOY_FILTER_ALPHA) + rawY * JOY_FILTER_ALPHA;

  float nx = normalizeAxis(filteredX, centerX);
  float ny = normalizeAxis(filteredY, centerY);

  ui.joyRawX = rawX;
  ui.joyRawY = rawY;
  ui.joyButtonPressed = btnPressed;
  ui.joyNormX = nx;
  ui.joyNormY = ny;

  float vx = -nx * POINTER_MAX_SPEED * dt;
  float vy = -ny * POINTER_MAX_SPEED * dt;

  ui.pointer.x += vx;
  ui.pointer.y += vy;

  float minX = 0.0f;
  float maxX = (float)(TFT_W - 1);
  float minY = 34.0f;
  float maxY = (float)(TFT_H - 43);

  if (ui.currentPage == PAGE_VIBRATION) {
    minX = 260.0f;
    maxX = 459.0f;
    minY = 80.0f;
    maxY = 229.0f;
  } else if (ui.currentPage == PAGE_THERMAL) {
    minX = 10.0f;
    maxX = 329.0f;
    minY = 40.0f;
    maxY = 255.0f;
  } else if (ui.currentPage == PAGE_SOUND) {
    minX = 20.0f;
    maxX = 459.0f;
    minY = 150.0f;
    maxY = 269.0f;
  }

  ui.pointer.x = clampFloatJoystick(ui.pointer.x, minX, maxX);
  ui.pointer.y = clampFloatJoystick(ui.pointer.y, minY, maxY);

  ui.pointer.xi = (int)(ui.pointer.x + 0.5f);
  ui.pointer.yi = (int)(ui.pointer.y + 0.5f);

  bool currentState = btnPressed ? LOW : HIGH;

  if (currentState != lastBtnState && (now - lastButtonChangeMs) > JOY_DEBOUNCE_MS) {
    lastButtonChangeMs = now;
    lastBtnState = currentState;

    if (currentState == LOW) {
      pressStartMs = now;
      longPressHandled = false;
    } else {
      unsigned long heldMs = now - pressStartMs;
      if (!longPressHandled && heldMs >= 20) {
        shortPressPending = true;
      }
      pressStartMs = 0;
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
