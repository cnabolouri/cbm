#include "vibration_fft.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

void VibrationFFT::buildOrderedBuffer(const VibrationSampler& sampler, const float* src, float* out, int count) {
  int head = sampler.getHead();
  int stored = sampler.getCount();
  int start = sampler.isFull() ? head : 0;

  for (int i = 0; i < count; i++) {
    int idx = sampler.isFull() ? ((start + i) % VibrationSampler::BUFFER_SIZE) : i;
    if (i < stored) out[i] = src[idx];
    else out[i] = 0.0f;
  }
}

void VibrationFFT::applyHannWindow(float* data, int count) {
  if (count <= 1) return;

  for (int n = 0; n < count; n++) {
    float w = 0.5f * (1.0f - cosf((2.0f * PI * n) / (count - 1)));
    data[n] *= w;
  }
}

void VibrationFFT::pickTopPeaks(const float* hz, const float* mag, int bins, SpectrumPeak peaks[3], bool& harmonic2, bool& harmonic3) {
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

void VibrationFFT::compute(const VibrationSampler& sampler, VibrationAxis axis, VibrationSpectrumData& out) {
  out = VibrationSpectrumData{};

  int count = sampler.getCount();
  if (count < 64) return;
  if (count > VIBRATION_FFT_INPUT_LIMIT) count = VIBRATION_FFT_INPUT_LIMIT;

  const float* src = sampler.getBufferX();
  out.sourceAxis = "X";

  if (axis == VIB_AXIS_Y) {
    src = sampler.getBufferY();
    out.sourceAxis = "Y";
  } else if (axis == VIB_AXIS_Z) {
    src = sampler.getBufferZ();
    out.sourceAxis = "Z";
  } else if (axis == VIB_AXIS_MAG) {
    src = sampler.getBufferMag();
    out.sourceAxis = "MAG";
  }

  buildOrderedBuffer(sampler, src, ordered, count);

  float mean = 0.0f;
  for (int i = 0; i < count; i++) mean += ordered[i];
  mean /= count;

  for (int i = 0; i < count; i++) {
    windowed[i] = ordered[i] - mean;
  }

  applyHannWindow(windowed, count);

  int usableBins = count / 2;
  if (usableBins > VibrationSpectrumData::MAX_BINS) usableBins = VibrationSpectrumData::MAX_BINS;
  out.bins = usableBins;

  float totalMag = 0.0f;

  for (int k = 1; k < usableBins; k++) {
    float real = 0.0f;
    float imag = 0.0f;

    for (int n = 0; n < count; n++) {
      float angle = (2.0f * PI * k * n) / count;
      real += windowed[n] * cosf(angle);
      imag -= windowed[n] * sinf(angle);
    }

    float mag = sqrtf(real * real + imag * imag) / count;
    float hz = (sampler.getSampleRateHz() * k) / count;

    out.hz[k] = hz;
    out.mag[k] = mag;

    if (mag > out.dominantMag) {
      out.dominantMag = mag;
      out.dominantHz = hz;
    }

    if (hz < 20.0f) out.lowBand += mag;
    else if (hz < 80.0f) out.midBand += mag;
    else out.highBand += mag;

    totalMag += mag;
  }

  pickTopPeaks(out.hz, out.mag, out.bins, out.peaks, out.harmonic2, out.harmonic3);

  if (out.dominantMag <= 0.0f) {
    out.character = "Quiet";
  } else {
    float tonalRatio = out.dominantMag / fmaxf(totalMag, 0.0001f);

    if (tonalRatio > 0.20f) out.character = "Tonal";
    else if (out.highBand > out.midBand && out.highBand > out.lowBand) out.character = "High-frequency";
    else if (out.lowBand > out.midBand && out.lowBand > out.highBand) out.character = "Low-frequency";
    else out.character = "Broadband";
  }

  if (axis == VIB_AXIS_MAG && out.character == "Tonal") {
    out.character = "Overall tonal";
  }
}
