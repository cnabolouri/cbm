#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "../types.h"
#include "../config.h"

class VibrationSensor {
public:
  bool begin();
  bool update(VibrationData& out);
  bool update(VibrationData& out, VibrationSpectrumData& spectrumOut);
  void resetStats();

  float getLastRawXg() const;
  float getLastRawYg() const;
  float getLastRawZg() const;

private:
  SPIClass spi = SPIClass(FSPI);

  static const uint8_t REG_WHO_AM_I = 0x0F;
  static const uint8_t REG_CTRL1_XL = 0x10;
  static const uint8_t REG_CTRL3_C  = 0x12;
  static const uint8_t REG_OUTX_L_A = 0x28;
  static const uint8_t WHOAMI = 0x7B;

  uint8_t readReg(uint8_t reg);
  void readRegs(uint8_t reg, uint8_t* buf, size_t len);
  void writeReg(uint8_t reg, uint8_t val);

  bool haveBias = false;
  float biasXg = 0.0f;
  float biasYg = 0.0f;
  float biasZg = 0.0f;

  float sx = 0.0f;
  float sy = 0.0f;
  float sz = 0.0f;
  float st = 0.0f;

  unsigned long statsCount = 0;
  float sumTotal = 0.0f;
  float maxTotal = 0.0f;

  float lastRawXg = 0.0f;
  float lastRawYg = 0.0f;
  float lastRawZg = 0.0f;
};
