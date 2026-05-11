#pragma once
#include <Arduino.h>
#include "../types.h"
#include "vibration_sampler.h"

class VibrationFFT {
public:
  void compute(const VibrationSampler& sampler, VibrationAxis axis, VibrationSpectrumData& out);

private:
  static const int VIBRATION_FFT_INPUT_LIMIT = VibrationSampler::BUFFER_SIZE;

  float ordered[VIBRATION_FFT_INPUT_LIMIT] = {0};
  float windowed[VIBRATION_FFT_INPUT_LIMIT] = {0};

  void buildOrderedBuffer(const VibrationSampler& sampler, const float* src, float* out, int count);
  void applyHannWindow(float* data, int count);
  void pickTopPeaks(const float* hz, const float* mag, int bins, SpectrumPeak peaks[3], bool& harmonic2, bool& harmonic3);
};
