#include "touch.h"

TouchInput::TouchInput(
  XPT2046_Touchscreen& ts,
  int screenW,
  int screenH,
  int minX,
  int maxX,
  int minY,
  int maxY,
  bool swapXY,
  bool invertX,
  bool invertY
) : ts(ts),
    screenW(screenW),
    screenH(screenH),
    minX(minX),
    maxX(maxX),
    minY(minY),
    maxY(maxY),
    swapXY(swapXY),
    invertX(invertX),
    invertY(invertY) {}

void TouchInput::begin() {
}

bool TouchInput::readMappedPoint(int &sx, int &sy) {
  if (!ts.touched()) return false;

  TS_Point p = ts.getPoint();

  int x, y;
  if (swapXY) {
    x = map(p.y, minY, maxY, 0, screenW - 1);
    y = map(p.x, minX, maxX, 0, screenH - 1);
  } else {
    x = map(p.x, minX, maxX, 0, screenW - 1);
    y = map(p.y, minY, maxY, 0, screenH - 1);
  }

  if (invertX) x = (screenW - 1) - x;
  if (invertY) y = (screenH - 1) - y;

  sx = constrain(x, 0, screenW - 1);
  sy = constrain(y, 0, screenH - 1);
  return true;
}

TouchInput::EventType TouchInput::update() {
  int x, y;
  if (!readMappedPoint(x, y)) {
    touchPressed = false;
    return TOUCH_NONE;
  }

  touchX = x;
  touchY = y;
  touchPressed = true;

  if (millis() - lastTouchMs < 250) return TOUCH_NONE;
  lastTouchMs = millis();

  if (y >= 200) {
    if (x < 64) return TOUCH_TAB_VIB;
    if (x < 128) return TOUCH_TAB_TEMP;
    if (x < 192) return TOUCH_TAB_THERMAL;
    if (x < 256) return TOUCH_TAB_SOUND;
    return TOUCH_TAB_SYS;
  }

  if (x < 30) return TOUCH_PREV_PAGE;
  if (x > 290) return TOUCH_NEXT_PAGE;

  return TOUCH_NONE;
}

int TouchInput::lastX() const {
  return touchX;
}

int TouchInput::lastY() const {
  return touchY;
}

bool TouchInput::isPressed() const {
  return touchPressed;
}
