#include "tft_ui.h"
#include "../config.h"
#include <math.h>

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

void TFTUI::nextPage() {
  setPage((Page)((currentPage + 1) % 4));
}

void TFTUI::prevPage() {
  setPage((Page)((currentPage + 3) % 4));
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
    case PAGE_SOUND: updateSound(live); break;
    case PAGE_SYS:   updateSystem(live, touchX, touchY); break;
  }
}

void TFTUI::drawTabs() {
  const char* labels[4] = {"VIB", "TEMP", "SND", "SYS"};
  for (int i = 0; i < 4; i++) {
    uint16_t bg = (currentPage == i) ? ILI9341_BLUE : ILI9341_DARKCYAN;
    tft.fillRect(i * 80, 204, 80, 36, bg);
    tft.drawRect(i * 80, 204, 80, 36, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(i * 80 + 14, 214);
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
