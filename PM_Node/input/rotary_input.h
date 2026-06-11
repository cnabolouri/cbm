#pragma once
#include <Arduino.h>

class RotaryInput {
public:
  bool begin();
  void update();

  bool consumeNextPage();
  bool consumePrevPage();
  bool consumePress();

private:
  static void IRAM_ATTR isrEncoder();
  static uint8_t IRAM_ATTR readAB();

  static volatile int8_t encAccum;
  static volatile bool nextPagePending;
  static volatile bool prevPagePending;

  bool lastBtnState = HIGH;
  unsigned long lastBtnChangeMs = 0;
  bool pressPending = false;
};
