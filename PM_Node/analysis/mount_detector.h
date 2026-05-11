#pragma once
#include <Arduino.h>
#include "../types.h"

class MountDetector {
public:
  void begin();
  void update(float rawXg, float rawYg, float rawZg, MountData& out);

private:
  float baseX = 0.0f;
  float baseY = 0.0f;
  float baseZ = 0.0f;

  float prevBaseX = 0.0f;
  float prevBaseY = 0.0f;
  float prevBaseZ = 0.0f;

  bool initialized = false;

  unsigned long stableStartMs = 0;
  unsigned long lastUpdateMs = 0;

  MountState state = MOUNT_UNKNOWN;

  static constexpr float BASELINE_ALPHA = 0.995f;
  static constexpr float MOVING_DYNAMIC_G = 0.20f;
  static constexpr float MOVING_DRIFT_G = 0.04f;
  static constexpr float MONITOR_BREAK_G = 0.35f;
  static constexpr unsigned long REQUIRED_STABLE_MS = 5000;

  float mag3(float x, float y, float z) const;
  float clampf(float v, float lo, float hi) const;
};
