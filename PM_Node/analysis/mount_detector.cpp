#include "mount_detector.h"
#include <math.h>

void MountDetector::begin() {
  initialized = false;
  state = MOUNT_UNKNOWN;
  stableStartMs = 0;
  lastUpdateMs = millis();
}

float MountDetector::mag3(float x, float y, float z) const {
  return sqrtf(x * x + y * y + z * z);
}

float MountDetector::clampf(float v, float lo, float hi) const {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void MountDetector::update(float rawXg, float rawYg, float rawZg, MountData& out) {
  unsigned long now = millis();
  if (!initialized) {
    baseX = prevBaseX = rawXg;
    baseY = prevBaseY = rawYg;
    baseZ = prevBaseZ = rawZg;
    initialized = true;
    state = MOUNT_STABILIZING;
    stableStartMs = now;
  }

  baseX = BASELINE_ALPHA * baseX + (1.0f - BASELINE_ALPHA) * rawXg;
  baseY = BASELINE_ALPHA * baseY + (1.0f - BASELINE_ALPHA) * rawYg;
  baseZ = BASELINE_ALPHA * baseZ + (1.0f - BASELINE_ALPHA) * rawZg;

  float dynX = rawXg - baseX;
  float dynY = rawYg - baseY;
  float dynZ = rawZg - baseZ;
  float dynamicG = mag3(dynX, dynY, dynZ);

  float driftX = baseX - prevBaseX;
  float driftY = baseY - prevBaseY;
  float driftZ = baseZ - prevBaseZ;
  float driftG = mag3(driftX, driftY, driftZ);

  prevBaseX = baseX;
  prevBaseY = baseY;
  prevBaseZ = baseZ;
  lastUpdateMs = now;

  bool movingNow = (dynamicG > MOVING_DYNAMIC_G) || (driftG > MOVING_DRIFT_G);

  switch (state) {
    case MOUNT_UNKNOWN:
      state = movingNow ? MOUNT_MOVING : MOUNT_STABILIZING;
      stableStartMs = now;
      break;

    case MOUNT_MOVING:
      if (!movingNow) {
        state = MOUNT_STABILIZING;
        stableStartMs = now;
      }
      break;

    case MOUNT_STABILIZING:
      if (movingNow) {
        state = MOUNT_MOVING;
      } else if (now - stableStartMs >= REQUIRED_STABLE_MS) {
        state = MOUNT_MONITORING;
      }
      break;

    case MOUNT_MONITORING:
      if (dynamicG > MONITOR_BREAK_G || driftG > MOVING_DRIFT_G * 1.5f) {
        state = MOUNT_MOVING;
        stableStartMs = now;
      }
      break;
  }

  float conf = 0.0f;
  if (state == MOUNT_MOVING) {
    conf = 10.0f;
  } else if (state == MOUNT_STABILIZING) {
    float frac = clampf((float)(now - stableStartMs) / (float)REQUIRED_STABLE_MS, 0.0f, 1.0f);
    conf = 30.0f + frac * 55.0f;
  } else if (state == MOUNT_MONITORING) {
    conf = 95.0f;
  } else {
    conf = 20.0f;
  }

  out.state = state;
  out.confidence = conf;
  out.dynamicG = dynamicG;
  out.driftG = driftG;
  out.stableMs = (state == MOUNT_STABILIZING || state == MOUNT_MONITORING) ? (now - stableStartMs) : 0;
  out.analysisTrusted = (state == MOUNT_MONITORING);
}
