#pragma once
#include <Arduino.h>
#include "../types.h"

class FFTAnalyzer {
public:
  void computeSoundSpectrum(const int32_t* samples, int count, float sampleRate, SoundSpectrumData& out);

private:
  static const int FFT_INPUT_LIMIT = 1024;

  float windowed[FFT_INPUT_LIMIT] = {0};

  void applyHannWindow(const int32_t* in, int count);
  void pickTopPeaks(const float* hz, const float* mag, int bins, SpectrumPeak peaks[3], bool& harmonic2, bool& harmonic3);
};
