#pragma once
#include <Arduino.h>
#include <Arduino_GFX.h>

#if __has_include(<Arduino_ESP32SPI.h>)
#include <Arduino_ESP32SPI.h>
#elif __has_include("../../libraries/GFX_Library_for_Arduino/src/databus/Arduino_ESP32SPI.h")
#include "../../libraries/GFX_Library_for_Arduino/src/databus/Arduino_ESP32SPI.h"
#else
#error "Arduino_ESP32SPI.h not found. Install Arduino_GFX / GFX_Library_for_Arduino or add it to your include path."
#endif

#if __has_include(<Arduino_ST7796.h>)
#include <Arduino_ST7796.h>
#elif __has_include("../../libraries/GFX_Library_for_Arduino/src/display/Arduino_ST7796.h")
#include "../../libraries/GFX_Library_for_Arduino/src/display/Arduino_ST7796.h"
#else
#error "Arduino_ST7796.h not found. Install Arduino_GFX / GFX_Library_for_Arduino or add it to your include path."
#endif

#include "../types.h"
#include "../config.h"

class TFTUI {
public:
  bool begin();
  void draw(const LiveData& live);

private:
  Arduino_DataBus *bus = nullptr;
  Arduino_GFX *gfx = nullptr;

  int lastPage = -1;

  void drawHeader(const LiveData& live);
  void drawFooter(const LiveData& live);
  void drawHomePage(const LiveData& live);
  void drawVibrationPage(const LiveData& live);
  void drawTemperaturePage(const LiveData& live);
  void drawThermalPage(const LiveData& live);
  void drawSoundPage(const LiveData& live);
  void drawSystemPage(const LiveData& live);
  void drawCrosshair(int x, int y, uint16_t color);
  uint16_t thermalColor565(float tempF, float minF, float maxF, ThermalPalette palette) const;
};
