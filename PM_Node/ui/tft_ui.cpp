#include "tft_ui.h"

#define COLOR_BLACK 0x0000
#define COLOR_BLUE 0x001F
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_CYAN 0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE 0xFFFF
#define COLOR_GRAY 0x8410

static float clampFloatTFT(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t TFTUI::thermalColor565(float tempF, float minF, float maxF,
                                ThermalPalette palette) const {
  float t = (tempF - minF) / ((maxF - minF) > 0.001f ? (maxF - minF) : 0.001f);
  t = clampFloatTFT(t, 0.0f, 1.0f);

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

void TFTUI::draw(const LiveData &live) {
  if (!gfx)
    return;

  unsigned long now = millis();

  if (live.ui.currentPage != lastPage || live.ui.recOn != lastRecOn) {
    pageNeedsFullRedraw = true;
  }

  if (pageNeedsFullRedraw) {
    gfx->fillScreen(COLOR_BLACK);
    drawHeader(live);
    drawStaticPage(live);
    drawFooter(live);
    lastPage = live.ui.currentPage;
    lastRecOn = live.ui.recOn;
    lastPointerX = -1;
    lastPointerY = -1;
    pageNeedsFullRedraw = false;
  }

  if (lastDynamicDrawMs == 0 || now - lastDynamicDrawMs >= 50) {
    lastDynamicDrawMs = now;
    drawHeader(live);
    drawDynamicElements(live);
  }
}

void TFTUI::drawStaticPage(const LiveData &live) {
  switch (live.ui.currentPage) {
    case PAGE_HOME:
      drawHomePage(live);
      break;
    case PAGE_VIBRATION:
      drawVibrationPage(live);
      break;
    case PAGE_THERMAL:
      drawThermalPage(live);
      break;
    case PAGE_SOUND:
      drawSoundPage(live);
      break;
    case PAGE_SYSTEM:
      drawSystemPage(live);
      break;
    default:
      drawHomePage(live);
      break;
  }
}

void TFTUI::drawDynamicElements(const LiveData &live) {
  gfx->fillRect(0, 34, TFT_W, TFT_H - 76, COLOR_BLACK);
  drawStaticPage(live);
  drawFooter(live);

  drawCrosshair(live.ui.pointer.xi, live.ui.pointer.yi, COLOR_CYAN);
  lastPointerX = live.ui.pointer.xi;
  lastPointerY = live.ui.pointer.yi;
}

void TFTUI::drawHeader(const LiveData &live) {
  const char *labels[PAGE_COUNT] = {"HOME", "VIBRATION", "THERMAL", "SOUND",
                                    "SYSTEM"};
  int pageIndex = live.ui.currentPage;
  if (pageIndex < 0)
    pageIndex = 0;
  if (pageIndex >= PAGE_COUNT)
    pageIndex = PAGE_COUNT - 1;
  const char *title = labels[pageIndex];

  gfx->fillRect(0, 0, TFT_W, 34, COLOR_GRAY);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(8, 8);
  gfx->print(title);

  gfx->fillRect(300, 0, 180, 34, COLOR_GRAY);
  gfx->setCursor(320, 8);
  gfx->setTextColor(live.ui.recOn ? COLOR_RED : COLOR_WHITE);
  if (live.ui.recOn) {
    gfx->print("REC ON");
  } else {
    gfx->print("REC OFF");
  }
}

void TFTUI::drawFooter(const LiveData &live) {
  gfx->fillRect(0, TFT_H - 42, TFT_W, 42, COLOR_GRAY);
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(1);

  gfx->setCursor(8, TFT_H - 34);
  gfx->printf("X:%4d Y:%4d SW:%s", live.ui.joyRawX, live.ui.joyRawY,
              live.ui.joyButtonPressed ? "PRESSED" : "RELEASED");

  gfx->setCursor(8, TFT_H - 18);
  gfx->printf("Cur:(%d,%d) Norm:(%.2f,%.2f)", live.ui.pointer.xi,
              live.ui.pointer.yi, live.ui.joyNormX, live.ui.joyNormY);
}

void TFTUI::drawHomePage(const LiveData &live) {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(16, 60);
  gfx->print("PM NODE");

  gfx->setTextSize(1);
  gfx->setCursor(16, 110);
  gfx->print("Short press: next page");
  gfx->setCursor(16, 126);
  gfx->print("Long press : record toggle");
  gfx->setCursor(16, 142);
  gfx->print("Joystick   : move pointer");

}

void TFTUI::drawVibrationPage(const LiveData &live) {
  gfx->setTextColor(COLOR_CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Vibration");

  gfx->setTextColor(COLOR_WHITE);

  gfx->setCursor(16, 96);
  gfx->print("VX: ");
  gfx->print(live.vibration.vx, 3);
  gfx->print(" g-dyn");

  gfx->setCursor(16, 124);
  gfx->print("VY: ");
  gfx->print(live.vibration.vy, 3);
  gfx->print(" g-dyn");

  gfx->setCursor(16, 152);
  gfx->print("VZ: ");
  gfx->print(live.vibration.vz, 3);
  gfx->print(" g-dyn");

  gfx->setCursor(16, 180);
  gfx->print("VT: ");
  gfx->print(live.vibration.vt, 3);
  gfx->print(" g-dyn");

  gfx->setCursor(16, 216);
  gfx->print("Max Total: ");
  gfx->print(live.vibration.maxTotal, 3);

  gfx->setCursor(16, 242);
  gfx->print("Avg Total: ");
  gfx->print(live.vibration.avgTotal, 3);

  gfx->setCursor(260, 56);
  gfx->print("FFT: ");
  if (live.vibrationSpectrum.valid) {
    gfx->print(live.vibrationSpectrum.dominantHz, 1);
    gfx->print(" Hz");
  } else {
    gfx->print("warming up...");
  }

  const int plotX = 260;
  const int plotY = 80;
  const int plotW = 200;
  const int plotH = 150;

  gfx->drawRect(plotX, plotY, plotW, plotH, COLOR_WHITE);

  if (live.vibrationSpectrum.valid) {
    float maxBin = 0.001f;
    for (int i = 1; i < live.vibrationSpectrum.bins; i++) {
      if (live.vibrationSpectrum.mag[i] > maxBin)
        maxBin = live.vibrationSpectrum.mag[i];
    }

    for (int i = 1; i < live.vibrationSpectrum.bins; i++) {
      int x = plotX + (i * plotW) / live.vibrationSpectrum.bins;
      int h = (int)((live.vibrationSpectrum.mag[i] / maxBin) * (plotH - 2));
      if (h < 0)
        h = 0;
      if (h > plotH - 2)
        h = plotH - 2;
      gfx->drawFastVLine(x, plotY + plotH - 1 - h, h, COLOR_YELLOW);
    }

    int px = live.ui.pointer.xi;
    int py = live.ui.pointer.yi;
    if (px >= plotX && px < plotX + plotW && py >= plotY && py < plotY + plotH) {
      int bin = ((px - plotX) * live.vibrationSpectrum.bins) / plotW;
      if (bin < 1) bin = 1;
      if (bin >= live.vibrationSpectrum.bins) bin = live.vibrationSpectrum.bins - 1;

      gfx->drawFastVLine(px, plotY + 1, plotH - 2, COLOR_CYAN);
      gfx->fillRect(plotX, plotY + plotH + 6, plotW, 18, COLOR_BLACK);
      gfx->setTextColor(COLOR_CYAN);
      gfx->setTextSize(1);
      gfx->setCursor(plotX, plotY + plotH + 8);
      gfx->print("Ptr ");
      gfx->print(live.vibrationSpectrum.hz[bin], 1);
      gfx->print(" Hz  mag ");
      gfx->print(live.vibrationSpectrum.mag[bin], 4);
    }
  }

  gfx->setTextSize(1);
  gfx->setCursor(16, 274);
  gfx->print("AX: ");
  gfx->print(live.vibration.ax_g, 3);
  gfx->print(" g");

  gfx->setCursor(160, 274);
  gfx->print("AY: ");
  gfx->print(live.vibration.ay_g, 3);
  gfx->print(" g");

  gfx->setCursor(304, 274);
  gfx->print("AZ: ");
  gfx->print(live.vibration.az_g, 3);
  gfx->print(" g");

}

void TFTUI::drawTemperaturePage(const LiveData &live) {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(12, 48);
  gfx->print("Object: ");
  gfx->print(live.temperature.objF, 1);
  gfx->print(" F");

  gfx->setCursor(12, 90);
  gfx->print("Ref:    ");
  gfx->print(live.temperature.refF, 1);
  gfx->print(" F");

  gfx->setCursor(12, 132);
  gfx->print("Ambient:");
  gfx->print(live.temperature.ambF, 1);
  gfx->print(" F");

  gfx->setCursor(12, 174);
  gfx->print("Delta:  ");
  gfx->print(live.temperature.deltaF, 1);
  gfx->print(" F");
}

void TFTUI::drawThermalPage(const LiveData &live) {
  const int imgX = 10;
  const int imgY = 40;
  const int imgW = 320;
  const int imgH = 216;
  const int cellW = imgW / THERMAL_W;
  const int cellH = imgH / THERMAL_H;

  gfx->fillRect(imgX - 1, imgY - 1, imgW + 2, imgH + 2, COLOR_WHITE);
  if (!live.thermal.valid) {
    gfx->setTextColor(COLOR_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(imgX + 20, imgY + 90);
    gfx->print("Thermal data");
    gfx->setCursor(imgX + 20, imgY + 120);
    gfx->print("not available");
    return;
  }

  const float minF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_FIXED)
                         ? live.thermalDisplay.fixedMinF
                         : live.thermal.minF;
  const float maxF = (live.thermalDisplay.rangeMode == THERMAL_RANGE_FIXED)
                         ? live.thermalDisplay.fixedMaxF
                         : live.thermal.maxF;

  for (int y = 0; y < THERMAL_H; y++) {
    for (int x = 0; x < THERMAL_W; x++) {
      float temp = live.thermal.pixelsF[y * THERMAL_W + x];
      uint16_t color =
          thermalColor565(temp, minF, maxF, live.thermalDisplay.palette);
      int px = imgX + x * cellW;
      int py = imgY + y * cellH;
      int w = (x == THERMAL_W - 1) ? (imgW - cellW * x) : cellW;
      int h = (y == THERMAL_H - 1) ? (imgH - cellH * y) : cellH;
      gfx->fillRect(px, py, w, h, color);
    }
  }

  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  int statsX = imgX + imgW + 12;
  gfx->setCursor(statsX, imgY);
  gfx->print("Hotspot:");
  gfx->setCursor(statsX, imgY + 28);
  gfx->print(live.thermal.hotspotF, 1);
  gfx->print(" F");

  gfx->setCursor(statsX, imgY + 60);
  gfx->print("Center:");
  gfx->setCursor(statsX, imgY + 88);
  gfx->print(live.thermal.centerF, 1);
  gfx->print(" F");

  gfx->setCursor(statsX, imgY + 120);
  gfx->print("Min:");
  gfx->setCursor(statsX, imgY + 148);
  gfx->print(minF, 1);
  gfx->print(" F");

  gfx->setCursor(statsX, imgY + 180);
  gfx->print("Max:");
  gfx->setCursor(statsX, imgY + 208);
  gfx->print(maxF, 1);
  gfx->print(" F");

  int px = live.ui.pointer.xi;
  int py = live.ui.pointer.yi;
  if (px >= imgX && px < imgX + imgW && py >= imgY && py < imgY + imgH) {
    int thermalX = map(px, imgX, imgX + imgW - 1, 0, THERMAL_W - 1);
    int thermalY = map(py, imgY, imgY + imgH - 1, 0, THERMAL_H - 1);
    if (thermalX < 0) thermalX = 0;
    if (thermalX >= THERMAL_W) thermalX = THERMAL_W - 1;
    if (thermalY < 0) thermalY = 0;
    if (thermalY >= THERMAL_H) thermalY = THERMAL_H - 1;
    float tempF = live.thermal.pixelsF[thermalY * THERMAL_W + thermalX];

    gfx->fillRect(imgX, imgY + imgH + 4, imgW, 18, COLOR_BLACK);
    gfx->setTextColor(COLOR_CYAN);
    gfx->setTextSize(1);
    gfx->setCursor(imgX, imgY + imgH + 7);
    gfx->print("Pointer ");
    gfx->print(thermalX);
    gfx->print(",");
    gfx->print(thermalY);
    gfx->print("  ");
    gfx->print(tempF, 1);
    gfx->print(" F");
  }
}

void TFTUI::drawSoundPage(const LiveData &live) {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(16, 56);
  gfx->println("Sound");
  gfx->setCursor(360, 56);
  gfx->print("SNDv2");

  gfx->setTextSize(1);
  gfx->setCursor(16, 96);
  gfx->print("Level: ");
  gfx->print(live.sound.dbRel, 1);
  gfx->print(" dB rel");

  gfx->setCursor(16, 116);
  gfx->print("Peak: ");
  if (live.soundSpectrum.valid) {
    gfx->print(live.soundSpectrum.dominantHz, 1);
    gfx->print(" Hz");
  } else {
    gfx->print("warming up...");
  }

  gfx->setCursor(16, 132);
  gfx->print("FFT: ");
  gfx->print(live.soundSpectrum.valid ? "active" : "warming up");

  const int plotX = 20;
  const int plotY = 150;
  const int plotW = 440;
  const int plotH = 120;

  gfx->drawRect(plotX, plotY, plotW, plotH, COLOR_WHITE);

  if (live.soundSpectrum.valid) {
    int binCount = live.soundSpectrum.bins;
    if (binCount > SoundSpectrumData::BINS)
      binCount = SoundSpectrumData::BINS;

    float maxBin = 0.001f;
    for (int i = 1; i < binCount; i++) {
      if (live.soundSpectrum.mag[i] > maxBin)
        maxBin = live.soundSpectrum.mag[i];
    }

    for (int i = 1; i < binCount; i++) {
      int x = plotX + (i * plotW) / binCount;
      int h = (int)((live.soundSpectrum.mag[i] / maxBin) * (plotH - 2));
      if (h < 0)
        h = 0;
      if (h > plotH - 2)
        h = plotH - 2;
      gfx->drawFastVLine(x, plotY + plotH - 1 - h, h, COLOR_CYAN);
    }

    int px = live.ui.pointer.xi;
    int py = live.ui.pointer.yi;
    if (px >= plotX && px < plotX + plotW && py >= plotY && py < plotY + plotH) {
      int bin = ((px - plotX) * binCount) / plotW;
      if (bin < 1) bin = 1;
      if (bin >= binCount) bin = binCount - 1;

      gfx->drawFastVLine(px, plotY + 1, plotH - 2, COLOR_YELLOW);
      gfx->fillRect(180, 96, 290, 42, COLOR_BLACK);
      gfx->setTextColor(COLOR_YELLOW);
      gfx->setTextSize(1);
      gfx->setCursor(180, 98);
      gfx->print("Ptr ");
      gfx->print(live.soundSpectrum.hz[bin], 1);
      gfx->print(" Hz");
      gfx->setCursor(180, 114);
      gfx->print("mag ");
      gfx->print(live.soundSpectrum.mag[bin], 4);
    }
  }

}

void TFTUI::drawSystemPage(const LiveData &live) {
  gfx->setTextColor(COLOR_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(12, 48);
  gfx->print("SD: ");
  gfx->print(live.system.sdOK ? "OK" : "FAIL");

  gfx->setCursor(12, 90);
  gfx->print("Rec: ");
  gfx->print(live.system.recording ? "ON" : "OFF");

  gfx->setCursor(12, 132);
  gfx->print("Mode: ");
  gfx->print(live.system.recordingMode == REC_ACTIVE                ? "ACTIVE"
             : live.system.recordingMode == REC_ARMED_WAITING_MOUNT ? "ARMED"
                                                                    : "IDLE");

  gfx->setTextSize(1);
  gfx->setCursor(12, 180);
  gfx->print(live.system.statusText);
}

void TFTUI::drawCrosshair(int x, int y, uint16_t color) {
  gfx->drawLine(x - 8, y, x + 8, y, color);
  gfx->drawLine(x, y - 8, x, y + 8, color);
  gfx->drawCircle(x, y, 10, color);
}
