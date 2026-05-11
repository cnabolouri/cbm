#include "temperature.h"
#include "../config.h"

TemperatureSensor::TemperatureSensor()
  : i2c(0), mlx(), oneWire(PIN_DS18B20), ds(&oneWire) {}

bool TemperatureSensor::begin() {
  i2c.begin(I2C_SDA, I2C_SCL, 100000);

  ds.begin();
  ds.setResolution(10);
  ds.setWaitForConversion(false);
  ds.requestTemperatures();
  lastRequestMs = millis();

  bool mlxOk = mlx.begin(0x5A, &i2c);

  lastRefF = 0.0f;
  lastObjF = 0.0f;
  lastAmbF = 0.0f;
  lastDeltaF = 0.0f;

  return mlxOk;
}

void TemperatureSensor::update(TemperatureData& out) {
  unsigned long now = millis();

  float ambC = mlx.readAmbientTempC();
  float objC = mlx.readObjectTempC();

  lastAmbF = ambC * 9.0f / 5.0f + 32.0f;
  lastObjF = objC * 9.0f / 5.0f + 32.0f;

  if (now - lastRequestMs >= DS_INTERVAL_MS) {
    float refC = ds.getTempCByIndex(0);
    if (refC != DEVICE_DISCONNECTED_C) {
      lastRefF = refC * 9.0f / 5.0f + 32.0f;
      dsReady = true;
    }

    ds.requestTemperatures();
    lastRequestMs = now;
  }

  lastDeltaF = dsReady ? (lastObjF - lastRefF) : 0.0f;

  out.objF = lastObjF;
  out.refF = dsReady ? lastRefF : 0.0f;
  out.ambF = lastAmbF;
  out.deltaF = lastDeltaF;
}
