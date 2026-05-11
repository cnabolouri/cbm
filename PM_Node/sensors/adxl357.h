#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "../types.h"

class ADXL357Sensor {
public:
  ADXL357Sensor();

  bool begin();
  void update(float dt, VibrationData& out);
  float getLastRawXg() const;
  float getLastRawYg() const;
  float getLastRawZg() const;

private:
  uint8_t readReg(uint8_t reg);
  void writeReg(uint8_t reg, uint8_t value);
  void readRegs(uint8_t startReg, uint8_t *buf, size_t len);
  bool readXYZRaw(int32_t &x, int32_t &y, int32_t &z);
  int32_t convert20Bit(uint8_t b3, uint8_t b2, uint8_t b1);

  SPIClass spi;
  bool initialized = false;

  float vx = 0, vy = 0, vz = 0;
  float axBase = 0, ayBase = 0, azBase = 0;
  bool accelInitialized = false;
  unsigned long bootMs = 0;
  float lastRawXg = 0.0f;
  float lastRawYg = 0.0f;
  float lastRawZg = 0.0f;
};
