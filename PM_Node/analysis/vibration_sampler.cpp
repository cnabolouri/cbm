#include "vibration_sampler.h"
#include <math.h>

void VibrationSampler::begin(float sampleRateHzIn) {
  sampleRateHz = sampleRateHzIn;
  count = 0;
  head = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    bufferX[i] = 0.0f;
    bufferY[i] = 0.0f;
    bufferZ[i] = 0.0f;
    bufferMag[i] = 0.0f;
  }
}

void VibrationSampler::update(float rawX_g, float rawY_g, float rawZ_g) {
  bufferX[head] = rawX_g;
  bufferY[head] = rawY_g;
  bufferZ[head] = rawZ_g;
  bufferMag[head] = sqrtf(rawX_g * rawX_g + rawY_g * rawY_g + rawZ_g * rawZ_g);

  head = (head + 1) % BUFFER_SIZE;
  if (count < BUFFER_SIZE) count++;
}

const float* VibrationSampler::getBufferX() const {
  return bufferX;
}

const float* VibrationSampler::getBufferY() const {
  return bufferY;
}

const float* VibrationSampler::getBufferZ() const {
  return bufferZ;
}

const float* VibrationSampler::getBufferMag() const {
  return bufferMag;
}

int VibrationSampler::getCount() const {
  return count;
}

int VibrationSampler::getHead() const {
  return head;
}

float VibrationSampler::getSampleRateHz() const {
  return sampleRateHz;
}

bool VibrationSampler::isFull() const {
  return count >= BUFFER_SIZE;
}
