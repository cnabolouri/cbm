#include "tft_ui.h"

#define COLOR_BLACK 0x0000
#define COLOR_BLUE 0x001F
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_CYAN 0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE 0xFFFF
#define COLOR_ORANGE 0xFD20
#define COLOR_MAGENTA 0xF81F
#define COLOR_GRAY 0x8410
#define COLOR_DARK 0x2104

static float clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static const int TEMP_HISTORY_LEN = 120;
static float tempHistory[TEMP_HISTORY_LEN] = {0};
static int tempHistoryIndex = 0;
static bool tempHistoryFilled = false;
static unsigned long lastTempHistoryMs = 0;

static const int VIB_HISTORY_LEN = 120;
static float vibXHistory[VIB_HISTORY_LEN] = {0};
static float vibYHistory[VIB_HISTORY_LEN] = {0};
static float vibZHistory[VIB_HISTORY_LEN] = {0};
static float vibTHistory[VIB_HISTORY_LEN] = {0};
static int vibHistoryIndex = 0;
static bool vibHistoryFilled = false;
static unsigned long lastVibHistoryMs = 0;

static const int SOUND_HISTORY_LEN = 120;
static float soundHistory[SOUND_HISTORY_LEN] = {0};
static int soundHistoryIndex = 0;
static bool soundHistoryFilled = false;
static unsigned long lastSoundHistoryMs = 0;

static void pushTempHistory(float v) {
  tempHistory[tempHistoryIndex] = v;
  tempHistoryIndex++;
  if (tempHistoryIndex >= TEMP_HISTORY_LEN) {
    tempHistoryIndex = 0;
    tempHistoryFilled = true;
  }
}

static void pushVibHistory(float vx, float vy, float vz, float vt) {
  vibXHistory[vibHistoryIndex] = vx;
  vibYHistory[vibHistoryIndex] = vy;
  vibZHistory[vibHistoryIndex] = vz;
  vibTHistory[vibHistoryIndex] = vt;
  vibHistoryIndex++;
  if (vibHistoryIndex >= VIB_HISTORY_LEN) {
    vibHistoryIndex = 0;
    vibHistoryFilled = true;
  }
}

static void pushSoundHistory(float v) {
  soundHistory[soundHistoryIndex] = v;
  soundHistoryIndex++;
  if (soundHistoryIndex >= SOUND_HISTORY_LEN) {
    soundHistoryIndex = 0;
    soundHistoryFilled = true;
  }
}

static void getHistoryRange(const float *data, int count, bool wrapped,
                            int writeIndex, float &minV, float &maxV) {
  int n = wrapped ? count : writeIndex;
  if (n <= 0) {
    minV = 0.0f;
    maxV = 1.0f;
    return;
  }

  int startIndex = wrapped ? writeIndex : 0;
  minV = data[startIndex];
  maxV = data[startIndex];

  for (int i = 1; i < n; i++) {
    float v = data[(startIndex + i) % count];
    if (v < minV)
      minV = v;
    if (v > maxV)
      maxV = v;
  }

  if ((maxV - minV) < 0.001f) {
    float pad = max(0.01f, fabs(maxV) * 0.05f);
    minV -= pad;
    maxV += pad;
  }
}

static int historyIndexAt(int i, int count, bool wrapped, int writeIndex) {
  return wrapped ? ((writeIndex + i) % count) : i;
}

static int plotYForValue(float v, float minV, float maxV, int plotY,
                         int plotH) {
  float norm = (v - minV) / max(0.001f, maxV - minV);
  norm = clampf(norm, 0.0f, 1.0f);
  return plotY + plotH - 1 - (int)(norm * (plotH - 1));
}

static void drawSingleTrendPlot(Arduino_GFX *gfx, const float *data, int count,
                                bool wrapped, int writeIndex, int x, int y,
                                int w, int h, uint16_t color,
                                const char *title) {
  gfx->fillRect(x, y, w, h, COLOR_DARK);
  gfx->drawRect(x, y, w, h, COLOR_WHITE);

  int n = wrapped ? count : writeIndex;
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(x + 6, y + 12);
  gfx->print(title);

  if (n <= 1)
    return;

  float minV, maxV;
  getHistoryRange(data, count, wrapped, writeIndex, minV, maxV);

  gfx->setCursor(x + w - 58, y + 12);
  gfx->print(maxV, 2);
  gfx->setCursor(x + 6, y + h - 7);
  gfx->print(minV, 2);

  const int plotX = x + 6;
  const int plotY = y + 20;
  const int plotW = w - 12;
  const int plotH = h - 30;

  int prevX = plotX;
  int prevY = plotYForValue(data[historyIndexAt(0, count, wrapped, writeIndex)],
                            minV, maxV, plotY, plotH);
  for (int i = 1; i < n; i++) {
    int idx = historyIndexAt(i, count, wrapped, writeIndex);
    float v = data[idx];
    float t = (n > 1) ? ((float)i / (float)(n - 1)) : 0.0f;
    int px = plotX + (int)(t * (plotW - 1));
    int py = plotYForValue(v, minV, maxV, plotY, plotH);
    gfx->drawLine(prevX, prevY, px, py, color);
    prevX = px;
    prevY = py;
  }
}

static void drawMultiVibrationTrendPlot(Arduino_GFX *gfx, int x, int y, int w,
                                        int h) {
  gfx->fillRect(x, y, w, h, COLOR_DARK);
  gfx->drawRect(x, y, w, h, COLOR_WHITE);

  int n = vibHistoryFilled ? VIB_HISTORY_LEN : vibHistoryIndex;
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(x + 6, y + 12);
  gfx->print("Trend");

  if (n <= 1)
    return;

  float minV = 0.0f;
  float maxV = 0.0f;
  bool first = true;
  const float *series[4] = {vibXHistory, vibYHistory, vibZHistory, vibTHistory};

  for (int s = 0; s < 4; s++) {
    for (int i = 0; i < n; i++) {
      float v = series[s][historyIndexAt(i, VIB_HISTORY_LEN, vibHistoryFilled,
                                         vibHistoryIndex)];
      if (first) {
        minV = v;
        maxV = v;
        first = false;
      } else {
        if (v < minV)
          minV = v;
        if (v > maxV)
          maxV = v;
      }
    }
  }

  if ((maxV - minV) < 0.001f) {
    float pad = max(0.01f, fabs(maxV) * 0.05f);
    minV -= pad;
    maxV += pad;
  }

  gfx->setCursor(x + w - 58, y + 12);
  gfx->print(maxV, 2);
  gfx->setCursor(x + 6, y + h - 7);
  gfx->print(minV, 2);

  gfx->setTextColor(COLOR_CYAN);
  gfx->setCursor(x + 52, y + 12);
  gfx->print("VX");
  gfx->setTextColor(COLOR_GREEN);
  gfx->setCursor(x + 82, y + 12);
  gfx->print("VY");
  gfx->setTextColor(COLOR_MAGENTA);
  gfx->setCursor(x + 112, y + 12);
  gfx->print("VZ");
  gfx->setTextColor(COLOR_YELLOW);
  gfx->setCursor(x + 142, y + 12);
  gfx->print("VT");

  const int plotX = x + 6;
  const int plotY = y + 22;
  const int plotW = w - 12;
  const int plotH = h - 32;

  auto drawSeries = [&](const float *data, uint16_t color) {
    int prevX = plotX;
    int prevY = plotYForValue(
        data[historyIndexAt(0, VIB_HISTORY_LEN, vibHistoryFilled,
                            vibHistoryIndex)],
        minV, maxV, plotY, plotH);

    for (int i = 1; i < n; i++) {
      int idx =
          historyIndexAt(i, VIB_HISTORY_LEN, vibHistoryFilled, vibHistoryIndex);
      float t = (float)i / (float)(n - 1);
      int px = plotX + (int)(t * (plotW - 1));
      int py = plotYForValue(data[idx], minV, maxV, plotY, plotH);
      gfx->drawLine(prevX, prevY, px, py, color);
      prevX = px;
      prevY = py;
    }
  };

  drawSeries(vibXHistory, COLOR_CYAN);
  drawSeries(vibYHistory, COLOR_GREEN);
  drawSeries(vibZHistory, COLOR_MAGENTA);
  drawSeries(vibTHistory, COLOR_YELLOW);
}

static float bilinearSampleF(const float *arr, int w, int h, float x, float y) {
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

static void getThermalCropWindow(const ThermalDisplayState &disp, float &x0,
                                 float &y0, float &cropW, float &cropH) {
  float zoom = disp.zoom;
  cropW = THERMAL_W / zoom;
  cropH = THERMAL_H / zoom;

  x0 = disp.centerX - cropW * 0.5f;
  y0 = disp.centerY - cropH * 0.5f;

  if (x0 < 0.0f)
    x0 = 0.0f;
  if (y0 < 0.0f)
    y0 = 0.0f;
  if (x0 + cropW > THERMAL_W)
    x0 = THERMAL_W - cropW;
  if (y0 + cropH > THERMAL_H)
    y0 = THERMAL_H - cropH;
}

uint16_t TFTUI::thermalColor565(float tempF, float minF, float maxF,
                                ThermalPalette palette) const {
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

bool TFTUI::begin() {
  bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO);
  gfx = new Arduino_ST7796(bus, TFT_RST, 1, false, 320, 480, 0, 0, 0, 0);

  if (!gfx->begin()) {
    return false;
  }

  gfx->setRotation(1);
  gfx->fillScreen(COLOR_BLACK);
  return true;
}

void TFTUI::drawHeader(const LiveData &live) {
  const char *pageName = "HOME";
  switch (live.ui.currentPage) {
  case PAGE_VIBRATION:
    pageName = "VIBRATION";
    break;
  case PAGE_VIBRATION_FFT:
    pageName = "VIB FFT";
    break;
  case PAGE_THERMAL:
    pageName = "THERMAL CAM";
    break;
  case PAGE_TEMPERATURE:
    pageName = "TEMP PROBE";
    break;
  case PAGE_SOUND:
    pageName = "SOUND";
    break;
  case PAGE_SOUND_FFT:
    pageName = "SND FFT";
    break;
  case PAGE_SYSTEM:
    pageName = "SYSTEM";
    break;
  default:
    pageName = "HOME";
    break;
  }

  gfx->fillRect(0, 0, TFT_W, 34, COLOR_GRAY);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(8, 8);
  gfx->print(pageName);

  gfx->fillRect(300, 0, 180, 34, COLOR_GRAY);
  gfx->setCursor(320, 8);
  gfx->setTextColor(live.ui.recOn ? COLOR_RED : COLOR_WHITE);
  gfx->print(live.ui.recOn ? "REC ON" : "REC OFF");
}

void TFTUI::drawFooter(const LiveData &live) {
  gfx->fillRect(0, TFT_H - 42, TFT_W, 42, COLOR_GRAY);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(8, TFT_H - 34);
  gfx->print("Page: ");
  gfx->print(live.ui.currentPage);

  gfx->setCursor(70, TFT_H - 34);
  gfx->print("REC: ");
  gfx->print(live.ui.recOn ? "ON" : "OFF");

  gfx->fillRect(128, TFT_H - 20, 344, 16, COLOR_GRAY);
  gfx->setCursor(130, TFT_H - 16);
  gfx->print(live.system.statusText);
}

void TFTUI::drawHomePageStatic() {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("PM NODE");

  gfx->setTextSize(1);
  gfx->setCursor(16, 92);
  gfx->print("Short press : next page");
  gfx->setCursor(16, 108);
  gfx->print("Long press  : REC on/off");
  gfx->setCursor(16, 124);
  gfx->print("Joystick    : inspect plots");

  gfx->fillRect(16, 150, 210, 110, COLOR_DARK);
  gfx->drawRect(16, 150, 210, 110, COLOR_WHITE);
  gfx->fillRect(250, 150, 210, 110, COLOR_DARK);
  gfx->drawRect(250, 150, 210, 110, COLOR_WHITE);
}

void TFTUI::drawHomePageDynamic(const LiveData &live) {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->fillRect(26, 160, 190, 90, COLOR_DARK);
  gfx->fillRect(260, 160, 190, 90, COLOR_DARK);

  gfx->setCursor(26, 164);
  gfx->print("Mount      : ");
  gfx->print(live.mount.attached ? "ATTACHED" : "NOT ATTACHED");

  gfx->setCursor(26, 182);
  gfx->print("Thermal    : ");
  gfx->print(live.thermal.valid ? "READY" : "NOT READY");

  gfx->setCursor(26, 200);
  gfx->print("Sound FFT  : ");
  gfx->print(live.soundSpectrum.valid ? "READY" : "NOT READY");

  gfx->setCursor(26, 218);
  gfx->print("Vib FFT    : ");
  gfx->print(live.vibrationSpectrum.valid ? "READY" : "NOT READY");

  gfx->setCursor(26, 236);
  gfx->print("REC        : ");
  gfx->print(live.ui.recOn ? "ON" : "OFF");

  gfx->setCursor(260, 164);
  gfx->print("Sound Level: ");
  gfx->print(live.sound.dbRel, 1);

  gfx->setCursor(260, 182);
  gfx->print("Sound Peak : ");
  gfx->print(live.soundSpectrum.valid ? live.soundSpectrum.dominantHz : 0.0f,
             1);
  gfx->print(" Hz");

  gfx->setCursor(260, 200);
  gfx->print("Vib Total  : ");
  gfx->print(live.vibration.vt, 3);

  gfx->setCursor(260, 218);
  gfx->print("Thermal Hot: ");
  if (live.thermal.valid) {
    gfx->print(live.thermal.hotspotF, 1);
    gfx->print(" F");
  } else {
    gfx->print("--");
  }
}

void TFTUI::drawVibrationPageStatic() {
  gfx->setTextColor(COLOR_CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Vibration");

  gfx->fillRect(16, 84, 180, 180, COLOR_DARK);
  gfx->drawRect(16, 84, 180, 180, COLOR_WHITE);

  gfx->fillRect(210, 84, 250, 180, COLOR_DARK);
  gfx->drawRect(210, 84, 250, 180, COLOR_WHITE);
}

void TFTUI::drawVibrationPageDynamic(const LiveData &live) {
  unsigned long now = millis();
  if (now - lastVibHistoryMs > 250) {
    lastVibHistoryMs = now;
    pushVibHistory(live.vibration.vx, live.vibration.vy, live.vibration.vz,
                   live.vibration.vt);
  }

  gfx->fillRect(24, 96, 160, 166, COLOR_DARK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(24, 100);
  gfx->print("VX: ");
  gfx->print(live.vibration.vx, 3);

  gfx->setCursor(24, 118);
  gfx->print("VY: ");
  gfx->print(live.vibration.vy, 3);

  gfx->setCursor(24, 136);
  gfx->print("VZ: ");
  gfx->print(live.vibration.vz, 3);

  gfx->setCursor(24, 154);
  gfx->print("VT: ");
  gfx->print(live.vibration.vt, 3);

  gfx->setCursor(24, 180);
  gfx->print("Max: ");
  gfx->print(live.vibration.maxTotal, 3);

  gfx->setCursor(24, 198);
  gfx->print("Avg: ");
  gfx->print(live.vibration.avgTotal, 3);

  gfx->setCursor(24, 216);
  gfx->print("Mount: ");
  gfx->print(live.mount.attached ? "STABLE" : "NOT MOUNTED");

  gfx->setCursor(24, 234);
  gfx->print("AX: ");
  gfx->print(live.vibration.ax_g, 3);

  gfx->setCursor(24, 248);
  gfx->print("AY: ");
  gfx->print(live.vibration.ay_g, 3);

  gfx->setCursor(24, 262);
  gfx->print("AZ: ");
  gfx->print(live.vibration.az_g, 3);

  drawMultiVibrationTrendPlot(gfx, 211, 85, 248, 178);
}

void TFTUI::drawThermalPageStatic() {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 56);
  gfx->print("Thermal");

  gfx->drawRect(9, 83, 252, 182, COLOR_WHITE);
  gfx->fillRect(275, 84, 150, 140, COLOR_DARK);
  gfx->drawRect(275, 84, 150, 140, COLOR_WHITE);
  gfx->drawRect(439, 89, 12, 152, COLOR_WHITE);
}

void TFTUI::drawVibrationFFTPageStatic() {
  gfx->setTextColor(COLOR_YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Vibration FFT");

  gfx->fillRect(16, 84, 444, 180, COLOR_DARK);
  gfx->drawRect(16, 84, 444, 180, COLOR_WHITE);
}

void TFTUI::drawVibrationFFTPageDynamic(const LiveData &live) {
  gfx->fillRect(16, 64, 220, 22, COLOR_BLACK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(16, 72);
  gfx->print("Dominant: ");
  if (live.vibrationSpectrum.valid) {
    gfx->print(live.vibrationSpectrum.dominantHz, 1);
    gfx->print(" Hz");
  } else {
    gfx->print("warming up...");
  }

  const int plotX = 17;
  const int plotY = 85;
  const int plotW = 442;
  const int plotH = 178;
  gfx->fillRect(plotX, plotY, plotW, plotH, COLOR_DARK);

  if (live.vibrationSpectrum.valid) {
    int binCount = live.vibrationSpectrum.bins;
    if (binCount > VibrationSpectrumData::MAX_BINS)
      binCount = VibrationSpectrumData::MAX_BINS;
    if (binCount <= 1)
      return;

    float maxBin = 0.001f;
    for (int i = 1; i < binCount; i++) {
      if (live.vibrationSpectrum.mag[i] > maxBin)
        maxBin = live.vibrationSpectrum.mag[i];
    }

    for (int i = 1; i < binCount; i++) {
      int x = plotX + (i * (plotW - 1)) / (binCount - 1);
      int h = (int)((live.vibrationSpectrum.mag[i] / maxBin) * (plotH - 4));
      if (h < 0)
        h = 0;
      if (h > plotH - 4)
        h = plotH - 4;
      gfx->drawFastVLine(x, plotY + plotH - 2 - h, h, COLOR_YELLOW);
    }

    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(plotX + 6, plotY + 12);
    gfx->print("Mag max ");
    gfx->print(maxBin, 4);
    gfx->setCursor(plotX + 6, plotY + plotH - 6);
    gfx->print("0 Hz");
    gfx->setCursor(plotX + plotW - 58, plotY + plotH - 6);
    gfx->print(live.vibrationSpectrum.hz[binCount - 1], 0);
    gfx->print(" Hz");
  }
}

void TFTUI::drawThermalPageDynamic(const LiveData &live) {
  if (!live.thermal.valid) {
    gfx->fillRect(16, 100, 220, 20, COLOR_BLACK);
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(16, 100);
    gfx->print("MLX90640 not ready");
    return;
  }

  const int imgX = 10;
  const int imgY = 84;
  const int imgW = 250;
  const int imgH = 180;
  const int cellW = imgW / THERMAL_W;
  const int cellH = imgH / THERMAL_H;

  float imageMinF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO)
                        ? live.thermal.minF
                        : live.thermalDisplay.fixedMinF;
  float imageMaxF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO)
                        ? live.thermal.maxF
                        : live.thermalDisplay.fixedMaxF;

  float scaleMinF = live.thermal.minF;
  float scaleMaxF = live.thermal.maxF;

  float x0, y0, cropW, cropH;
  getThermalCropWindow(live.thermalDisplay, x0, y0, cropW, cropH);

  for (int ty = 0; ty < THERMAL_H; ty++) {
    for (int tx = 0; tx < THERMAL_W; tx++) {
      float srcX = x0 + (tx + 0.5f) * (cropW / THERMAL_W);
      float srcY = y0 + (ty + 0.5f) * (cropH / THERMAL_H);
      float temp = bilinearSampleF(live.thermal.pixelsF, THERMAL_W, THERMAL_H,
                                   srcX, srcY);
      uint16_t color = thermalColor565(temp, imageMinF, imageMaxF,
                                       live.thermalDisplay.palette);
      int px = imgX + tx * cellW;
      int py = imgY + ty * cellH;
      int pw = (tx == THERMAL_W - 1) ? (imgW - cellW * tx) : cellW;
      int ph = (ty == THERMAL_H - 1) ? (imgH - cellH * ty) : cellH;
      gfx->fillRect(px, py, pw, ph, color);
    }
  }

  int hx =
      imgX +
      (int)(((live.thermal.hotspotX - x0) / max(0.001f, cropW - 1)) * imgW);
  int hy =
      imgY +
      (int)(((live.thermal.hotspotY - y0) / max(0.001f, cropH - 1)) * imgH);
  if (hx >= imgX && hx < imgX + imgW && hy >= imgY && hy < imgY + imgH) {
    gfx->drawLine(hx - 6, hy, hx + 6, hy, COLOR_RED);
    gfx->drawLine(hx, hy - 6, hx, hy + 6, COLOR_RED);
  }

  gfx->fillRect(284, 100, 130, 90, COLOR_DARK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(284, 100);
  gfx->print("Hot: ");
  gfx->print(live.thermal.hotspotF, 1);
  gfx->print(" F");

  gfx->setCursor(284, 118);
  gfx->print("Min: ");
  gfx->print(scaleMinF, 1);

  gfx->setCursor(284, 136);
  gfx->print("Max: ");
  gfx->print(scaleMaxF, 1);

  gfx->setCursor(284, 154);
  gfx->print("Center: ");
  gfx->print(live.thermal.centerF, 1);

  gfx->setCursor(284, 172);
  gfx->print("Ambient: ");
  gfx->print(live.thermal.ambientF, 1);

  const int barX = 440;
  const int barY = 90;
  const int barW = 10;
  const int barH = 150;
  for (int i = 0; i < barH; i++) {
    float f = scaleMaxF - (scaleMaxF - scaleMinF) * (i / (float)barH);
    gfx->drawFastHLine(
        barX, barY + i, barW,
        thermalColor565(f, scaleMinF, scaleMaxF, live.thermalDisplay.palette));
  }

  int hotBarY = barY + (int)(((scaleMaxF - live.thermal.hotspotF) /
                              max(0.001f, scaleMaxF - scaleMinF)) *
                             barH);
  if (hotBarY >= barY && hotBarY <= barY + barH) {
    gfx->fillTriangle(barX + barW + 8, hotBarY, barX + barW + 1, hotBarY - 4,
                      barX + barW + 1, hotBarY + 4, COLOR_RED);
    gfx->drawFastHLine(barX - 1, hotBarY, barW + 2, COLOR_RED);
  }
}

void TFTUI::drawTemperaturePageStatic() {
  gfx->setTextColor(COLOR_ORANGE);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Temp Probe");

  gfx->fillRect(16, 84, 180, 158, COLOR_DARK);
  gfx->drawRect(16, 84, 180, 158, COLOR_WHITE);

  gfx->fillRect(210, 84, 250, 180, COLOR_DARK);
  gfx->drawRect(210, 84, 250, 180, COLOR_WHITE);
}

void TFTUI::drawTemperaturePageDynamic(const LiveData &live) {
  gfx->fillRect(24, 96, 160, 66, COLOR_DARK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  if (!live.temperature.valid) {
    gfx->setCursor(24, 100);
    gfx->print("DS18B20 not ready");
    return;
  }

  gfx->setCursor(24, 100);
  gfx->print("Current: ");
  gfx->print(live.temperature.refF, 1);
  gfx->print(" F");

  gfx->setCursor(24, 118);
  gfx->print("Min:     ");
  gfx->print(live.temperature.minRefF, 1);
  gfx->print(" F");

  gfx->setCursor(24, 136);
  gfx->print("Max:     ");
  gfx->print(live.temperature.maxRefF, 1);
  gfx->print(" F");

  unsigned long now = millis();
  if (now - lastTempHistoryMs > 500) {
    lastTempHistoryMs = now;
    pushTempHistory(live.temperature.refF);
  }

  drawSingleTrendPlot(gfx, tempHistory, TEMP_HISTORY_LEN, tempHistoryFilled,
                      tempHistoryIndex, 211, 85, 248, 178, COLOR_ORANGE,
                      "Temp Trend");
}

void TFTUI::drawSoundPageStatic() {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Sound");

  gfx->fillRect(16, 84, 180, 70, COLOR_DARK);
  gfx->drawRect(16, 84, 180, 70, COLOR_WHITE);

  gfx->fillRect(16, 170, 444, 100, COLOR_DARK);
  gfx->drawRect(16, 170, 444, 100, COLOR_WHITE);
}

void TFTUI::drawSoundPageDynamic(const LiveData &live) {
  unsigned long now = millis();
  if (now - lastSoundHistoryMs > 250) {
    lastSoundHistoryMs = now;
    pushSoundHistory(live.sound.dbRel);
  }

  gfx->fillRect(24, 100, 160, 36, COLOR_DARK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(24, 100);
  gfx->print("Level: ");
  gfx->print(live.sound.dbRel, 1);
  gfx->print(" dB");

  gfx->setCursor(24, 118);
  gfx->print("Peak Hz: ");
  if (live.soundSpectrum.valid) {
    gfx->print(live.soundSpectrum.dominantHz, 1);
    gfx->print(" Hz");
  } else {
    gfx->print("warming up...");
  }

  drawSingleTrendPlot(gfx, soundHistory, SOUND_HISTORY_LEN, soundHistoryFilled,
                      soundHistoryIndex, 16, 170, 444, 100, COLOR_CYAN,
                      "Sound Level Trend");
}

void TFTUI::drawSoundFFTPageStatic() {
  gfx->setTextColor(COLOR_CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Sound FFT");

  gfx->fillRect(16, 84, 444, 180, COLOR_DARK);
  gfx->drawRect(16, 84, 444, 180, COLOR_WHITE);
}

void TFTUI::drawSoundFFTPageDynamic(const LiveData &live) {
  gfx->fillRect(16, 64, 220, 22, COLOR_BLACK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(16, 72);
  gfx->print("Dominant: ");
  if (live.soundSpectrum.valid) {
    gfx->print(live.soundSpectrum.dominantHz, 1);
    gfx->print(" Hz");
  } else {
    gfx->print("warming up...");
  }

  const int plotX = 17;
  const int plotY = 85;
  const int plotW = 442;
  const int plotH = 178;
  gfx->fillRect(plotX, plotY, plotW, plotH, COLOR_DARK);

  if (live.soundSpectrum.valid) {
    int binCount = live.soundSpectrum.bins;
    if (binCount > SoundSpectrumData::MAX_BINS)
      binCount = SoundSpectrumData::MAX_BINS;
    if (binCount <= 1)
      return;

    float maxBin = 0.001f;
    for (int i = 1; i < binCount; i++) {
      if (live.soundSpectrum.mag[i] > maxBin)
        maxBin = live.soundSpectrum.mag[i];
    }

    for (int i = 1; i < binCount; i++) {
      int x = plotX + (i * (plotW - 1)) / (binCount - 1);
      int h = (int)((live.soundSpectrum.mag[i] / maxBin) * (plotH - 4));
      if (h < 0)
        h = 0;
      if (h > plotH - 4)
        h = plotH - 4;
      gfx->drawFastVLine(x, plotY + plotH - 2 - h, h, COLOR_CYAN);
    }

    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(plotX + 6, plotY + 12);
    gfx->print("Mag max ");
    gfx->print(maxBin, 4);
    gfx->setCursor(plotX + 6, plotY + plotH - 6);
    gfx->print("0 Hz");
    gfx->setCursor(plotX + plotW - 58, plotY + plotH - 6);
    gfx->print(live.soundSpectrum.hz[binCount - 1], 0);
    gfx->print(" Hz");
  }
}

void TFTUI::drawSystemPageStatic() {
  gfx->setTextColor(COLOR_MAGENTA);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("System");

  gfx->fillRect(16, 84, 444, 180, COLOR_DARK);
  gfx->drawRect(16, 84, 444, 180, COLOR_WHITE);
}

void TFTUI::drawSystemPageDynamic(const LiveData &live) {
  gfx->fillRect(28, 102, 420, 150, COLOR_DARK);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(28, 102);
  gfx->print("REC: ");
  gfx->print(live.ui.recOn ? "ON" : "OFF");

  gfx->setCursor(28, 120);
  gfx->print("Status: ");
  gfx->print(live.system.statusText);

  gfx->setCursor(28, 138);
  gfx->print("Thermal: ");
  gfx->print(live.thermal.valid ? "READY" : "NOT READY");

  gfx->setCursor(28, 156);
  gfx->print("Sound FFT: ");
  gfx->print(live.soundSpectrum.valid ? "READY" : "NOT READY");

  gfx->setCursor(28, 174);
  gfx->print("Vib FFT: ");
  gfx->print(live.vibrationSpectrum.valid ? "READY" : "NOT READY");

  gfx->setCursor(28, 192);
  gfx->print("Mount: ");
  gfx->print(live.mount.attached ? "ATTACHED" : "NOT ATTACHED");

  gfx->setCursor(28, 210);
  gfx->print("Sound Peak: ");
  gfx->print(live.soundSpectrum.valid ? live.soundSpectrum.dominantHz : 0.0f,
             1);
  gfx->print(" Hz");

  gfx->setCursor(28, 228);
  gfx->print("Thermal Hot: ");
  if (live.thermal.valid) {
    gfx->print(live.thermal.hotspotF, 1);
    gfx->print(" F");
  } else {
    gfx->print("--");
  }

  gfx->setCursor(28, 246);
  gfx->print("Vib Total: ");
  gfx->print(live.vibration.vt, 3);
}

void TFTUI::draw(const LiveData &live) {
  unsigned long now = millis();
  bool fullRedraw = false;

  if (live.ui.currentPage != lastPage || live.ui.recOn != lastRec) {
    fullRedraw = true;
    lastPage = live.ui.currentPage;
    lastRec = live.ui.recOn;
  }

  if (fullRedraw) {
    gfx->fillScreen(COLOR_BLACK);
    drawHeader(live);

    switch (live.ui.currentPage) {
    case PAGE_VIBRATION:
      drawVibrationPageStatic();
      break;
    case PAGE_VIBRATION_FFT:
      drawVibrationFFTPageStatic();
      break;
    case PAGE_THERMAL:
      drawThermalPageStatic();
      break;
    case PAGE_TEMPERATURE:
      drawTemperaturePageStatic();
      break;
    case PAGE_SOUND:
      drawSoundPageStatic();
      break;
    case PAGE_SOUND_FFT:
      drawSoundFFTPageStatic();
      break;
    case PAGE_SYSTEM:
      drawSystemPageStatic();
      break;
    default:
      drawHomePageStatic();
      break;
    }

    drawFooter(live);
  }

  if (fullRedraw || (now - lastDynamicRefresh > 150)) {
    lastDynamicRefresh = now;

    switch (live.ui.currentPage) {
    case PAGE_VIBRATION:
      drawVibrationPageDynamic(live);
      break;
    case PAGE_VIBRATION_FFT:
      drawVibrationFFTPageDynamic(live);
      break;
    case PAGE_THERMAL:
      drawThermalPageDynamic(live);
      break;
    case PAGE_TEMPERATURE:
      drawTemperaturePageDynamic(live);
      break;
    case PAGE_SOUND:
      drawSoundPageDynamic(live);
      break;
    case PAGE_SOUND_FFT:
      drawSoundFFTPageDynamic(live);
      break;
    case PAGE_SYSTEM:
      drawSystemPageDynamic(live);
      break;
    default:
      drawHomePageDynamic(live);
      break;
    }

    drawFooter(live);
  }
}
