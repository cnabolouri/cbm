#pragma once
#include <Arduino.h>

#if __has_include(<Arduino_GFX_Library.h>)
#include <Arduino_GFX_Library.h>
#elif __has_include(                                                           \
    "../../libraries/GFX_Library_for_Arduino/src/Arduino_GFX_Library.h")
#include "../../libraries/GFX_Library_for_Arduino/src/Arduino_GFX_Library.h"
#else
#error                                                                         \
    "Arduino_GFX_Library.h not found. Install Arduino_GFX / GFX_Library_for_Arduino or add it to your include path."
#endif

#include "../config.h"
#include "../types.h"

class TFTUI {
public:
  bool begin();
  void draw(const LiveData &live);

private:
  Arduino_DataBus *bus = nullptr;
  Arduino_GFX *gfx = nullptr;

  int lastPage = -1;
  bool lastRec = false;
  unsigned long lastDynamicRefresh = 0;

  void drawHeader(const LiveData &live);
  void drawFooter(const LiveData &live);

  void drawHomePageStatic();
  void drawHomePageDynamic(const LiveData &live);

  void drawVibrationPageStatic();
  void drawVibrationPageDynamic(const LiveData &live);
  void drawVibrationFFTPageStatic();
  void drawVibrationFFTPageDynamic(const LiveData &live);

  void drawThermalPageStatic();
  void drawThermalPageDynamic(const LiveData &live);

  void drawTemperaturePageStatic();
  void drawTemperaturePageDynamic(const LiveData &live);

  void drawSoundPageStatic();
  void drawSoundPageDynamic(const LiveData &live);
  void drawSoundFFTPageStatic();
  void drawSoundFFTPageDynamic(const LiveData &live);

  void drawSystemPageStatic();
  void drawSystemPageDynamic(const LiveData &live);

  uint16_t thermalColor565(float tempF, float minF, float maxF,
                           ThermalPalette palette) const;
};
