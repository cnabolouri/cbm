#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "../types.h"

class TFTUI {
public:
  TFTUI(Adafruit_ILI9341& display);

  void begin();
  void setPage(Page page);
  Page getPage() const;
  void nextPage();
  void prevPage();

  bool getThermalPointerRequest(int& x, int& y);
  bool getThermalPanRequest(float& x, float& y);
  void setTouchPoint(int x, int y, bool pressed);
  bool consumeThermalRangeToggleRequest();
  bool consumeThermalZoomCycleRequest();
  bool consumeThermalPaletteCycleRequest();
  bool consumeThermalHotspotToggleRequest();
  bool consumeThermalHotspotLockHereRequest(int& x, int& y);
  bool consumeThermalThresholdCycleRequest();
  void render(const LiveData& live, int touchX, int touchY);

private:
  void drawTabs();
  void drawRecBadge(uint16_t bgColor);

  void drawDashboardChrome();
  void drawVibrationChrome();
  void drawTemperatureChrome();
  void drawThermalChrome();
  void drawSoundChrome();
  void drawSystemChrome();

  void updateDashboard(const LiveData& live);
  void updateVibration(const LiveData& live);
  void updateTemperature(const LiveData& live);
  void updateThermal(const LiveData& live);
  void drawThermalPage(const LiveData& live);
  void updateSound(const LiveData& live);
  void updateSystem(const LiveData& live, int touchX, int touchY);

  void drawGrid(int x, int y, int w, int h);
  void drawHistoryLine(float *arr, int head, int len, int x, int y, int w, int h, float maxVal, uint16_t color, bool absoluteValue);
  void pushHist(float *arr, int len, int &head, float value);
  float maxAbs(const float *arr, int len) const;
  float vibrationScale() const;
  float temperatureScale() const;
  float soundScale() const;
  void printAdaptive(float v);

  Adafruit_ILI9341& tft;
  Page currentPage = PAGE_VIB;
  Page lastDrawnPage = (Page)(-1);
  unsigned long lastUiUpdateMs = 0;
  const unsigned long UI_UPDATE_MS = 250;

  int lastTouchX = 0;
  int lastTouchY = 0;
  bool lastTouchPressed = false;

  int pendingThermalX = -1;
  int pendingThermalY = -1;
  float pendingThermalCenterX = -1.0f;
  float pendingThermalCenterY = -1.0f;
  bool thermalRangeToggleRequested = false;
  bool thermalZoomCycleRequested = false;
  bool thermalPaletteCycleRequested = false;
  bool thermalHotspotToggleRequested = false;
  bool thermalThresholdCycleRequested = false;
  int thermalHotspotLockX = -1;
  int thermalHotspotLockY = -1;
  ThermalDisplayState lastThermalDisplay;

  static const int HISTORY_LEN = 140;
  float vibX[HISTORY_LEN] = {0};
  float vibY[HISTORY_LEN] = {0};
  float vibZ[HISTORY_LEN] = {0};
  float vibT[HISTORY_LEN] = {0};
  int vibHead = 0;

  float tempObj[HISTORY_LEN] = {0};
  float tempRef[HISTORY_LEN] = {0};
  float tempAmb[HISTORY_LEN] = {0};
  float tempDT[HISTORY_LEN] = {0};
  int tempHead = 0;

  float sndDb[HISTORY_LEN] = {0};
  float sndHzScaled[HISTORY_LEN] = {0};
  int sndHead = 0;
};
