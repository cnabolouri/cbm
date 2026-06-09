#include <SPI.h>
#include "config.h"
#include "types.h"

#include "input/joystick_input.h"
#include "sensors/vibration_sensor.h"
#include "sensors/temperature_sensor.h"
#include "sensors/thermal_camera_sensor.h"
#include "sensors/microphone.h"

#include "storage/sd_manager.h"
#include "storage/recorder.h"

#include "web/web_ui.h"

#include "analysis/diagnostics.h"
#include "analysis/fft_analyzer.h"
#include "analysis/vibration_sampler.h"
#include "analysis/vibration_fft.h"
#include "analysis/mount_detector.h"

#include "ui/tft_ui.h"

// =====================================================
// GLOBAL HARDWARE OBJECTS
// =====================================================

// =====================================================
// GLOBAL APP OBJECTS
// =====================================================
SDManager sdManager;
Recorder recorder(sdManager);
LiveData live;
WebUI webUI(sdManager, recorder, live);
DiagnosticsEngine diagnostics;
FFTAnalyzer fftAnalyzer;
VibrationSampler vibrationSampler;
VibrationFFT vibrationFFT;
VibrationAxis selectedVibrationFFTaxis = VIB_AXIS_X;
MountDetector mountDetector;

JoystickInput joystickInput;
VibrationSensor vibrationSensor;
TemperatureSensor temperatureSensor;
ThermalCameraSensor thermalCameraSensor;
MicrophoneSensor microphoneSensor;

TFTUI tftUI;

// =====================================================
// TIMING
// =====================================================
unsigned long lastMicros = 0;
unsigned long recordingStartMs = 0;
unsigned long lastCsvLogMs = 0;
unsigned long lastFlushMs = 0;
unsigned long lastFftMs = 0;
unsigned long lastVibrationSampleMicros = 0;
unsigned long lastVibrationFftMs = 0;
RecordingMode recordingMode = REC_IDLE;
bool manualOverride = false;
String lastActionMessage = "";
ThermalRangeMode thermalRangeMode = THERMAL_RANGE_AUTO;
float thermalFixedMinF = THERMAL_FIXED_MIN_F;
float thermalFixedMaxF = THERMAL_FIXED_MAX_F;
float thermalZoom = 1.0f;
float thermalCenterX = (THERMAL_W - 1) * 0.5f;
float thermalCenterY = (THERMAL_H - 1) * 0.5f;
ThermalPalette thermalPalette = THERMAL_PALETTE_IRON;
unsigned long lastTelemetryMs = 0;
static const unsigned long TELEMETRY_INTERVAL_MS = 250;
ThermalHotspotMode thermalHotspotMode = THERMAL_HOTSPOT_AUTO;
int thermalLockedHotspotX = 0;
int thermalLockedHotspotY = 0;
float thermalLockedHotspotF = 0.0f;
ThermalThresholdMode thermalThresholdMode = THERMAL_THRESHOLD_OFF;
float thermalThresholdF = 100.0f;

// =====================================================
// HELPERS
// =====================================================
void syncThermalDisplayState() {
  live.thermalDisplay.rangeMode = thermalRangeMode;
  live.thermalDisplay.fixedMinF = thermalFixedMinF;
  live.thermalDisplay.fixedMaxF = thermalFixedMaxF;
  live.thermalDisplay.zoom = thermalZoom;
  live.thermalDisplay.centerX = thermalCenterX;
  live.thermalDisplay.centerY = thermalCenterY;
  live.thermalDisplay.palette = thermalPalette;
  live.thermalDisplay.hotspotMode = thermalHotspotMode;
  live.thermalDisplay.lockedHotspotX = thermalLockedHotspotX;
  live.thermalDisplay.lockedHotspotY = thermalLockedHotspotY;
  live.thermalDisplay.lockedHotspotF = thermalLockedHotspotF;
  live.thermalDisplay.thresholdMode = thermalThresholdMode;
  live.thermalDisplay.thresholdF = thermalThresholdF;
}

void printTelemetry() {
  Serial.print("VIB vx="); Serial.print(live.vibration.vx, 2);
  Serial.print(" vy="); Serial.print(live.vibration.vy, 2);
  Serial.print(" vz="); Serial.print(live.vibration.vz, 2);
  Serial.print(" vt="); Serial.print(live.vibration.vt, 2);
  Serial.print(" max="); Serial.print(live.vibration.maxTotal, 2);
  Serial.print(" avg="); Serial.print(live.vibration.avgTotal, 2);
  Serial.print(" ax="); Serial.print(live.vibration.ax_g, 3);
  Serial.print(" ay="); Serial.print(live.vibration.ay_g, 3);
  Serial.print(" az="); Serial.print(live.vibration.az_g, 3);
  Serial.print("  sndDb="); Serial.print(live.sound.dbRel, 1);
  Serial.print("  sndPeak="); Serial.print(live.sound.peakHz, 1);
  Serial.print("  sndFFT="); Serial.print(live.soundSpectrum.valid ? "T" : "F");
  Serial.print(" mount="); Serial.print(live.mount.analysisTrusted ? "T" : "F");
  Serial.print(" state="); Serial.print((int)live.mount.state);
  Serial.print(" conf="); Serial.print(live.mount.confidence, 1);
  Serial.print(" joyX="); Serial.print(live.ui.joyRawX);
  Serial.print(" joyY="); Serial.print(live.ui.joyRawY);
  Serial.print(" joyNx="); Serial.print(live.ui.joyNormX, 2);
  Serial.print(" joyNy="); Serial.print(live.ui.joyNormY, 2);
  Serial.print(" ptr="); Serial.print(live.ui.pointer.xi);
  Serial.print(",");
  Serial.print(live.ui.pointer.yi);
  Serial.print(" page="); Serial.print(live.ui.currentPage);
  Serial.print(" status="); Serial.println(live.system.statusText);
}

void computeThermalThresholdRegion(ThermalFrameData& thermal, const ThermalDisplayState& display) {
  thermal.thresholdRegion = ThermalRegionStats{};
  if (!thermal.valid) return;
  if (display.thresholdMode != THERMAL_THRESHOLD_ABOVE) return;

  const float thresholdF = display.thresholdF;
  int count = 0;
  float sumF = 0.0f;
  float maxRegionF = -100000.0f;
  int minX = THERMAL_W - 1;
  int minY = THERMAL_H - 1;
  int maxX = 0;
  int maxY = 0;

  for (int y = 0; y < THERMAL_H; y++) {
    for (int x = 0; x < THERMAL_W; x++) {
      int idx = y * THERMAL_W + x;
      float v = thermal.pixelsF[idx];
      if (v >= thresholdF) {
        count++;
        sumF += v;
        if (v > maxRegionF) maxRegionF = v;
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
      }
    }
  }

  if (count <= 0) return;

  thermal.thresholdRegion.valid = true;
  thermal.thresholdRegion.pixelCount = count;
  thermal.thresholdRegion.percentOfFrame = 100.0f * ((float)count / (float)THERMAL_PIXELS);
  thermal.thresholdRegion.avgF = sumF / (float)count;
  thermal.thresholdRegion.maxF = maxRegionF;
  thermal.thresholdRegion.minX = minX;
  thermal.thresholdRegion.minY = minY;
  thermal.thresholdRegion.maxX = maxX;
  thermal.thresholdRegion.maxY = maxY;
}

void handleJoystickActions() {
  joystickInput.update(live.ui);

  if (joystickInput.consumeShortPress()) {
    live.ui.currentPage = (live.ui.currentPage + 1) % PAGE_COUNT;
  }

  if (joystickInput.consumeLongPress()) {
    if (recordingMode == REC_IDLE) {
      requestRecordingStart();
    } else {
      requestRecordingStop();
    }
  }
}

void updateThermalPointerFromJoystick() {
  if (live.ui.currentPage != PAGE_THERMAL) return;

  const int imgX = 10;
  const int imgY = 40;
  const int imgW = 320;
  const int imgH = 216;

  int px = live.ui.pointer.xi;
  int py = live.ui.pointer.yi;

  if (px < imgX) px = imgX;
  if (px >= imgX + imgW) px = imgX + imgW - 1;
  if (py < imgY) py = imgY;
  if (py >= imgY + imgH) py = imgY + imgH - 1;

  int thermalX = map(px, imgX, imgX + imgW - 1, 0, THERMAL_W - 1);
  int thermalY = map(py, imgY, imgY + imgH - 1, 0, THERMAL_H - 1);
  thermalCameraSensor.setPointer(thermalX, thermalY);
}

bool isAnalysisTrustedNow() {
  return live.mount.analysisTrusted || manualOverride;
}

bool beginActualRecording() {
  if (recorder.isRecording()) return true;

  if (!recorder.start()) {
    recordingMode = REC_IDLE;
    lastActionMessage = "Recording start failed";
    live.system.statusText = lastActionMessage;
    Serial.println(lastActionMessage);
    return false;
  }

  recordingStartMs = millis();
  lastCsvLogMs = 0;
  lastFlushMs = 0;
  recordingMode = REC_ACTIVE;

  live.system.recording = true;
  live.system.currentBaseName = recorder.currentBaseName();
  live.system.recordingMode = recordingMode;
  live.system.statusText = "Recording active: " + recorder.currentBaseName();

  lastActionMessage = live.system.statusText;
  Serial.println(lastActionMessage);
  return true;
}

void armRecording() {
  if (recordingMode == REC_ACTIVE || recorder.isRecording()) return;

  recordingMode = REC_ARMED_WAITING_MOUNT;
  live.system.recording = false;
  live.system.recordingMode = recordingMode;
  live.system.statusText = "Waiting for stable mount...";

  lastActionMessage = live.system.statusText;
  Serial.println(lastActionMessage);
}

void stopRecordingWorkflow() {
  if (recorder.isRecording()) {
    recorder.stop();
  }

  recordingMode = REC_IDLE;
  live.system.recording = false;
  live.system.recordingMode = recordingMode;
  live.system.currentBaseName = recorder.currentBaseName();
  live.system.statusText = "Recording stopped";

  lastActionMessage = live.system.statusText;
  Serial.println(lastActionMessage);
}

void requestRecordingStart() {
  if (recordingMode == REC_ACTIVE || recorder.isRecording()) {
    lastActionMessage = "Already recording";
    live.system.statusText = lastActionMessage;
    return;
  }

  if (isAnalysisTrustedNow()) {
    beginActualRecording();
  } else {
    armRecording();
  }
}

void requestRecordingStop() {
  if (recordingMode == REC_IDLE && !recorder.isRecording()) {
    lastActionMessage = "Recorder idle";
    live.system.statusText = lastActionMessage;
    return;
  }

  stopRecordingWorkflow();
}

void processArmedRecording() {
  if (recordingMode == REC_ARMED_WAITING_MOUNT && isAnalysisTrustedNow()) {
    beginActualRecording();
  }
}

void updateSystemState() {
  live.system.sdOK = sdManager.isReady();
  live.system.recording = recorder.isRecording();
  live.ui.recOn = recorder.isRecording();
  live.system.currentBaseName = recorder.currentBaseName();
  live.system.recordingMode = recordingMode;
  live.system.manualOverride = manualOverride;
  syncThermalDisplayState();

  if (recordingMode == REC_ARMED_WAITING_MOUNT) {
    live.system.statusText = "Waiting for stable mount...";
  } else if (recordingMode == REC_ACTIVE) {
    if (isAnalysisTrustedNow()) {
      live.system.statusText = "Recording active: " + recorder.currentBaseName();
    } else {
      live.system.statusText = "Recording active - mount unstable";
    }
  }
}

void updateSensors(float dt) {
  vibrationSensor.update(live.vibration, live.vibrationSpectrum);

  unsigned long nowMicros = micros();
  if (lastVibrationSampleMicros == 0 || (nowMicros - lastVibrationSampleMicros) >= 5000) {
    vibrationSampler.update(
      vibrationSensor.getLastRawXg(),
      vibrationSensor.getLastRawYg(),
      vibrationSensor.getLastRawZg()
    );
    lastVibrationSampleMicros = nowMicros;
  }

  mountDetector.update(
    vibrationSensor.getLastRawXg(),
    vibrationSensor.getLastRawYg(),
    vibrationSensor.getLastRawZg(),
    live.mount
  );

  microphoneSensor.update(live.sound);
  temperatureSensor.update(live.temperature);
  thermalCameraSensor.update(live.thermal);
  syncThermalDisplayState();

  if (live.thermal.valid) {
    if (thermalHotspotMode == THERMAL_HOTSPOT_LOCKED) {
      int lx = thermalLockedHotspotX;
      int ly = thermalLockedHotspotY;
      if (lx < 0) lx = 0;
      if (lx >= THERMAL_W) lx = THERMAL_W - 1;
      if (ly < 0) ly = 0;
      if (ly >= THERMAL_H) ly = THERMAL_H - 1;
      thermalLockedHotspotF = live.thermal.pixelsF[ly * THERMAL_W + lx];
    } else {
      thermalLockedHotspotX = live.thermal.hotspotX;
      thermalLockedHotspotY = live.thermal.hotspotY;
      thermalLockedHotspotF = live.thermal.hotspotF;
    }
    syncThermalDisplayState();
    computeThermalThresholdRegion(live.thermal, live.thermalDisplay);

    live.temperature.objF = live.thermal.hotspotF;
    live.temperature.ambF = live.thermal.centerF;
    live.temperature.deltaF = (live.temperature.refF > 0.0f)
      ? live.thermal.hotspotF - live.temperature.refF
      : 0.0f;
  } else {
    live.thermal.thresholdRegion = ThermalRegionStats{};
  }

  unsigned long nowMs = millis();
  if (lastFftMs == 0 || (nowMs - lastFftMs) >= 250) {
    fftAnalyzer.computeSoundSpectrum(
      microphoneSensor.getCenteredSamples(),
      microphoneSensor.getSampleCount(),
      AUDIO_SAMPLE_RATE,
      live.soundSpectrum
    );
    live.sound.peakHz = live.soundSpectrum.valid ? live.soundSpectrum.dominantHz : 0.0f;
    lastFftMs = nowMs;
  }

  if (lastVibrationFftMs == 0 || (nowMs - lastVibrationFftMs) >= 250) {
    vibrationFFT.compute(vibrationSampler, selectedVibrationFFTaxis, live.vibrationSpectrum);
    lastVibrationFftMs = nowMs;
  }
}

void updateRecording() {
  if (!recorder.isRecording()) return;

  recorder.appendAudioSamples(
    microphoneSensor.getCenteredSamples(),
    microphoneSensor.getSampleCount()
  );

  unsigned long nowMs = millis();
  if (lastCsvLogMs == 0 || (nowMs - lastCsvLogMs) >= CSV_LOG_INTERVAL_MS) {
    recorder.appendCsvRow(live, nowMs - recordingStartMs);
    lastCsvLogMs = nowMs;
  }

  if (lastFlushMs == 0 || (nowMs - lastFlushMs) >= 1000) {
    recorder.flush();
    lastFlushMs = nowMs;
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1500);

  live.ui.pointer.x = TFT_W * 0.5f;
  live.ui.pointer.y = TFT_H * 0.5f;
  live.ui.pointer.xi = TFT_W / 2;
  live.ui.pointer.yi = TFT_H / 2;

  // Modular inputs / UI
  joystickInput.begin();
  tftUI.begin();

  // Sensors
  bool vibOK = vibrationSensor.begin();
  bool tempOK = temperatureSensor.begin();
  bool thermalOK = thermalCameraSensor.begin();
  bool micOK  = microphoneSensor.begin();
  Serial.println(thermalOK ? "MLX90640 ready" : "MLX90640 not found");
  Serial.println(vibOK ? "IIS3DWB ready" : "IIS3DWB not found");
  vibrationSampler.begin(200.0f);
  mountDetector.begin();

  // Storage
  bool sdOK = sdManager.begin();

  // Live system state bootstrap
  live.system.sdOK = sdOK;
  live.system.recording = false;
  live.system.currentBaseName = "";
  live.system.recordingMode = recordingMode;
  live.system.manualOverride = manualOverride;
  live.system.statusText = "Recorder idle";

  // Web UI
  webUI.setRecordingCallbacks(
    []() -> bool {
      requestRecordingStart();
      return true;
    },
    []() { requestRecordingStop(); }
  );
  webUI.setStatusMessageSource(&lastActionMessage);
  webUI.setVibrationFFTaxisRef(&selectedVibrationFFTaxis);
  webUI.setManualOverrideRef(&manualOverride);
  webUI.setThermalRangeRefs(&thermalRangeMode, &thermalFixedMinF, &thermalFixedMaxF);
  webUI.setThermalZoomRef(&thermalZoom);
  webUI.setThermalCenterRefs(&thermalCenterX, &thermalCenterY);
  webUI.setThermalPaletteRef(&thermalPalette);
  webUI.setThermalHotspotRefs(&thermalHotspotMode, &thermalLockedHotspotX, &thermalLockedHotspotY);
  webUI.setThermalThresholdRefs(&thermalThresholdMode, &thermalThresholdF);
  webUI.setThermalPointerCallback([](int x, int y) {
    thermalCameraSensor.setPointer(x, y);
  });
  webUI.begin();

  // Timing
  lastMicros = micros();

  // Boot display
  live.system.statusText = "Booting...";
  live.system.recording = false;
  live.system.currentBaseName = "";
  live.system.recordingMode = recordingMode;
  live.system.sdOK = sdOK;
  live.ui.currentPage = PAGE_HOME;
  live.ui.recOn = false;
  tftUI.draw(live);

  delay(1000);

  updateSystemState();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // Web
  webUI.handleClient();

  // Inputs
  handleJoystickActions();
  updateThermalPointerFromJoystick();
  tftUI.draw(live);

  // Sensor timing
  unsigned long now = micros();
  float dt = (now - lastMicros) / 1000000.0f;
  lastMicros = now;

  if (dt <= 0.0f || dt > 0.2f) {
    dt = 0.02f;
  }

  // Sensors
  updateSensors(dt);

  // Armed recording starts automatically once mount trust is available.
  processArmedRecording();

  // State
  updateSystemState();

  // Telemetry
  unsigned long nowMs = millis();
  if (lastTelemetryMs == 0 || (nowMs - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
    printTelemetry();
    lastTelemetryMs = nowMs;
  }

  // Diagnostics
  if (isAnalysisTrustedNow()) {
    diagnostics.update(live);
  } else {
    live.analysis = AnalysisData{};
  }

  // Recording
  updateRecording();

  delay(1);
}
