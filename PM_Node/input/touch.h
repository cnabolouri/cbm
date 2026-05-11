#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include "../types.h"

class TouchInput {
public:
  enum EventType {
    TOUCH_NONE,
    TOUCH_TAB_VIB,
    TOUCH_TAB_TEMP,
    TOUCH_TAB_SOUND,
    TOUCH_TAB_SYS,
    TOUCH_PREV_PAGE,
    TOUCH_NEXT_PAGE
  };

  TouchInput(
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
  );

  void begin();
  EventType update();
  int lastX() const;
  int lastY() const;

private:
  bool readMappedPoint(int &sx, int &sy);

  XPT2046_Touchscreen& ts;
  int screenW;
  int screenH;
  int minX;
  int maxX;
  int minY;
  int maxY;
  bool swapXY;
  bool invertX;
  bool invertY;

  unsigned long lastTouchMs = 0;
  int touchX = 0;
  int touchY = 0;
};