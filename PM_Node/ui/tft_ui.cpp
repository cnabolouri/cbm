#include "tft_ui.h"
#include "../config.h"
#include <math.h>

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static uint16_t thermalColor565(float tempF, float minF, float maxF, ThermalPalette palette) {
  float t = (tempF - minF) / ((maxF - minF) > 0.001f ? (maxF - minF) : 0.001f);
  t = clampf(t, 0.0f, 1.0f);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  switch (palette) {
    case THERMAL_PALETTE_GRAYSCALE: {
      uint8_t v = (uint8_t)(255.0f * t);
      r = v;
      g = v;
      b = v;
      break;
    }

    case THERMAL_PALETTE_RAINBOW: {
      if (t < 0.25f) {
        float u = t / 0.25f;
        r = 0;
        g = (uint8_t)(255.0f * u);
        b = 255;
      } else if (t < 0.50f) {
        float u = (t - 0.25f) / 0.25f;
        r = 0;
        g = 255;
        b = (uint8_t)(255.0f * (1.0f - u));
      } else if (t < 0.75f) {
        float u = (t - 0.50f) / 0.25f;
        r = (uint8_t)(255.0f * u);
        g = 255;
        b = 0;
      } else {
        float u = (t - 0.75f) / 0.25f;
        r = 255;
        g = (uint8_t)(255.0f * (1.0f - u));
        b = 0;
      }
      break;
    }

    case THERMAL_PALETTE_IRON:
    default: {
      if (t < 0.33f) {
        float u = t / 0.33f;
        r = (uint8_t)(80.0f * u);
        g = 0;
        b = (uint8_t)(40.0f + 100.0f * u);
      } else if (t < 0.66f) {
        float u = (t - 0.33f) / 0.33f;
        r = (uint8_t)(80.0f + 120.0f * u);
        g = (uint8_t)(40.0f * u);
        b = (uint8_t)(140.0f * (1.0f - u));
      } else {
        float u = (t - 0.66f) / 0.34f;
        r = 255;
        g = (uint8_t)(60.0f + 195.0f * u);
        b = (uint8_t)(20.0f * (1.0f - u));
      }
      break;
    }
  }

  return rgb565(r, g, b);
}

static float bilinearSampleF(const float* arr, int w, int h, float x, float y) {
  int x0 = (int)x;
  int y0 = (int)y;
  int x1 = min(w - 1, x0 + 1);
  int y1 = min(h - 1, y0 + 1);

  float tx = x - x0;
  float ty = y - y0;

  float q00 = arr[y0 * w + x0];
  float q10 = arr[y0 * w + x1];
  float q01 = arr[y1 * w + x0];
  float q11 = arr[y1 * w + x1];

  float a = q00 * (1.0f - tx) + q10 * tx;
  float b = q01 * (1.0f - tx) + q11 * tx;
  return a * (1.0f - ty) + b * ty;
}

static void getThermalCropWindow(
  const ThermalDisplayState& disp,
  float& x0, float& y0, float& cropW, float& cropH
) {
  float zoom = disp.zoom;
  if (zoom < 1.0f) zoom = 1.0f;
  if (zoom > 3.0f) zoom = 3.0f;

  cropW = THERMAL_W / zoom;
  cropH = THERMAL_H / zoom;

  float halfW = (cropW - 1.0f) * 0.5f;
  float halfH = (cropH - 1.0f) * 0.5f;

  x0 = disp.centerX - halfW;
  y0 = disp.centerY - halfH;

  if (x0 < 0.0f) x0 = 0.0f;
  if (y0 < 0.0f) y0 = 0.0f;
  if (x0 + cropW > THERMAL_W) x0 = THERMAL_W - cropW;
  if (y0 + cropH > THERMAL_H) y0 = THERMAL_H - cropH;
  if (x0 < 0.0f) x0 = 0.0f;
  if (y0 < 0.0f) y0 = 0.0f;
}

TFTUI::TFTUI(Adafruit_ILI9341& display) : tft(display) {}

void TFTUI::begin() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
}

void TFTUI::setPage(Page page) {
  if (page != currentPage) {
    currentPage = page;
    lastDrawnPage = (Page)(-1);
  }
}

Page TFTUI::getPage() const {
  return currentPage;
}

void TFTUI::setTouchPoint(int x, int y, bool pressed) {
  bool wasPressed = lastTouchPressed;
  lastTouchX = x;
  lastTouchY = y;
  lastTouchPressed = pressed;

  if (!pressed || currentPage != PAGE_THERMAL) return;

  if (!wasPressed &&
      x >= 280 && x <= 319 &&
      y >= 26 && y <= 50) {
    thermalZoomCycleRequested = true;
    return;
  }

  if (!wasPressed &&
      x >= 280 && x <= 319 &&
      y >= 52 && y <= 76) {
    thermalPaletteCycleRequested = true;
    return;
  }

  if (!wasPressed &&
      x >= 280 && x <= 319 &&
      y >= 78 && y <= 102) {
    thermalHotspotToggleRequested = true;
    return;
  }

  if (!wasPressed &&
      x >= 280 && x <= 319 &&
      y >= 104 && y <= 128) {
    thermalThresholdCycleRequested = true;
    return;
  }

  const int barX = 300;
  const int barY = 35;
  const int barW = 10;
  const int barH = 150;

  if (!wasPressed &&
      x >= (barX - 10) && x <= (barX + barW + 10) &&
      y >= (barY - 10) && y <= (barY + barH + 10)) {
    thermalRangeToggleRequested = true;
    return;
  }

  const int imgX = 10;
  const int imgY = 30;
  const int imgW = 220;
  const int imgH = 180;

  if (x >= imgX && x < (imgX + imgW) &&
      y >= imgY && y < (imgY + imgH)) {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float cropW = THERMAL_W;
    float cropH = THERMAL_H;
    getThermalCropWindow(lastThermalDisplay, x0, y0, cropW, cropH);

    float tx = x0 + (float)(x - imgX) * (cropW - 1.0f) / (imgW - 1);
    float ty = y0 + (float)(y - imgY) * (cropH - 1.0f) / (imgH - 1);

    tx = clampf(tx, 0.0f, (float)(THERMAL_W - 1));
    ty = clampf(ty, 0.0f, (float)(THERMAL_H - 1));

    int newPX = (int)tx;
    int newPY = (int)ty;
    if (pendingThermalX < 0 || pendingThermalY < 0 ||
        abs(newPX - pendingThermalX) >= 1 || abs(newPY - pendingThermalY) >= 1) {
      pendingThermalX = newPX;
      pendingThermalY = newPY;
    }
    pendingThermalCenterX = tx;
    pendingThermalCenterY = ty;
    thermalHotspotLockX = newPX;
    thermalHotspotLockY = newPY;
  }
}

bool TFTUI::consumeThermalRangeToggleRequest() {
  bool requested = thermalRangeToggleRequested;
  thermalRangeToggleRequested = false;
  return requested;
}

bool TFTUI::consumeThermalZoomCycleRequest() {
  bool requested = thermalZoomCycleRequested;
  thermalZoomCycleRequested = false;
  return requested;
}

bool TFTUI::consumeThermalPaletteCycleRequest() {
  bool requested = thermalPaletteCycleRequested;
  thermalPaletteCycleRequested = false;
  return requested;
}

bool TFTUI::consumeThermalHotspotToggleRequest() {
  bool requested = thermalHotspotToggleRequested;
  thermalHotspotToggleRequested = false;
  return requested;
}

bool TFTUI::consumeThermalHotspotLockHereRequest(int& x, int& y) {
  if (thermalHotspotLockX < 0 || thermalHotspotLockY < 0) return false;

  x = thermalHotspotLockX;
  y = thermalHotspotLockY;
  thermalHotspotLockX = -1;
  thermalHotspotLockY = -1;
  return true;
}

bool TFTUI::consumeThermalThresholdCycleRequest() {
  bool requested = thermalThresholdCycleRequested;
  thermalThresholdCycleRequested = false;
  return requested;
}

bool TFTUI::getThermalPointerRequest(int& x, int& y) {
  if (pendingThermalX < 0 || pendingThermalY < 0) return false;

  x = pendingThermalX;
  y = pendingThermalY;
  pendingThermalX = -1;
  pendingThermalY = -1;
  return true;
}

bool TFTUI::getThermalPanRequest(float& x, float& y) {
  if (pendingThermalCenterX < 0.0f || pendingThermalCenterY < 0.0f) return false;

  x = pendingThermalCenterX;
  y = pendingThermalCenterY;
  pendingThermalCenterX = -1.0f;
  pendingThermalCenterY = -1.0f;
  return true;
}

void TFTUI::nextPage() {
  setPage((Page)((currentPage + 1) % 5));
}

void TFTUI::prevPage() {
  setPage((Page)((currentPage + 4) % 5));
}

void TFTUI::render(const LiveData& live, int touchX, int touchY) {
  pushHist(vibX, HISTORY_LEN, vibHead, live.vibration.x_in_s);
  pushHist(vibY, HISTORY_LEN, vibHead, live.vibration.y_in_s);
  pushHist(vibZ, HISTORY_LEN, vibHead, live.vibration.z_in_s);
  pushHist(vibT, HISTORY_LEN, vibHead, live.vibration.total_in_s);

  pushHist(tempObj, HISTORY_LEN, tempHead, live.temperature.objF);
  pushHist(tempRef, HISTORY_LEN, tempHead, live.temperature.refF);
  pushHist(tempAmb, HISTORY_LEN, tempHead, live.temperature.ambF);
  pushHist(tempDT, HISTORY_LEN, tempHead, live.temperature.deltaF);

  pushHist(sndDb, HISTORY_LEN, sndHead, live.sound.db);
  pushHist(sndHzScaled, HISTORY_LEN, sndHead, live.sound.hz / 100.0f);

  if (currentPage != lastDrawnPage) {
    switch (currentPage) {
      case PAGE_VIB:   drawVibrationChrome(); break;
      case PAGE_TEMP:  drawTemperatureChrome(); break;
      case PAGE_THERMAL: drawThermalChrome(); break;
      case PAGE_SOUND: drawSoundChrome(); break;
      case PAGE_SYS:   drawSystemChrome(); break;
    }
    lastDrawnPage = currentPage;
  }

  if (millis() - lastUiUpdateMs < UI_UPDATE_MS) return;
  lastUiUpdateMs = millis();

  switch (currentPage) {
    case PAGE_VIB:   updateVibration(live); break;
    case PAGE_TEMP:  updateTemperature(live); break;
    case PAGE_THERMAL: updateThermal(live); break;
    case PAGE_SOUND: updateSound(live); break;
    case PAGE_SYS:   updateSystem(live, touchX, touchY); break;
  }
}

void TFTUI::drawTabs() {
  const char* labels[5] = {"VIB", "TEMP", "THRM", "SND", "SYS"};
  for (int i = 0; i < 5; i++) {
    uint16_t bg = (currentPage == i) ? ILI9341_BLUE : ILI9341_DARKCYAN;
    tft.fillRect(i * 64, 204, 64, 36, bg);
    tft.drawRect(i * 64, 204, 64, 36, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(i * 64 + 14, 216);
    tft.print(labels[i]);
  }
}

void TFTUI::drawRecBadge(uint16_t bgColor) {
  tft.fillRect(250, 2, 64, 18, bgColor);
  if (bgColor == ILI9341_NAVY || bgColor == ILI9341_DARKGREEN || bgColor == ILI9341_MAROON || bgColor == ILI9341_PURPLE) {
    // background already filled
  }
}

void TFTUI::drawVibrationChrome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 22, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 4);
  tft.print("Vibration");

  tft.setTextSize(1);
  tft.fillRect(8, 18, 7, 7, ILI9341_RED);     tft.setCursor(18, 18);  tft.print("X");
  tft.fillRect(36, 18, 7, 7, ILI9341_GREEN);  tft.setCursor(46, 18);  tft.print("Y");
  tft.fillRect(64, 18, 7, 7, ILI9341_BLUE);   tft.setCursor(74, 18);  tft.print("Z");
  tft.fillRect(92, 18, 7, 7, ILI9341_YELLOW); tft.setCursor(102, 18); tft.print("TOT");

  tft.drawRect(6, 26, 308, 145, ILI9341_DARKGREY);

  tft.drawRect(4,   176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(82,  176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(160, 176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(238, 176, 74, 24, ILI9341_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(8,   166); tft.print("X in/s");
  tft.setCursor(86,  166); tft.print("Y in/s");
  tft.setCursor(164, 166); tft.print("Z in/s");
  tft.setCursor(242, 166); tft.print("TOT");

  drawTabs();
}

void TFTUI::drawTemperatureChrome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 22, ILI9341_DARKGREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 4);
  tft.print("Temperature");

  tft.drawRect(6, 26, 308, 145, ILI9341_DARKGREY);

  tft.drawRect(4,   176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(82,  176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(160, 176, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(238, 176, 74, 24, ILI9341_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(8,   166); tft.print("Obj F");
  tft.setCursor(86,  166); tft.print("Ref F");
  tft.setCursor(164, 166); tft.print("Amb F");
  tft.setCursor(242, 166); tft.print("dT F");

  drawTabs();
}

void TFTUI::drawThermalChrome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.print("Thermal");

  drawTabs();
}

void TFTUI::drawSoundChrome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 22, ILI9341_MAROON);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 4);
  tft.print("Sound");

  tft.drawRect(6, 26, 308, 115, ILI9341_DARKGREY);

  tft.drawRect(4,   150, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(82,  150, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(160, 150, 74, 24, ILI9341_DARKGREY);
  tft.drawRect(238, 150, 74, 24, ILI9341_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(8,   140); tft.print("Lvl dB");
  tft.setCursor(86,  140); tft.print("Freq");
  tft.setCursor(164, 140); tft.print("RMS");
  tft.setCursor(242, 140); tft.print("Peak");

  tft.drawRect(10, 182, 300, 14, ILI9341_WHITE);

  drawTabs();
}

void TFTUI::drawSystemChrome() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 22, ILI9341_PURPLE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 4);
  tft.print("System");

  tft.drawRect(8, 30, 304, 74, ILI9341_DARKGREY);
  tft.drawRect(8, 110, 304, 88, ILI9341_DARKGREY);

  drawTabs();
}

void TFTUI::updateVibration(const LiveData& live) {
  tft.fillRect(7, 27, 306, 143, ILI9341_BLACK);
  drawGrid(6, 26, 308, 145);

  float scale = vibrationScale();
  drawHistoryLine(vibX, vibHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_RED, true);
  drawHistoryLine(vibY, vibHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_GREEN, true);
  drawHistoryLine(vibZ, vibHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_BLUE, true);
  drawHistoryLine(vibT, vibHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_YELLOW, true);

  tft.fillRect(248, 26, 64, 10, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(250, 28);
  tft.print("FS:");
  tft.print(scale, 4);

  tft.fillRect(8,   181, 64, 14, ILI9341_BLACK);
  tft.fillRect(86,  181, 64, 14, ILI9341_BLACK);
  tft.fillRect(164, 181, 64, 14, ILI9341_BLACK);
  tft.fillRect(242, 181, 64, 14, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8,   183); printAdaptive(live.vibration.x_in_s);
  tft.setCursor(86,  183); printAdaptive(live.vibration.y_in_s);
  tft.setCursor(164, 183); printAdaptive(live.vibration.z_in_s);
  tft.setCursor(242, 183); printAdaptive(live.vibration.total_in_s);
}

void TFTUI::updateTemperature(const LiveData& live) {
  tft.fillRect(7, 27, 306, 143, ILI9341_BLACK);
  drawGrid(6, 26, 308, 145);

  float scale = temperatureScale();
  drawHistoryLine(tempObj, tempHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_YELLOW, false);
  drawHistoryLine(tempRef, tempHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_CYAN, false);
  drawHistoryLine(tempAmb, tempHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_ORANGE, false);
  drawHistoryLine(tempDT,  tempHead, HISTORY_LEN, 7, 27, 306, 143, scale, ILI9341_MAGENTA, false);

  tft.fillRect(248, 26, 64, 10, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(250, 28);
  tft.print("FS:");
  tft.print(scale, 1);

  tft.fillRect(8,   181, 64, 14, ILI9341_BLACK);
  tft.fillRect(86,  181, 64, 14, ILI9341_BLACK);
  tft.fillRect(164, 181, 64, 14, ILI9341_BLACK);
  tft.fillRect(242, 181, 64, 14, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8,   183); printAdaptive(live.temperature.objF);
  tft.setCursor(86,  183); printAdaptive(live.temperature.refF);
  tft.setCursor(164, 183); printAdaptive(live.temperature.ambF);
  tft.setCursor(242, 183); printAdaptive(live.temperature.deltaF);
}

void TFTUI::updateThermal(const LiveData& live) {
  drawThermalPage(live);
}

void TFTUI::drawThermalPage(const LiveData& live) {
  lastThermalDisplay = live.thermalDisplay;

  const int imgX = 10;
  const int imgY = 30;
  const int imgW = 220;
  const int imgH = 180;

  if (!live.thermal.valid) {
    tft.fillRect(0, 24, 320, 180, ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 40);
    tft.print("No thermal frame");
    return;
  }

  float imageMinF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO)
    ? live.thermal.minF
    : live.thermalDisplay.fixedMinF;
  float imageMaxF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO)
    ? live.thermal.maxF
    : live.thermalDisplay.fixedMaxF;
  float scaleMinF = live.thermal.minF;
  float scaleMaxF = live.thermal.maxF;

  int effectiveHotX = live.thermal.hotspotX;
  int effectiveHotY = live.thermal.hotspotY;
  float effectiveHotF = live.thermal.hotspotF;
  if (live.thermalDisplay.hotspotMode == THERMAL_HOTSPOT_LOCKED) {
    effectiveHotX = live.thermalDisplay.lockedHotspotX;
    effectiveHotY = live.thermalDisplay.lockedHotspotY;
    effectiveHotF = live.thermalDisplay.lockedHotspotF;
  }

  float x0 = 0.0f;
  float y0 = 0.0f;
  float cropW = THERMAL_W;
  float cropH = THERMAL_H;
  getThermalCropWindow(live.thermalDisplay, x0, y0, cropW, cropH);

  for (int dy = 0; dy < THERMAL_DRAW_H_TFT; dy++) {
    for (int dx = 0; dx < THERMAL_DRAW_W_TFT; dx++) {
      float sx = x0 + (float)dx * (cropW - 1.0f) / (THERMAL_DRAW_W_TFT - 1);
      float sy = y0 + (float)dy * (cropH - 1.0f) / (THERMAL_DRAW_H_TFT - 1);

      float v = bilinearSampleF(live.thermal.pixelsF, THERMAL_W, THERMAL_H, sx, sy);
      uint16_t color = thermalColor565(v, imageMinF, imageMaxF, live.thermalDisplay.palette);
      bool aboveThreshold = (live.thermalDisplay.thresholdMode == THERMAL_THRESHOLD_ABOVE &&
                             v >= live.thermalDisplay.thresholdF);
      int px = imgX + dx * imgW / THERMAL_DRAW_W_TFT;
      int py = imgY + dy * imgH / THERMAL_DRAW_H_TFT;
      int pw = max(1, imgW / THERMAL_DRAW_W_TFT + 1);
      int ph = max(1, imgH / THERMAL_DRAW_H_TFT + 1);
      tft.fillRect(px, py, pw, ph, color);
      if (aboveThreshold && pw >= 2 && ph >= 2) {
        tft.drawPixel(px, py, ILI9341_YELLOW);
      }
    }
  }

  tft.drawRect(imgX - 1, imgY - 1, imgW + 2, imgH + 2, ILI9341_WHITE);

  if (live.thermal.thresholdRegion.valid) {
    int bx0 = imgX + (int)(((live.thermal.thresholdRegion.minX - x0) / max(0.001f, cropW - 1.0f)) * imgW);
    int by0 = imgY + (int)(((live.thermal.thresholdRegion.minY - y0) / max(0.001f, cropH - 1.0f)) * imgH);
    int bx1 = imgX + (int)(((live.thermal.thresholdRegion.maxX - x0) / max(0.001f, cropW - 1.0f)) * imgW);
    int by1 = imgY + (int)(((live.thermal.thresholdRegion.maxY - y0) / max(0.001f, cropH - 1.0f)) * imgH);
    int bw = bx1 - bx0;
    int bh = by1 - by0;
    if (bw > 0 && bh > 0) {
      tft.drawRect(bx0, by0, bw, bh, ILI9341_YELLOW);
    }
  }

  int hx = imgX + (int)(((effectiveHotX - x0) / max(0.001f, cropW - 1.0f)) * imgW);
  int hy = imgY + (int)(((effectiveHotY - y0) / max(0.001f, cropH - 1.0f)) * imgH);
  if (hx >= imgX && hx < imgX + imgW && hy >= imgY && hy < imgY + imgH) {
    tft.drawLine(hx - 6, hy, hx + 6, hy, ILI9341_RED);
    tft.drawLine(hx, hy - 6, hx, hy + 6, ILI9341_RED);
  }

  int px = imgX + (int)(((live.thermal.pointerX - x0) / max(0.001f, cropW - 1.0f)) * imgW);
  int py = imgY + (int)(((live.thermal.pointerY - y0) / max(0.001f, cropH - 1.0f)) * imgH);
  if (px >= imgX && px < imgX + imgW && py >= imgY && py < imgY + imgH) {
    tft.drawLine(px - 5, py, px + 5, py, ILI9341_WHITE);
    tft.drawLine(px, py - 5, px, py + 5, ILI9341_WHITE);
  }

  tft.fillRect(236, 30, 58, 174, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);

  int tx = 240;
  tft.setCursor(tx, 35);  tft.print("Hot:");
  tft.setCursor(tx, 48);  tft.print(effectiveHotF, 1); tft.print("F");

  tft.setCursor(tx, 72);  tft.print("Ptr:");
  tft.setCursor(tx, 85);  tft.print(live.thermal.pointerDisplayF, 1); tft.print("F");

  tft.setCursor(tx, 109); tft.print("Min:");
  tft.setCursor(tx, 122); tft.print(imageMinF, 1);

  tft.setCursor(tx, 146); tft.print("Max:");
  tft.setCursor(tx, 159); tft.print(imageMaxF, 1);

  tft.setCursor(tx, 183); tft.print("XY:");
  tft.print(live.thermal.pointerX);
  tft.print(",");
  tft.print(live.thermal.pointerY);

  tft.setCursor(tx, 196); tft.print("C:");
  tft.print((int)live.thermalDisplay.centerX);
  tft.print(",");
  tft.print((int)live.thermalDisplay.centerY);

  if (live.thermal.thresholdRegion.valid) {
    tft.setCursor(tx, 208); tft.print("HP:");
    tft.print(live.thermal.thresholdRegion.pixelCount);
    tft.setCursor(tx, 220); tft.print("A:");
    tft.print(live.thermal.thresholdRegion.percentOfFrame, 0);
    tft.print("%");
    tft.setCursor(tx, 232); tft.print("Av:");
    tft.print(live.thermal.thresholdRegion.avgF, 0);
  }

  const int barX = 300;
  const int barY = 35;
  const int barW = 10;
  const int barH = 150;

  for (int i = 0; i < barH; i++) {
    float f = scaleMaxF - (float)i * (scaleMaxF - scaleMinF) / (barH - 1);
    tft.drawFastHLine(barX, barY + i, barW, thermalColor565(f, scaleMinF, scaleMaxF, live.thermalDisplay.palette));
  }
  tft.drawRect(barX - 1, barY - 1, barW + 2, barH + 2, ILI9341_WHITE);

  tft.setCursor(barX - 4, barY - 18);
  tft.print("F");

  if (live.thermalDisplay.thresholdMode == THERMAL_THRESHOLD_ABOVE) {
    float thresholdF = live.thermalDisplay.thresholdF;
    int thrBarY = barY + (int)(((scaleMaxF - thresholdF) / max(0.001f, scaleMaxF - scaleMinF)) * barH);
    if (thrBarY >= barY && thrBarY <= barY + barH) {
      tft.drawFastHLine(barX - 4, thrBarY, barW + 8, ILI9341_YELLOW);
      tft.setCursor(barX + 18, thrBarY - 4);
      tft.setTextColor(ILI9341_YELLOW);
      tft.print("T");
      tft.setTextColor(ILI9341_WHITE);
    }
  }

  int hotBarY = barY + (int)(((scaleMaxF - effectiveHotF) / max(0.001f, scaleMaxF - scaleMinF)) * barH);
  if (hotBarY >= barY && hotBarY <= barY + barH) {
    tft.fillTriangle(barX + barW + 8, hotBarY, barX + barW + 1, hotBarY - 4, barX + barW + 1, hotBarY + 4, ILI9341_RED);
    tft.drawFastHLine(barX - 1, hotBarY, barW + 2, ILI9341_RED);
  }

  int ptrBarY = barY + (int)(((scaleMaxF - live.thermal.pointerDisplayF) / max(0.001f, scaleMaxF - scaleMinF)) * barH);
  if (ptrBarY >= barY && ptrBarY <= barY + barH) {
    tft.fillTriangle(barX - 8, ptrBarY, barX - 1, ptrBarY - 4, barX - 1, ptrBarY + 4, ILI9341_WHITE);
    tft.drawFastHLine(barX - 1, ptrBarY, barW + 2, ILI9341_WHITE);
  }

  tft.setCursor(barX - 8, barY - 10);
  tft.print((int)scaleMaxF);
  tft.setCursor(barX - 8, barY + barH + 4);
  tft.print((int)scaleMinF);

  tft.setCursor(240, 210);
  tft.print("Mode:");
  tft.setCursor(240, 223);
  tft.print(live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO ? "AUTO" : "FIXED");

  tft.fillRect(285, 28, 28, 18, ILI9341_BLACK);
  tft.drawRect(285, 28, 28, 18, ILI9341_WHITE);
  tft.setCursor(292, 33);
  tft.print("Z");

  tft.fillRect(285, 52, 28, 18, ILI9341_BLACK);
  tft.drawRect(285, 52, 28, 18, ILI9341_WHITE);
  tft.setCursor(289, 57);
  tft.print("P");

  tft.fillRect(285, 78, 28, 18, ILI9341_BLACK);
  tft.drawRect(285, 78, 28, 18, ILI9341_WHITE);
  tft.setCursor(289, 83);
  tft.print("H");

  tft.fillRect(285, 104, 28, 18, ILI9341_BLACK);
  tft.drawRect(285, 104, 28, 18, ILI9341_WHITE);
  tft.setCursor(289, 109);
  tft.print("T");

  tft.setCursor(240, 236);
  tft.print("Z:");
  tft.print(live.thermalDisplay.zoom, 1);
  tft.print("x");

  tft.setCursor(240, 248);
  switch (live.thermalDisplay.palette) {
    case THERMAL_PALETTE_RAINBOW: tft.print("RBOW"); break;
    case THERMAL_PALETTE_GRAYSCALE: tft.print("GRAY"); break;
    default: tft.print("IRON"); break;
  }

  tft.setCursor(240, 260);
  tft.print(live.thermalDisplay.hotspotMode == THERMAL_HOTSPOT_AUTO ? "HAUTO" : "HLOCK");

  tft.setCursor(240, 272);
  if (live.thermalDisplay.thresholdMode == THERMAL_THRESHOLD_OFF) {
    tft.print("THR OFF");
  } else {
    tft.print("THR ");
    tft.print(live.thermalDisplay.thresholdF, 0);
  }
}

void TFTUI::updateSound(const LiveData& live) {
  tft.fillRect(7, 27, 306, 113, ILI9341_BLACK);
  drawGrid(6, 26, 308, 115);

  float scale = soundScale();
  drawHistoryLine(sndDb, sndHead, HISTORY_LEN, 7, 27, 306, 113, scale, ILI9341_GREEN, false);
  drawHistoryLine(sndHzScaled, sndHead, HISTORY_LEN, 7, 27, 306, 113, scale, ILI9341_YELLOW, false);

  tft.fillRect(248, 26, 64, 10, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(250, 28);
  tft.print("FS:");
  tft.print(scale, 1);

  tft.fillRect(8,   155, 64, 14, ILI9341_BLACK);
  tft.fillRect(86,  155, 64, 14, ILI9341_BLACK);
  tft.fillRect(164, 155, 64, 14, ILI9341_BLACK);
  tft.fillRect(242, 155, 64, 14, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8,   157); printAdaptive(live.sound.db);
  tft.setCursor(86,  157); printAdaptive(live.sound.hz);
  tft.setCursor(164, 157); printAdaptive(live.sound.rms);
  tft.setCursor(242, 157); tft.print(live.sound.peak);

  tft.fillRect(11, 183, 298, 12, ILI9341_BLACK);
  int fillW = (int)((live.sound.rms / (scale > 0 ? scale : 1.0f)) * 298.0f);
  if (fillW < 0) fillW = 0;
  if (fillW > 298) fillW = 298;
  tft.fillRect(11, 183, fillW, 12, ILI9341_GREEN);
}

void TFTUI::updateSystem(const LiveData& live, int touchX, int touchY) {
  tft.fillRect(12, 34, 296, 66, ILI9341_BLACK);
  tft.fillRect(12, 114, 296, 80, ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.setCursor(16, 36);
  tft.print("SD: ");
  tft.print(live.system.sdOK ? "OK" : "FAIL");

  tft.setCursor(16, 58);
  tft.print("REC: ");
  tft.print(live.system.recording ? "ON" : "OFF");

  tft.setCursor(16, 80);
  tft.print("Touch:");
  tft.print(touchX);
  tft.print(",");
  tft.print(touchY);

  tft.setCursor(16, 116);
  tft.print("File:");
  if (live.system.currentBaseName.length()) tft.print(live.system.currentBaseName);
  else tft.print("None");

  tft.setCursor(16, 140);
  tft.print("Vtot:");
  printAdaptive(live.vibration.total_in_s);

  tft.setTextSize(1);
  tft.setCursor(16, 162);
  tft.print("Mount:");
  switch (live.mount.state) {
    case MOUNT_MOVING: tft.print("MOVING"); break;
    case MOUNT_STABILIZING: tft.print("STABIL"); break;
    case MOUNT_MONITORING: tft.print("MONITOR"); break;
    default: tft.print("UNKNOWN"); break;
  }

  tft.setCursor(112, 162);
  tft.print("Conf:");
  tft.print(live.mount.confidence, 0);
  tft.print("%");

  tft.setCursor(210, 162);
  tft.print("Trust:");
  tft.print(live.mount.analysisTrusted ? "YES" : "NO");

  tft.setCursor(16, 180);
  tft.print("Dyn:");
  tft.print(live.mount.dynamicG, 3);
  tft.print("g Drift:");
  tft.print(live.mount.driftG, 3);
  tft.print("g");

  tft.setCursor(16, 192);
  tft.print("Mode:");
  switch (live.system.recordingMode) {
    case REC_ACTIVE: tft.print("ACTIVE"); break;
    case REC_ARMED_WAITING_MOUNT: tft.print("ARMED"); break;
    default: tft.print("IDLE"); break;
  }
  tft.print(" Ovr:");
  tft.print(live.system.manualOverride ? "ON" : "OFF");
}

void TFTUI::drawGrid(int x, int y, int w, int h) {
  for (int i = 1; i < 4; i++) {
    int gy = y + (h * i) / 4;
    tft.drawFastHLine(x + 1, gy, w - 2, ILI9341_DARKGREY);
  }
  for (int i = 1; i < 6; i++) {
    int gx = x + (w * i) / 6;
    tft.drawFastVLine(gx, y + 1, h - 2, ILI9341_DARKGREY);
  }
}

void TFTUI::drawHistoryLine(float *arr, int head, int len, int x, int y, int w, int h, float maxVal, uint16_t color, bool absoluteValue) {
  if (maxVal < 0.001f) maxVal = 0.001f;

  int lastX = -1;
  int lastY = -1;

  for (int i = 0; i < len; i++) {
    int pos = (head + i) % len;
    float v = arr[pos];
    if (absoluteValue) v = fabs(v);
    if (v < 0) v = 0;
    if (v > maxVal) v = maxVal;

    int px = x + (i * (w - 1)) / (len - 1);
    int py = y + h - 1 - (int)((v / maxVal) * (h - 1));

    if (lastX >= 0) tft.drawLine(lastX, lastY, px, py, color);
    lastX = px;
    lastY = py;
  }
}

void TFTUI::pushHist(float *arr, int len, int &head, float value) {
  arr[head] = value;
  head = (head + 1) % len;
}

float TFTUI::maxAbs(const float *arr, int len) const {
  float m = 0.0f;
  for (int i = 0; i < len; i++) {
    float v = fabs(arr[i]);
    if (v > m) m = v;
  }
  return m;
}

float TFTUI::vibrationScale() const {
  float m = maxAbs(vibX, HISTORY_LEN);
  float v = maxAbs(vibY, HISTORY_LEN); if (v > m) m = v;
  v = maxAbs(vibZ, HISTORY_LEN); if (v > m) m = v;
  v = maxAbs(vibT, HISTORY_LEN); if (v > m) m = v;

  if (m < 0.001f) m = 0.001f;
  m *= 1.35f;

  if (m <= 0.002f) return 0.002f;
  if (m <= 0.005f) return 0.005f;
  if (m <= 0.01f) return 0.01f;
  if (m <= 0.02f) return 0.02f;
  if (m <= 0.05f) return 0.05f;
  if (m <= 0.10f) return 0.10f;
  if (m <= 0.20f) return 0.20f;
  if (m <= 0.50f) return 0.50f;
  if (m <= 1.00f) return 1.00f;
  return 2.00f;
}

float TFTUI::temperatureScale() const {
  float m = 0.0f;
  for (int i = 0; i < HISTORY_LEN; i++) {
    if (tempObj[i] > m) m = tempObj[i];
    if (tempRef[i] > m) m = tempRef[i];
    if (tempAmb[i] > m) m = tempAmb[i];
    if (tempDT[i] > m) m = tempDT[i];
  }
  if (m < 80.0f) m = 80.0f;
  return m;
}

float TFTUI::soundScale() const {
  float m = 0.0f;
  for (int i = 0; i < HISTORY_LEN; i++) {
    if (sndDb[i] > m) m = sndDb[i];
    if (sndHzScaled[i] > m) m = sndHzScaled[i];
  }
  if (m < 10.0f) m = 10.0f;
  return m * 1.2f;
}

void TFTUI::printAdaptive(float v) {
  float av = fabs(v);
  if (av >= 10.0f) tft.print(v, 1);
  else if (av >= 1.0f) tft.print(v, 2);
  else if (av >= 0.1f) tft.print(v, 3);
  else if (av >= 0.01f) tft.print(v, 4);
  else tft.print(v, 5);
}
