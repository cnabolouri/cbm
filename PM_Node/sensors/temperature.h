#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MLX90614.h>
#include "../types.h"

class TemperatureSensor {
public:
  TemperatureSensor();
  bool begin();
  void update(TemperatureData& out);

private:
  TwoWire i2c;
  Adafruit_MLX90614 mlx;
  OneWire oneWire;
  DallasTemperature ds;

  bool dsReady = false;
  unsigned long lastRequestMs = 0;
  static const unsigned long DS_INTERVAL_MS = 1000;

  float lastRefF = 0.0f;
  float lastObjF = 0.0f;
  float lastAmbF = 0.0f;
  float lastDeltaF = 0.0f;
};
