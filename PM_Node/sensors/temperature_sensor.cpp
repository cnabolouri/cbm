#include "temperature_sensor.h"

bool TemperatureSensor::begin() {
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  requested = true;
  lastRequestMs = millis();
  return sensors.getDeviceCount() > 0;
}

bool TemperatureSensor::update(TemperatureData &out) {
  unsigned long now = millis();

  if (requested && (now - lastRequestMs >= 750)) {
    float c = sensors.getTempCByIndex(0);
    if (c > -100.0f && c < 150.0f) {
      out.refF = cToF(c);
      if (!out.valid) {
        out.minRefF = out.maxRefF = out.refF;
      } else {
        out.minRefF = min(out.minRefF, out.refF);
        out.maxRefF = max(out.maxRefF, out.refF);
      }
      out.valid = true;
    } else {
      out.valid = false;
    }
    requested = false;
  }

  if (!requested && (now - lastRequestMs >= 1000)) {
    sensors.requestTemperatures();
    lastRequestMs = now;
    requested = true;
  }

  out.deltaF = out.objF - out.refF;
  return !requested && out.valid;
}
