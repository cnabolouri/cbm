#include "temperature.h"
#include "../config.h"

TemperatureSensor::TemperatureSensor()
  : oneWire(PIN_DS18B20), ds(&oneWire) {}

bool TemperatureSensor::begin() {
  ds.begin();
  ds.setResolution(10);
  ds.setWaitForConversion(false);
  ds.requestTemperatures();
  lastRequestMs = millis();

  lastRefF = 0.0f;
  lastObjF = 0.0f;
  lastAmbF = 0.0f;
  lastDeltaF = 0.0f;

  return ds.getDeviceCount() > 0;
}

void TemperatureSensor::update(TemperatureData& out) {
  unsigned long now = millis();

  if (now - lastRequestMs >= DS_INTERVAL_MS) {
    float refC = ds.getTempCByIndex(0);
    if (refC != DEVICE_DISCONNECTED_C) {
      lastRefF = refC * 9.0f / 5.0f + 32.0f;
      dsReady = true;
    }

    ds.requestTemperatures();
    lastRequestMs = now;
  }

  out.objF = lastObjF;
  out.refF = dsReady ? lastRefF : 0.0f;
  out.ambF = lastAmbF;
  out.deltaF = lastDeltaF;
}
