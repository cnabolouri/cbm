#pragma once
#include <Arduino.h>

class VibrationSampler {
public:
  static const int BUFFER_SIZE = 256;

  void begin(float sampleRateHzIn);
  void update(float rawX_g, float rawY_g, float rawZ_g);

  const float* getBufferX() const;
  const float* getBufferY() const;
  const float* getBufferZ() const;
  const float* getBufferMag() const;
  int getCount() const;
  int getHead() const;
  float getSampleRateHz() const;
  bool isFull() const;

private:
  float bufferX[BUFFER_SIZE] = {0};
  float bufferY[BUFFER_SIZE] = {0};
  float bufferZ[BUFFER_SIZE] = {0};
  float bufferMag[BUFFER_SIZE] = {0};
  int count = 0;
  int head = 0;
  float sampleRateHz = 200.0f;
};
