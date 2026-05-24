#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

#include "config.h"
#include "types.h"

#include "sensors/adxl357.h"
#include "sensors/temperature.h"
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

#include "input/button.h"
#include "input/touch.h"

#include "ui/tft_ui.h"

// =====================================================
// GLOBAL HARDWARE OBJECTS
// =====================================================
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

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

ADXL357Sensor vibrationSensor;
TemperatureSensor temperatureSensor;
ThermalCameraSensor thermalCameraSensor;
MicrophoneSensor microphoneSensor;

ButtonInput button(
  BUTTON_PIN,
  BUTTON_DEBOUNCE_MS,
  SHORT_PRESS_MS,
  LONG_PRESS_MS
);

TouchInput touch(
  ts,
  TFT_W,
  TFT_H,
  TS_MINX,
  TS_MAXX,
  TS_MINY,
  TS_MAXY,
  TOUCH_SWAP_XY,
  TOUCH_INVERT_X,
  TOUCH_INVERT_Y
);

TFTUI tftUI(tft);

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

// =====================================================
// HELPERS
// =====================================================
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

void handleButtonEvent(ButtonInput::Event evt) {
  switch (evt) {
    case ButtonInput::SHORT_PRESS:
      if (recordingMode == REC_IDLE) requestRecordingStart();
      else requestRecordingStop();
      break;

    case ButtonInput::LONG_PRESS:
      tftUI.nextPage();
      break;

    case ButtonInput::NONE:
    default:
      break;
  }
}

void handleTouchEvent(TouchInput::EventType evt) {
  switch (evt) {
    case TouchInput::TOUCH_TAB_VIB:
      tftUI.setPage(PAGE_VIB);
      break;

    case TouchInput::TOUCH_TAB_TEMP:
      tftUI.setPage(PAGE_TEMP);
      break;

    case TouchInput::TOUCH_TAB_THERMAL:
      tftUI.setPage(PAGE_THERMAL);
      break;

    case TouchInput::TOUCH_TAB_SOUND:
      tftUI.setPage(PAGE_SOUND);
      break;

    case TouchInput::TOUCH_TAB_SYS:
      tftUI.setPage(PAGE_SYS);
      break;

    case TouchInput::TOUCH_PREV_PAGE:
      tftUI.prevPage();
      break;

    case TouchInput::TOUCH_NEXT_PAGE:
      tftUI.nextPage();
      break;

    case TouchInput::TOUCH_NONE:
    default:
      break;
  }
}

void updateSystemState() {
  live.system.sdOK = sdManager.isReady();
  live.system.recording = recorder.isRecording();
  live.system.currentBaseName = recorder.currentBaseName();
  live.system.recordingMode = recordingMode;
  live.system.manualOverride = manualOverride;
  live.thermalDisplay.rangeMode = thermalRangeMode;
  live.thermalDisplay.fixedMinF = thermalFixedMinF;
  live.thermalDisplay.fixedMaxF = thermalFixedMaxF;
  live.thermalDisplay.zoom = thermalZoom;
  live.thermalDisplay.centerX = thermalCenterX;
  live.thermalDisplay.centerY = thermalCenterY;

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
  vibrationSensor.update(dt, live.vibration);

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

  if (live.thermal.valid) {
    live.temperature.objF = live.thermal.hotspotF;
    live.temperature.ambF = live.thermal.centerF;
    live.temperature.deltaF = (live.temperature.refF > 0.0f)
      ? live.thermal.hotspotF - live.temperature.refF
      : 0.0f;
  }

  unsigned long nowMs = millis();
  if (isAnalysisTrustedNow()) {
    if (lastFftMs == 0 || (nowMs - lastFftMs) >= 250) {
      fftAnalyzer.computeSoundSpectrum(
        microphoneSensor.getCenteredSamples(),
        microphoneSensor.getSampleCount(),
        AUDIO_SAMPLE_RATE,
        live.soundSpectrum
      );
      lastFftMs = nowMs;
    }

    if (lastVibrationFftMs == 0 || (nowMs - lastVibrationFftMs) >= 250) {
      vibrationFFT.compute(vibrationSampler, selectedVibrationFFTaxis, live.vibrationSpectrum);
      lastVibrationFftMs = nowMs;
    }
  } else {
    live.soundSpectrum = SoundSpectrumData{};
    live.vibrationSpectrum = VibrationSpectrumData{};
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

void initTouchBusSafety() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1500);

  // Shared SPI bus for TFT + touch
  initTouchBusSafety();
  SPI.begin(TFT_SCLK, TOUCH_MISO, TFT_MOSI, TFT_CS);

  // Raw devices
  ts.begin();
  ts.setRotation(1);

  // Modular inputs / UI
  button.begin();
  touch.begin();
  tftUI.begin();

  // Sensors
  bool adxlOK = vibrationSensor.begin();
  bool tempOK = temperatureSensor.begin();
  bool thermalOK = thermalCameraSensor.begin();
  bool micOK  = microphoneSensor.begin();
  Serial.println(thermalOK ? "MLX90640 ready" : "MLX90640 not found");
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
  webUI.setThermalPointerCallback([](int x, int y) {
    thermalCameraSensor.setPointer(x, y);
  });
  webUI.begin();

  // Timing
  lastMicros = micros();

  // Boot screen
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(16, 18);
  tft.print("PM Node Boot");

  tft.setTextSize(2);
  tft.setCursor(16, 56);
  tft.print("ADXL: ");
  tft.print(adxlOK ? "OK" : "FAIL");

  tft.setCursor(16, 82);
  tft.print("TEMP: ");
  tft.print(tempOK ? "OK" : "FAIL");

  tft.setCursor(16, 108);
  tft.print("MLX : ");
  tft.print(thermalOK ? "OK" : "FAIL");

  tft.setCursor(16, 134);
  tft.print("MIC : ");
  tft.print(micOK ? "OK" : "FAIL");

  tft.setCursor(16, 160);
  tft.print("SD  : ");
  tft.print(sdOK ? "OK" : "FAIL");

  tft.setTextSize(1);
  tft.setCursor(16, 190);
  tft.print("Short press = record");
  tft.setCursor(16, 202);
  tft.print("Long press = next page");

  delay(1000);

  // Start on vibration page
  tftUI.setPage(PAGE_VIB);
  updateSystemState();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // Web
  webUI.handleClient();

  // Inputs
  TouchInput::EventType touchEvent = touch.update();
  tftUI.setTouchPoint(touch.lastX(), touch.lastY(), touch.isPressed());

  bool consumedThermalTouch = false;
  if (tftUI.consumeThermalRangeToggleRequest()) {
    thermalRangeMode = (thermalRangeMode == THERMAL_RANGE_AUTO)
      ? THERMAL_RANGE_FIXED
      : THERMAL_RANGE_AUTO;
    consumedThermalTouch = true;
  }
  if (tftUI.consumeThermalZoomCycleRequest()) {
    if (thermalZoom < 1.5f) thermalZoom = 2.0f;
    else if (thermalZoom < 2.5f) thermalZoom = 3.0f;
    else thermalZoom = 1.0f;

    if (thermalZoom == 1.0f) {
      thermalCenterX = (THERMAL_W - 1) * 0.5f;
      thermalCenterY = (THERMAL_H - 1) * 0.5f;
    }
    consumedThermalTouch = true;
  }

  handleButtonEvent(button.update());
  if (!consumedThermalTouch) {
    handleTouchEvent(touchEvent);
  }

  int thermalX = 0;
  int thermalY = 0;
  if (tftUI.getThermalPointerRequest(thermalX, thermalY)) {
    thermalCameraSensor.setPointer(thermalX, thermalY);
  }

  float thermalPanX = 0.0f;
  float thermalPanY = 0.0f;
  if (tftUI.getThermalPanRequest(thermalPanX, thermalPanY)) {
    thermalCenterX = thermalPanX;
    thermalCenterY = thermalPanY;
  }

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

  // Diagnostics
  if (isAnalysisTrustedNow()) {
    diagnostics.update(live);
  } else {
    live.analysis = AnalysisData{};
  }

  // Recording
  updateRecording();

  // TFT UI
  tftUI.render(live, touch.lastX(), touch.lastY());

  delay(1);
}
