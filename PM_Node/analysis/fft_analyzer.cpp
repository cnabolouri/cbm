#include "fft_analyzer.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

void FFTAnalyzer::applyHannWindow(const int32_t* in, int count) {
  if (count > FFT_INPUT_LIMIT) count = FFT_INPUT_LIMIT;
  if (count <= 1) return;

  for (int n = 0; n < count; n++) {
    float w = 0.5f * (1.0f - cosf((2.0f * PI * n) / (count - 1)));
    windowed[n] = in[n] * w;
  }
}

void FFTAnalyzer::pickTopPeaks(const float* hz, const float* mag, int bins, SpectrumPeak peaks[3], bool& harmonic2, bool& harmonic3) {
  for (int i = 0; i < 3; i++) {
    peaks[i] = SpectrumPeak{};
  }

  for (int i = 1; i < bins; i++) {
    float m = mag[i];
    if (m > peaks[0].mag) {
      peaks[2] = peaks[1];
      peaks[1] = peaks[0];
      peaks[0].hz = hz[i];
      peaks[0].mag = m;
    } else if (m > peaks[1].mag) {
      peaks[2] = peaks[1];
      peaks[1].hz = hz[i];
      peaks[1].mag = m;
    } else if (m > peaks[2].mag) {
      peaks[2].hz = hz[i];
      peaks[2].mag = m;
    }
  }

  harmonic2 = false;
  harmonic3 = false;

  if (peaks[0].hz > 1.0f) {
    if (peaks[1].hz > 0.0f) {
      float ratio2 = peaks[1].hz / peaks[0].hz;
      harmonic2 = fabsf(ratio2 - 2.0f) < 0.20f;
    }
    if (peaks[2].hz > 0.0f) {
      float ratio3 = peaks[2].hz / peaks[0].hz;
      harmonic3 = fabsf(ratio3 - 3.0f) < 0.30f;
    }
  }
}

void FFTAnalyzer::computeSoundSpectrum(const int32_t* samples, int count, float sampleRate, SoundSpectrumData& out) {
  out = SoundSpectrumData{};

  if (!samples || count < 64 || sampleRate <= 0.0f) return;
  if (count > FFT_INPUT_LIMIT) count = FFT_INPUT_LIMIT;

  applyHannWindow(samples, count);

  int usableBins = count / 2;
  if (usableBins > SoundSpectrumData::MAX_BINS) usableBins = SoundSpectrumData::MAX_BINS;
  out.bins = usableBins;
  out.sampleRateHz = sampleRate;

  float totalWeightedMag = 0.0f;
  float totalMag = 0.0f;
  float bestAmp = 0.0f;
  float bestHz = 0.0f;

  for (int k = 1; k < usableBins; k++) {
    float real = 0.0f;
    float imag = 0.0f;

    for (int n = 0; n < count; n++) {
      float angle = (2.0f * PI * k * n) / count;
      real += windowed[n] * cosf(angle);
      imag -= windowed[n] * sinf(angle);
    }

    float mag = sqrtf(real * real + imag * imag) / count;
    float hz = (sampleRate * k) / count;

    out.hz[k] = hz;
    out.mag[k] = mag;

    if (k >= 3 && k <= 80 && mag > bestAmp) {
      bestAmp = mag;
      bestHz = hz;
    }

    if (hz < 1000.0f) out.lowBand += mag;
    else if (hz < 4000.0f) out.midBand += mag;
    else out.highBand += mag;

    totalWeightedMag += hz * mag;
    totalMag += mag;
  }

  if (totalMag > 0.0f) {
    out.centroidHz = totalWeightedMag / totalMag;
  }

  pickTopPeaks(out.hz, out.mag, out.bins, out.peaks, out.harmonic2, out.harmonic3);

  static bool havePeak = false;
  static float smoothPeakHz = 0.0f;
  if (!havePeak) {
    smoothPeakHz = bestHz;
    havePeak = true;
  } else {
    smoothPeakHz = smoothPeakHz * 0.75f + bestHz * 0.25f;
  }

  out.dominantHz = smoothPeakHz;
  out.dominantMag = bestAmp;
  out.dominantAmp = bestAmp;
  out.valid = true;

  if (out.dominantMag <= 0.0f) {
    out.character = "Quiet";
  } else {
    float tonalRatio = out.dominantMag / fmaxf(totalMag, 0.0001f);

    if (tonalRatio > 0.20f) out.character = "Tonal";
    else if (out.highBand > out.midBand && out.highBand > out.lowBand) out.character = "Sharp";
    else if (out.lowBand > out.midBand && out.lowBand > out.highBand) out.character = "Low-heavy";
    else out.character = "Broadband";
  }
}
