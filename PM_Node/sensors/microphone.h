#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include "../config.h"
#include "../types.h"

class MicrophoneSensor {
public:
  MicrophoneSensor();
  bool begin();
  void update(SoundData& out);

  const int32_t* getCenteredSamples() const;
  int getSampleCount() const;

private:
  float computeDominantFreqZeroCross(const int32_t *buf, int count, float sampleRate);

  int32_t rawSamples[AUDIO_BUFFER_SAMPLES];
  int32_t centeredSamples[AUDIO_BUFFER_SAMPLES];
  int sampleCount = 0;
};
