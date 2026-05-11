#pragma once
#include <Arduino.h>
#include <FS.h>
#include "../types.h"
#include "sd_manager.h"

class Recorder {
public:
  explicit Recorder(SDManager& sdRef);

  bool start();
  void stop();
  bool isRecording() const;

  void appendAudioSamples(const int32_t* samples, int count);
  void appendCsvRow(const LiveData& data, unsigned long sessionMs);
  void flush();

  String currentBaseName() const;

private:
  void writeWavHeader(File& f, uint32_t dataSize, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels);

  SDManager& sd;
  bool recording = false;
  File wavFile;
  File csvFile;
  String baseName = "";
  uint32_t wavDataBytes = 0;
};
