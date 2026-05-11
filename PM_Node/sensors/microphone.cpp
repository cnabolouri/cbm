#include "microphone.h"
#include "../config.h"
#include <math.h>

MicrophoneSensor::MicrophoneSensor() {}

bool MicrophoneSensor::begin() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num = PIN_I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

void MicrophoneSensor::update(SoundData& out) {
  size_t bytesRead = 0;
  i2s_read(I2S_NUM_0, rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY);

  sampleCount = bytesRead / sizeof(int32_t);
  if (sampleCount <= 0) {
    out = SoundData{};
    return;
  }

  double mean = 0.0;
  for (int i = 0; i < sampleCount; i++) {
    centeredSamples[i] = rawSamples[i] >> 14;
    mean += centeredSamples[i];
  }
  mean /= sampleCount;

  double sumSq = 0.0;
  int peak = 0;
  for (int i = 0; i < sampleCount; i++) {
    centeredSamples[i] -= (int32_t)mean;
    int a = abs((int)centeredSamples[i]);
    if (a > peak) peak = a;
    sumSq += (double)centeredSamples[i] * (double)centeredSamples[i];
  }

  out.rms = sqrt(sumSq / sampleCount);
  out.peak = peak;
  out.hz = computeDominantFreqZeroCross(centeredSamples, sampleCount, AUDIO_SAMPLE_RATE);
  out.db = 20.0f * log10f((out.rms + 1.0f) / 10.0f);
}

float MicrophoneSensor::computeDominantFreqZeroCross(const int32_t *buf, int count, float sampleRate) {
  int crossings = 0;
  for (int i = 1; i < count; i++) {
    if ((buf[i - 1] < 0 && buf[i] >= 0) || (buf[i - 1] >= 0 && buf[i] < 0)) crossings++;
  }
  float seconds = (float)count / sampleRate;
  if (seconds <= 0) return 0;
  return (crossings / 2.0f) / seconds;
}

const int32_t* MicrophoneSensor::getCenteredSamples() const {
  return centeredSamples;
}

int MicrophoneSensor::getSampleCount() const {
  return sampleCount;
}
