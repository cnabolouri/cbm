#include "recorder.h"
#include "../config.h"

Recorder::Recorder(SDManager& sdRef) : sd(sdRef) {}

bool Recorder::start() {
  if (!sd.isReady()) {
    Serial.println("Cannot start recording: SD not ready");
    return false;
  }
  if (recording) return true;

  baseName = sd.nextRecordingBaseName();
  String wavName = baseName + ".wav";
  String csvName = baseName + ".csv";

  wavFile = sd.openWrite(wavName);
  if (!wavFile) {
    Serial.println("WAV open fail");
    return false;
  }

  csvFile = sd.openWrite(csvName);
  if (!csvFile) {
    Serial.println("CSV open fail");
    wavFile.close();
    return false;
  }

  for (int i = 0; i < 44; i++) wavFile.write((uint8_t)0);

  csvFile.println("ms,trusted,mount_state,mount_confidence,vx_in_s,vy_in_s,vz_in_s,vtot_in_s,objF,refF,ambF,dTF,soundRms,dBrel,freqHz");

  wavDataBytes = 0;
  recording = true;

  Serial.print("Recording START: ");
  Serial.println(baseName);
  return true;
}

void Recorder::stop() {
  if (!recording) return;

  writeWavHeader(wavFile, wavDataBytes, AUDIO_SAMPLE_RATE, 16, 1);
  wavFile.close();
  csvFile.close();
  recording = false;

  Serial.println("Recording STOP");
}

bool Recorder::isRecording() const {
  return recording;
}

void Recorder::appendAudioSamples(const int32_t* samples, int count) {
  if (!recording || !wavFile || count <= 0) return;

  for (int i = 0; i < count; i++) {
    int32_t s = samples[i];
    if (s > 32767) s = 32767;
    if (s < -32768) s = -32768;
    int16_t out = (int16_t)s;
    wavFile.write((uint8_t*)&out, 2);
    wavDataBytes += 2;
  }
}

void Recorder::appendCsvRow(const LiveData& data, unsigned long sessionMs) {
  if (!recording || !csvFile) return;

  csvFile.print(sessionMs); csvFile.print(",");
  csvFile.print(data.mount.analysisTrusted ? 1 : 0); csvFile.print(",");
  csvFile.print((int)data.mount.state); csvFile.print(",");
  csvFile.print(data.mount.confidence, 1); csvFile.print(",");

  csvFile.print(data.vibration.x_in_s, 5); csvFile.print(",");
  csvFile.print(data.vibration.y_in_s, 5); csvFile.print(",");
  csvFile.print(data.vibration.z_in_s, 5); csvFile.print(",");
  csvFile.print(data.vibration.total_in_s, 5); csvFile.print(",");
  csvFile.print(data.temperature.objF, 2); csvFile.print(",");
  csvFile.print(data.temperature.refF, 2); csvFile.print(",");
  csvFile.print(data.temperature.ambF, 2); csvFile.print(",");
  csvFile.print(data.temperature.deltaF, 2); csvFile.print(",");
  csvFile.print(data.sound.rms, 2); csvFile.print(",");
  csvFile.print(data.sound.db, 2); csvFile.print(",");
  csvFile.println(data.sound.hz, 2);
}

void Recorder::flush() {
  if (wavFile) wavFile.flush();
  if (csvFile) csvFile.flush();
}

String Recorder::currentBaseName() const {
  return baseName;
}

void Recorder::writeWavHeader(File& f, uint32_t dataSize, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
  uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  uint16_t blockAlign = channels * (bitsPerSample / 8);
  uint32_t chunkSize = 36 + dataSize;

  f.seek(0);
  f.write((const uint8_t*)"RIFF", 4);
  f.write((uint8_t*)&chunkSize, 4);
  f.write((const uint8_t*)"WAVE", 4);

  f.write((const uint8_t*)"fmt ", 4);
  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;
  f.write((uint8_t*)&subchunk1Size, 4);
  f.write((uint8_t*)&audioFormat, 2);
  f.write((uint8_t*)&channels, 2);
  f.write((uint8_t*)&sampleRate, 4);
  f.write((uint8_t*)&byteRate, 4);
  f.write((uint8_t*)&blockAlign, 2);
  f.write((uint8_t*)&bitsPerSample, 2);

  f.write((const uint8_t*)"data", 4);
  f.write((uint8_t*)&dataSize, 4);
}
