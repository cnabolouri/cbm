#ifndef PM_NODE_TYPES_H
#define PM_NODE_TYPES_H

#include <Arduino.h>

enum Page {
  PAGE_VIB = 0,
  PAGE_TEMP = 1,
  PAGE_SOUND = 2,
  PAGE_SYS = 3
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
  MountState state = MOUNT_UNKNOWN;
  float confidence = 0.0f;
  float dynamicG = 0.0f;
  float driftG = 0.0f;
  unsigned long stableMs = 0;
  bool analysisTrusted = false;
};

struct VibrationData {
  float x_in_s = 0.0f;
  float y_in_s = 0.0f;
  float z_in_s = 0.0f;
  float total_in_s = 0.0f;
};

struct TemperatureData {
  float objF = 0.0f;
  float refF = 0.0f;
  float ambF = 0.0f;
  float deltaF = 0.0f;
};

struct SoundData {
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
};

struct SpectrumPeak {
  float hz = 0.0f;
  float mag = 0.0f;
};

struct SoundSpectrumData {
  static const int MAX_BINS = 64;

  int bins = 0;
  float hz[MAX_BINS] = {0};
  float mag[MAX_BINS] = {0};

  float dominantHz = 0.0f;
  float dominantMag = 0.0f;

  float lowBand = 0.0f;
  float midBand = 0.0f;
  float highBand = 0.0f;
  float centroidHz = 0.0f;

  SpectrumPeak peaks[3];
  bool harmonic2 = false;
  bool harmonic3 = false;

  String character = "Unknown";
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
};

struct LiveData {
  VibrationData vibration;
  TemperatureData temperature;
  SoundData sound;
  SystemState system;
  AnalysisData analysis;
  SoundSpectrumData soundSpectrum;
  VibrationSpectrumData vibrationSpectrum;
  MountData mount;
};

#endif // PM_NODE_TYPES_H
