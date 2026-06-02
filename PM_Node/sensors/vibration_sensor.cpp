#include "vibration_sensor.h"
#include <math.h>

static const float IIS3DWB_MG_PER_LSB = 0.061f;
static const float IIS3DWB_G_PER_LSB = IIS3DWB_MG_PER_LSB / 1000.0f;
static const float BIAS_ALPHA = 0.05f;
static const float DYNAMIC_ALPHA = 0.08f;

uint8_t VibrationSensor::readReg(uint8_t reg) {
  digitalWrite(IIS_CS, LOW);
  spi.transfer(reg | 0x80);
  uint8_t value = spi.transfer(0x00);
  digitalWrite(IIS_CS, HIGH);
  return value;
}

void VibrationSensor::readRegs(uint8_t reg, uint8_t* buf, size_t len) {
  digitalWrite(IIS_CS, LOW);
  spi.transfer(reg | 0x80);
  for (size_t i = 0; i < len; i++) {
    buf[i] = spi.transfer(0x00);
  }
  digitalWrite(IIS_CS, HIGH);
}

void VibrationSensor::writeReg(uint8_t reg, uint8_t val) {
  digitalWrite(IIS_CS, LOW);
  spi.transfer(reg & 0x7F);
  spi.transfer(val);
  digitalWrite(IIS_CS, HIGH);
}

bool VibrationSensor::begin() {
  pinMode(IIS_CS, OUTPUT);
  digitalWrite(IIS_CS, HIGH);

  spi.begin(IIS_SCK, IIS_MISO, IIS_MOSI, IIS_CS);
  delay(10);

  uint8_t who = readReg(REG_WHO_AM_I);
  if (who != WHOAMI) {
    return false;
  }

  writeReg(REG_CTRL3_C, 0x44);
  delay(10);
  writeReg(REG_CTRL1_XL, 0xA0);
  delay(10);

  haveBias = false;
  biasXg = 0.0f;
  biasYg = 0.0f;
  biasZg = 0.0f;
  sx = 0.0f;
  sy = 0.0f;
  sz = 0.0f;
  st = 0.0f;
  statsCount = 0;
  sumTotal = 0.0f;
  maxTotal = 0.0f;
  lastRawXg = 0.0f;
  lastRawYg = 0.0f;
  lastRawZg = 0.0f;

  return true;
}

void VibrationSensor::resetStats() {
  statsCount = 0;
  sumTotal = 0.0f;
  maxTotal = 0.0f;
}

bool VibrationSensor::update(VibrationData& out) {
  uint8_t raw[6] = {0};
  readRegs(REG_OUTX_L_A, raw, 6);

  int16_t rawX = (int16_t)((raw[1] << 8) | raw[0]);
  int16_t rawY = (int16_t)((raw[3] << 8) | raw[2]);
  int16_t rawZ = (int16_t)((raw[5] << 8) | raw[4]);

  float xg = rawX * IIS3DWB_G_PER_LSB;
  float yg = rawY * IIS3DWB_G_PER_LSB;
  float zg = rawZ * IIS3DWB_G_PER_LSB;

  if (!haveBias) {
    biasXg = xg;
    biasYg = yg;
    biasZg = zg;
    haveBias = true;
  } else {
    biasXg = biasXg * (1.0f - BIAS_ALPHA) + xg * BIAS_ALPHA;
    biasYg = biasYg * (1.0f - BIAS_ALPHA) + yg * BIAS_ALPHA;
    biasZg = biasZg * (1.0f - BIAS_ALPHA) + zg * BIAS_ALPHA;
  }

  float dxg = xg - biasXg;
  float dyg = yg - biasYg;
  float dzg = zg - biasZg;

  if (fabsf(dxg) > 4.0f) dxg = 0.0f;
  if (fabsf(dyg) > 4.0f) dyg = 0.0f;
  if (fabsf(dzg) > 4.0f) dzg = 0.0f;

  float vxRaw = fabsf(dxg);
  float vyRaw = fabsf(dyg);
  float vzRaw = fabsf(dzg);
  float vtRaw = sqrtf(vxRaw * vxRaw + vyRaw * vyRaw + vzRaw * vzRaw);

  sx = sx * (1.0f - DYNAMIC_ALPHA) + vxRaw * DYNAMIC_ALPHA;
  sy = sy * (1.0f - DYNAMIC_ALPHA) + vyRaw * DYNAMIC_ALPHA;
  sz = sz * (1.0f - DYNAMIC_ALPHA) + vzRaw * DYNAMIC_ALPHA;
  st = st * (1.0f - DYNAMIC_ALPHA) + vtRaw * DYNAMIC_ALPHA;

  if (sx < 0.001f) sx = 0.0f;
  if (sy < 0.001f) sy = 0.0f;
  if (sz < 0.001f) sz = 0.0f;
  if (st < 0.001f) st = 0.0f;

  out.vx = sx;
  out.vy = sy;
  out.vz = sz;
  out.vt = st;

  out.ax_g = dxg;
  out.ay_g = dyg;
  out.az_g = dzg;

  out.x_in_s = out.vx;
  out.y_in_s = out.vy;
  out.z_in_s = out.vz;
  out.total_in_s = out.vt;

  statsCount++;
  sumTotal += st;
  if (st > maxTotal) {
    maxTotal = st;
  }

  out.maxTotal = maxTotal;
  out.avgTotal = (statsCount > 0) ? (sumTotal / statsCount) : 0.0f;

  lastRawXg = xg;
  lastRawYg = yg;
  lastRawZg = zg;

  return true;
}

bool VibrationSensor::update(VibrationData& out, VibrationSpectrumData& spectrumOut) {
  (void)spectrumOut;
  return update(out);
}

float VibrationSensor::getLastRawXg() const {
  return lastRawXg;
}

float VibrationSensor::getLastRawYg() const {
  return lastRawYg;
}

float VibrationSensor::getLastRawZg() const {
  return lastRawZg;
}
