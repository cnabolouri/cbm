#ifndef PM_NODE_TYPES_H
#define PM_NODE_TYPES_H

#include <Arduino.h>
#include "config.h"

enum PageId {
  PAGE_HOME = 0,
  PAGE_VIBRATION,
  PAGE_THERMAL,
  PAGE_SOUND,
  PAGE_SYSTEM,
  PAGE_COUNT
};

enum VibrationAxis {
  VIB_AXIS_X = 0,
  VIB_AXIS_Y = 1,
  VIB_AXIS_Z = 2,
  VIB_AXIS_MAG = 3
};

enum MountState {
  MOUNT_UNKNOWN = 0,
  MOUNT_MOVING = 1,
  MOUNT_STABILIZING = 2,
  MOUNT_MONITORING = 3
};

struct MountData {
  bool attached = false;
  MountState state = MOUNT_UNKNOWN;
  float confidence = 0.0f;
  float dynamicG = 0.0f;
  float driftG = 0.0f;
  unsigned long stableMs = 0;
  bool analysisTrusted = false;
};

struct VibrationData {
  float vx = 0.0f;
  float vy = 0.0f;
  float vz = 0.0f;
  float vt = 0.0f;

  float x_in_s = 0.0f;
  float y_in_s = 0.0f;
  float z_in_s = 0.0f;
  float total_in_s = 0.0f;

  float ax_g = 0.0f;
  float ay_g = 0.0f;
  float az_g = 0.0f;

  float maxTotal = 0.0f;
  float avgTotal = 0.0f;
};

struct TemperatureData {
  float refF = 0.0f;    // DS18B20 reference/contact temp
  float objF = 0.0f;    // derived from thermal hotspot
  float deltaF = 0.0f;  // objF - refF
  float ambF = 0.0f;
};

struct UIPointerState {
  float x = TFT_W * 0.5f;
  float y = TFT_H * 0.5f;
  int xi = TFT_W / 2;
  int yi = TFT_H / 2;
};

struct UIState {
  int currentPage = 0;
  bool recOn = false;
  UIPointerState pointer;
  int joyRawX = 0;
  int joyRawY = 0;
  bool joyButtonPressed = false;
  float joyNormX = 0.0f;
  float joyNormY = 0.0f;
};

struct ThermalRegionStats {
  bool valid = false;
  int pixelCount = 0;
  float percentOfFrame = 0.0f;
  float avgF = 0.0f;
  float maxF = 0.0f;
  int minX = 0;
  int minY = 0;
  int maxX = 0;
  int maxY = 0;
};

struct ThermalFrameData {
  static const int RAW_W = 32;
  static const int RAW_H = 24;
  static const int RAW_PIXELS = RAW_W * RAW_H;

  bool valid = false;

  float pixelsF[RAW_PIXELS] = {0};

  float minF = 0.0f;
  float maxF = 0.0f;

  float hotspotF = 0.0f;
  int hotspotX = 0;
  int hotspotY = 0;

  float centerF = 0.0f;

  float pointerF = 0.0f;
  float pointerDisplayF = 0.0f;
  int pointerX = RAW_W / 2;
  int pointerY = RAW_H / 2;

  float ambientF = 0.0f;
  ThermalRegionStats thresholdRegion;
};

enum ThermalRangeMode {
  THERMAL_RANGE_AUTO = 0,
  THERMAL_RANGE_FIXED = 1
};

enum ThermalPalette {
  THERMAL_PALETTE_IRON = 0,
  THERMAL_PALETTE_RAINBOW = 1,
  THERMAL_PALETTE_GRAYSCALE = 2
};

enum ThermalHotspotMode {
  THERMAL_HOTSPOT_AUTO = 0,
  THERMAL_HOTSPOT_LOCKED = 1
};

enum ThermalThresholdMode {
  THERMAL_THRESHOLD_OFF = 0,
  THERMAL_THRESHOLD_ABOVE = 1
};

struct ThermalDisplayState {
  ThermalRangeMode rangeMode = THERMAL_RANGE_AUTO;
  float fixedMinF = THERMAL_FIXED_MIN_F;
  float fixedMaxF = THERMAL_FIXED_MAX_F;
  float zoom = 1.0f;
  float centerX = (THERMAL_W - 1) * 0.5f;
  float centerY = (THERMAL_H - 1) * 0.5f;
  ThermalPalette palette = THERMAL_PALETTE_IRON;
  ThermalHotspotMode hotspotMode = THERMAL_HOTSPOT_AUTO;
  int lockedHotspotX = 0;
  int lockedHotspotY = 0;
  float lockedHotspotF = 0.0f;
  ThermalThresholdMode thresholdMode = THERMAL_THRESHOLD_OFF;
  float thresholdF = 100.0f;
};

struct SoundData {
  float dbRel = 0.0f;
  float peakHz = 0.0f;

  float rms = 0.0f;
  float db = 0.0f;
  float hz = 0.0f;
  int peak = 0;
};

enum RecordingMode {
  REC_IDLE = 0,
  REC_ARMED_WAITING_MOUNT = 1,
  REC_ACTIVE = 2
};

struct SystemState {
  bool sdOK = false;
  bool recording = false;
  String currentBaseName = "";
  RecordingMode recordingMode = REC_IDLE;
  bool manualOverride = false;
  String statusText = "";
};

struct SessionSummary {
  bool valid = false;
  int totalSamples = 0;
  int trustedSamples = 0;
  unsigned long firstTrustedMs = 0;
  unsigned long lastTrustedMs = 0;
  unsigned long validDurationMs = 0;
  float trustedRatio = 0.0f;

  float maxTotalVib = 0.0f;
  float avgTotalVib = 0.0f;
  String dominantAxis = "X";
  String vibrationAlert = "Normal";

  float maxDeltaTemp = 0.0f;
  float avgDeltaTemp = 0.0f;
  String temperatureAlert = "Normal";

  float maxDb = 0.0f;
  float avgDb = 0.0f;
  float maxHz = 0.0f;
  String soundAlert = "Normal";

  String overallAlert = "Normal";
};

struct SessionInfo {
  String baseName;
  bool hasCsv = false;
  bool hasWav = false;
  size_t csvSize = 0;
  size_t wavSize = 0;
  bool hasMeta = false;
  String tag = "";
  String status = "";
  String note = "";
  SessionSummary summary;
};

struct VibrationAnalysis {
  float currentTotal = 0.0f;
  float maxTotal = 0.0f;
  float avgTotal = 0.0f;
  String dominantAxis = "X";
  String alert = "Normal";
};

struct TemperatureAnalysis {
  float currentDelta = 0.0f;
  float maxDelta = 0.0f;
  float avgDelta = 0.0f;
  bool risingFast = false;
  String alert = "Normal";
};

struct SoundAnalysis {
  float currentDb = 0.0f;
  float maxDb = 0.0f;
  float avgDb = 0.0f;
  float dominantHz = 0.0f;
  String alert = "Normal";
};

struct AnalysisData {
  VibrationAnalysis vibration;
  TemperatureAnalysis temperature;
  SoundAnalysis sound;
  bool valid = false;
};

struct SpectrumPeak {
  float hz = 0.0f;
  float mag = 0.0f;
};

struct SoundSpectrumData {
  static const int FFT_SIZE = 256;
  static const int BINS = 64;
  static const int MAX_BINS = 64;

  int bins = 0;
  float hz[MAX_BINS] = {0};
  float mag[MAX_BINS] = {0};

  float sampleRateHz = 0.0f;
  float dominantHz = 0.0f;
  float dominantMag = 0.0f;
  float dominantAmp = 0.0f;

  float lowBand = 0.0f;
  float midBand = 0.0f;
  float highBand = 0.0f;
  float centroidHz = 0.0f;

  SpectrumPeak peaks[3];
  bool harmonic2 = false;
  bool harmonic3 = false;

  String character = "Unknown";
  bool valid = false;
};

struct VibrationSpectrumData {
  static const int MAX_BINS = 64;

  int bins = 0;
  float hz[MAX_BINS] = {0};
  float mag[MAX_BINS] = {0};

  float dominantHz = 0.0f;
  float dominantMag = 0.0f;

  float lowBand = 0.0f;
  float midBand = 0.0f;
  float highBand = 0.0f;

  SpectrumPeak peaks[3];
  bool harmonic2 = false;
  bool harmonic3 = false;

  String sourceAxis = "X";
  String character = "Unknown";
  bool valid = false;
};

struct LiveData {
  VibrationData vibration;
  TemperatureData temperature;
  SoundData sound;
  SystemState system;
  UIState ui;
  AnalysisData analysis;
  SoundSpectrumData soundSpectrum;
  VibrationSpectrumData vibrationSpectrum;
  MountData mount;
  ThermalFrameData thermal;
  ThermalDisplayState thermalDisplay;
};

#endif // PM_NODE_TYPES_H
