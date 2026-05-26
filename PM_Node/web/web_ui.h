#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <functional>
#include "../types.h"
#include "../storage/sd_manager.h"
#include "../storage/recorder.h"

class WebUI {
public:
  WebUI(SDManager& sdRef, Recorder& recorderRef, LiveData& liveRef);

  void begin();
  void handleClient();
  void setRecordingCallbacks(std::function<bool()> startCb, std::function<void()> stopCb);
  void setStatusMessageSource(String* statusMsg);
  void setVibrationFFTaxisRef(VibrationAxis* axisPtr);
  void setManualOverrideRef(bool* overridePtrIn);
  void setThermalRangeRefs(ThermalRangeMode* modePtr, float* minPtr, float* maxPtr);
  void setThermalZoomRef(float* zoomPtr);
  void setThermalCenterRefs(float* cxPtr, float* cyPtr);
  void setThermalPaletteRef(ThermalPalette* palettePtr);
  void setThermalHotspotRefs(ThermalHotspotMode* modePtr, int* xPtr, int* yPtr);
  void setThermalThresholdRefs(ThermalThresholdMode* modePtr, float* thresholdPtr);
  void setThermalPointerCallback(std::function<void(int,int)> cb);

private:
  void initWiFiAP();
  void initRoutes();

  String htmlHeader(const String& title);
  String htmlFooter();

  void handleRoot();
  void handleOverviewPage();
  void handleLivePage();
  void handleLiveVibrationPage();
  void handleLiveVibrationFFTPage();
  void handleLiveTemperaturePage();
  void handleLiveThermalPage();
  void handleLiveSoundPage();
  void handleLiveSoundFFTPage();
  void handleSessions();
  void handleFiles();
  void handleDownload();
  void handleDelete();
  void handleDeleteSession();
  void handleEditSessionPage();
  void handleSaveSessionMeta();
  void handleComparePage();
  void handleRecordStart();
  void handleRecordStop();
  void handleSetVibrationFFTaxis();
  void handleSetManualOverride();
  void handleSetThermalRangeMode();
  void handleSetThermalZoom();
  void handleSetThermalCenter();
  void handleSetThermalPalette();
  void handleSetThermalHotspotMode();
  void handleSetThermalHotspotPoint();
  void handleSetThermalThreshold();
  void handleSetThermalPointer();
  void handleStatusJson();
  void handleLiveJson();
  void handleAnalysisJson();
  void handleThermalFrameJson();
  void handleSoundSpectrumJson();
  void handleVibrationSpectrumJson();

  void handleViewPage();
  void handleFileRaw();

  SDManager& sd;
  Recorder& recorder;
  LiveData& live;
  WebServer server;
  std::function<bool()> startRecordingCb;
  std::function<void()> stopRecordingCb;
  String* statusMessagePtr = nullptr;
  VibrationAxis* vibrationFFTaxisPtr = nullptr;
  bool* manualOverridePtr = nullptr;
  ThermalRangeMode* thermalRangeModePtr = nullptr;
  float* thermalFixedMinPtr = nullptr;
  float* thermalFixedMaxPtr = nullptr;
  float* thermalZoomPtr = nullptr;
  float* thermalCenterXPtr = nullptr;
  float* thermalCenterYPtr = nullptr;
  ThermalPalette* thermalPalettePtr = nullptr;
  ThermalHotspotMode* thermalHotspotModePtr = nullptr;
  int* thermalHotspotXPtr = nullptr;
  int* thermalHotspotYPtr = nullptr;
  ThermalThresholdMode* thermalThresholdModePtr = nullptr;
  float* thermalThresholdFPtr = nullptr;
  std::function<void(int,int)> thermalPointerSetter;
};
