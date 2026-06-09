#pragma once
#include <Arduino.h>

#if __has_include(<Arduino_GFX_Library.h>)
#include <Arduino_GFX_Library.h>
#elif __has_include("../../libraries/GFX_Library_for_Arduino/src/Arduino_GFX_Library.h")
#include "../../libraries/GFX_Library_for_Arduino/src/Arduino_GFX_Library.h"
#else
#error "Arduino_GFX_Library.h not found. Install Arduino_GFX / GFX_Library_for_Arduino or add it to your include path."
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
  bool pageNeedsFullRedraw = true;
  unsigned long lastDynamicDrawMs = 0;
  int lastPointerX = -1;
  int lastPointerY = -1;
  bool lastRecOn = false;

  void drawHeader(const LiveData& live);
  void drawFooter(const LiveData& live);
  void drawStaticPage(const LiveData& live);
  void drawDynamicElements(const LiveData& live);
  void drawHomePage(const LiveData& live);
  void drawVibrationPage(const LiveData& live);
  void drawTemperaturePage(const LiveData& live);
  void drawThermalPage(const LiveData& live);
  void drawSoundPage(const LiveData& live);
  void drawSystemPage(const LiveData& live);
  void drawCrosshair(int x, int y, uint16_t color);
  uint16_t thermalColor565(float tempF, float minF, float maxF, ThermalPalette palette) const;
};
