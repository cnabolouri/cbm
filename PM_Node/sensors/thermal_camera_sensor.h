#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include "../types.h"
#include "../config.h"

class ThermalCameraSensor {
public:
  bool begin();
  bool update(ThermalFrameData& out);

  void setPointer(int x, int y);

private:
  Adafruit_MLX90640 mlx;
  float frameC[THERMAL_PIXELS] = {0};
  float smoothedF[THERMAL_PIXELS] = {0};
  bool haveSmoothed = false;
  static constexpr float THERMAL_SMOOTH_ALPHA = 0.35f;

  int pointerX = THERMAL_W / 2;
  int pointerY = THERMAL_H / 2;

  float cToF(float c) const;
  int clampi(int v, int lo, int hi) const;
};
