#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../types.h"
#include "../config.h"

class TemperatureSensor {
public:
  bool begin();
  bool update(TemperatureData& out);

private:
  OneWire oneWire = OneWire(PIN_DS18B20);
  DallasTemperature sensors = DallasTemperature(&oneWire);

  unsigned long lastRequestMs = 0;
  bool requested = false;

  float cToF(float c) const {
    return c * 9.0f / 5.0f + 32.0f;
  }
};
