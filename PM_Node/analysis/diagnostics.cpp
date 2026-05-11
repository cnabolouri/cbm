#include "diagnostics.h"
#include <math.h>

void DiagnosticsEngine::push(float* arr, int len, int& head, float value) {
  arr[head] = value;
  head = (head + 1) % len;
}

float DiagnosticsEngine::avg(const float* arr, int len) const {
  int n = sampleCount < len ? sampleCount : len;
  if (n <= 0) return 0.0f;

  float s = 0.0f;
  for (int i = 0; i < n; i++) s += arr[i];
  return s / n;
}

float DiagnosticsEngine::maxv(const float* arr, int len) const {
  int n = sampleCount < len ? sampleCount : len;
  if (n <= 0) return 0.0f;

  float m = arr[0];
  for (int i = 1; i < n; i++) {
    if (arr[i] > m) m = arr[i];
  }
  return m;
}

void DiagnosticsEngine::update(LiveData& live) {
  push(vibHist, HISTORY_LEN, vibHead, live.vibration.total_in_s);
  push(deltaHist, HISTORY_LEN, deltaHead, live.temperature.deltaF);
  push(dbHist, HISTORY_LEN, dbHead, live.sound.db);
  if (sampleCount < HISTORY_LEN) sampleCount++;

  live.analysis.vibration.currentTotal = live.vibration.total_in_s;
  live.analysis.vibration.maxTotal = maxv(vibHist, HISTORY_LEN);
  live.analysis.vibration.avgTotal = avg(vibHist, HISTORY_LEN);

  float ax = fabs(live.vibration.x_in_s);
  float ay = fabs(live.vibration.y_in_s);
  float az = fabs(live.vibration.z_in_s);

  if (ax >= ay && ax >= az) live.analysis.vibration.dominantAxis = "X";
  else if (ay >= ax && ay >= az) live.analysis.vibration.dominantAxis = "Y";
  else live.analysis.vibration.dominantAxis = "Z";

  if (live.vibration.total_in_s < 0.05f) live.analysis.vibration.alert = "Normal";
  else if (live.vibration.total_in_s < 0.15f) live.analysis.vibration.alert = "Watch";
  else live.analysis.vibration.alert = "High";

  live.analysis.temperature.currentDelta = live.temperature.deltaF;
  live.analysis.temperature.maxDelta = maxv(deltaHist, HISTORY_LEN);
  live.analysis.temperature.avgDelta = avg(deltaHist, HISTORY_LEN);
  live.analysis.temperature.risingFast = (live.temperature.deltaF - live.analysis.temperature.avgDelta) > 3.0f;

  if (live.temperature.deltaF < 5.0f) live.analysis.temperature.alert = "Normal";
  else if (live.temperature.deltaF < 15.0f) live.analysis.temperature.alert = "Watch";
  else live.analysis.temperature.alert = "High";

  live.analysis.sound.currentDb = live.sound.db;
  live.analysis.sound.maxDb = maxv(dbHist, HISTORY_LEN);
  live.analysis.sound.avgDb = avg(dbHist, HISTORY_LEN);
  live.analysis.sound.dominantHz = live.sound.hz;

  if (live.sound.db < 40.0f) live.analysis.sound.alert = "Normal";
  else if (live.sound.db < 55.0f) live.analysis.sound.alert = "Watch";
  else live.analysis.sound.alert = "High";
}
