#include "thermal_camera_sensor.h"
#include <math.h>

bool ThermalCameraSensor::begin() {
  Wire.begin(I2C_SDA, I2C_SCL, 400000);

  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    return false;
  }

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);

  switch (MLX90640_REFRESH_HZ) {
    case 1: mlx.setRefreshRate(MLX90640_1_HZ); break;
    case 2: mlx.setRefreshRate(MLX90640_2_HZ); break;
    case 4: mlx.setRefreshRate(MLX90640_4_HZ); break;
    case 8: mlx.setRefreshRate(MLX90640_8_HZ); break;
    default: mlx.setRefreshRate(MLX90640_2_HZ); break;
  }

  return true;
}

float ThermalCameraSensor::cToF(float c) const {
  return c * 9.0f / 5.0f + 32.0f;
}

int ThermalCameraSensor::clampi(int v, int lo, int hi) const {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void ThermalCameraSensor::setPointer(int x, int y) {
  pointerX = clampi(x, 0, THERMAL_W - 1);
  pointerY = clampi(y, 0, THERMAL_H - 1);
}

bool ThermalCameraSensor::update(ThermalFrameData& out) {
  if (mlx.getFrame(frameC) != 0) {
    out.valid = false;
    return false;
  }

  out.valid = true;
  float ambientC = mlx.getTa();
  out.ambientF = cToF(ambientC);

  float localMin = 100000.0f;
  float localMax = -100000.0f;
  float localHot = -100000.0f;
  int localHotX = 0;
  int localHotY = 0;

  for (int y = 0; y < THERMAL_H; y++) {
    for (int x = 0; x < THERMAL_W; x++) {
      int idx = y * THERMAL_W + x;
      float c = frameC[idx];

      if (isnan(c) || isinf(c) || c < -100.0f || c > 1000.0f) {
        c = ambientC;
      }

      float f = cToF(c);
      if (!haveSmoothed) {
        smoothedF[idx] = f;
      } else {
        smoothedF[idx] = smoothedF[idx] * (1.0f - THERMAL_SMOOTH_ALPHA) + f * THERMAL_SMOOTH_ALPHA;
      }

      out.pixelsF[idx] = smoothedF[idx];

      if (out.pixelsF[idx] < localMin) localMin = out.pixelsF[idx];
      if (out.pixelsF[idx] > localMax) {
        localMax = out.pixelsF[idx];
        localHot = out.pixelsF[idx];
        localHotX = x;
        localHotY = y;
      }
    }
  }

  haveSmoothed = true;

  out.minF = localMin;
  out.maxF = localMax;
  out.hotspotF = localHot;
  out.hotspotX = localHotX;
  out.hotspotY = localHotY;

  int cx = THERMAL_W / 2;
  int cy = THERMAL_H / 2;
  out.centerF = out.pixelsF[cy * THERMAL_W + cx];

  out.pointerX = clampi(pointerX, 0, THERMAL_W - 1);
  out.pointerY = clampi(pointerY, 0, THERMAL_H - 1);
  out.pointerF = out.pixelsF[out.pointerY * THERMAL_W + out.pointerX];

  static bool havePointerSmooth = false;
  static float smoothPointerF = 0.0f;
  if (!havePointerSmooth) {
    smoothPointerF = out.pointerF;
    havePointerSmooth = true;
  } else {
    smoothPointerF = smoothPointerF * 0.65f + out.pointerF * 0.35f;
  }
  out.pointerDisplayF = smoothPointerF;

  return true;
}
