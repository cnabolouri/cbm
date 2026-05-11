#include "adxl357.h"
#include "../config.h"

static const uint8_t REG_DEVID_AD   = 0x00;
static const uint8_t REG_DEVID_MST  = 0x01;
static const uint8_t REG_PARTID     = 0x02;
static const uint8_t REG_REVID      = 0x03;
static const uint8_t REG_XDATA3     = 0x08;
static const uint8_t REG_RANGE      = 0x2C;
static const uint8_t REG_POWER_CTL  = 0x2D;
static const uint8_t REG_RESET      = 0x2F;

static const float BASELINE_ALPHA = 0.999f;
static const float ACC_DEADBAND = 0.12f;
static const float VEL_DECAY = 0.82f;
static const unsigned long WARMUP_MS = 5000;

ADXL357Sensor::ADXL357Sensor() : spi(HSPI) {}

bool ADXL357Sensor::begin() {
  pinMode(ADXL_CS, OUTPUT);
  digitalWrite(ADXL_CS, HIGH);
  spi.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);

  writeReg(REG_RESET, 0x52);
  delay(10);

  uint8_t devid_ad  = readReg(REG_DEVID_AD);
  uint8_t devid_mst = readReg(REG_DEVID_MST);
  uint8_t partid    = readReg(REG_PARTID);
  uint8_t revid     = readReg(REG_REVID);

  Serial.print("ADXL DEVID_AD=0x"); Serial.println(devid_ad, HEX);
  Serial.print("ADXL DEVID_MST=0x"); Serial.println(devid_mst, HEX);
  Serial.print("ADXL PARTID=0x"); Serial.println(partid, HEX);
  Serial.print("ADXL REVID=0x"); Serial.println(revid, HEX);

  if ((devid_ad == 0x00 && devid_mst == 0x00 && partid == 0x00) ||
      (devid_ad == 0xFF && devid_mst == 0xFF && partid == 0xFF)) {
    return false;
  }

  writeReg(REG_RANGE, 0x01);
  delay(5);
  writeReg(REG_POWER_CTL, 0x00);
  delay(20);

  bootMs = millis();
  initialized = true;
  return true;
}

void ADXL357Sensor::update(float dt, VibrationData& out) {
  if (!initialized) return;

  int32_t xr, yr, zr;
  if (!readXYZRaw(xr, yr, zr)) return;

  float ax_g = xr / LSB_PER_G_10G;
  float ay_g = yr / LSB_PER_G_10G;
  float az_g = zr / LSB_PER_G_10G;
  lastRawXg = ax_g;
  lastRawYg = ay_g;
  lastRawZg = az_g;

  float ax_mps2 = ax_g * G_TO_MPS2;
  float ay_mps2 = ay_g * G_TO_MPS2;
  float az_mps2 = az_g * G_TO_MPS2;

  if (!accelInitialized) {
    axBase = ax_mps2;
    ayBase = ay_mps2;
    azBase = az_mps2;
    vx = vy = vz = 0;
    accelInitialized = true;
  }

  axBase = BASELINE_ALPHA * axBase + (1.0f - BASELINE_ALPHA) * ax_mps2;
  ayBase = BASELINE_ALPHA * ayBase + (1.0f - BASELINE_ALPHA) * ay_mps2;
  azBase = BASELINE_ALPHA * azBase + (1.0f - BASELINE_ALPHA) * az_mps2;

  float axDyn = ax_mps2 - axBase;
  float ayDyn = ay_mps2 - ayBase;
  float azDyn = az_mps2 - azBase;

  if (fabs(axDyn) < ACC_DEADBAND) axDyn = 0;
  if (fabs(ayDyn) < ACC_DEADBAND) ayDyn = 0;
  if (fabs(azDyn) < ACC_DEADBAND) azDyn = 0;

  if (millis() < bootMs + WARMUP_MS) {
    vx = vy = vz = 0;
  } else {
    vx += axDyn * dt;
    vy += ayDyn * dt;
    vz += azDyn * dt;

    vx *= VEL_DECAY;
    vy *= VEL_DECAY;
    vz *= VEL_DECAY;

    if (fabs(vx) < 0.0005f) vx = 0;
    if (fabs(vy) < 0.0005f) vy = 0;
    if (fabs(vz) < 0.0005f) vz = 0;
  }

  out.x_in_s = vx * MPS_TO_INS;
  out.y_in_s = vy * MPS_TO_INS;
  out.z_in_s = vz * MPS_TO_INS;
  out.total_in_s = sqrtf(out.x_in_s*out.x_in_s + out.y_in_s*out.y_in_s + out.z_in_s*out.z_in_s);

  if (fabs(out.x_in_s) > 2.0f) out.x_in_s = 0;
  if (fabs(out.y_in_s) > 2.0f) out.y_in_s = 0;
  if (fabs(out.z_in_s) > 2.0f) out.z_in_s = 0;
  if (fabs(out.total_in_s) > 2.0f) out.total_in_s = 0;
}

float ADXL357Sensor::getLastRawXg() const {
  return lastRawXg;
}

float ADXL357Sensor::getLastRawYg() const {
  return lastRawYg;
}

float ADXL357Sensor::getLastRawZg() const {
  return lastRawZg;
}

uint8_t ADXL357Sensor::readReg(uint8_t reg) {
  digitalWrite(ADXL_CS, LOW);
  spi.transfer((reg << 1) | 0x01);
  uint8_t val = spi.transfer(0x00);
  digitalWrite(ADXL_CS, HIGH);
  return val;
}

void ADXL357Sensor::writeReg(uint8_t reg, uint8_t value) {
  digitalWrite(ADXL_CS, LOW);
  spi.transfer((reg << 1) & 0xFE);
  spi.transfer(value);
  digitalWrite(ADXL_CS, HIGH);
}

void ADXL357Sensor::readRegs(uint8_t startReg, uint8_t *buf, size_t len) {
  digitalWrite(ADXL_CS, LOW);
  spi.transfer((startReg << 1) | 0x01);
  for (size_t i = 0; i < len; i++) buf[i] = spi.transfer(0x00);
  digitalWrite(ADXL_CS, HIGH);
}

int32_t ADXL357Sensor::convert20Bit(uint8_t b3, uint8_t b2, uint8_t b1) {
  int32_t raw = ((int32_t)b3 << 12) | ((int32_t)b2 << 4) | ((int32_t)b1 >> 4);
  if (raw & 0x80000) raw -= 0x100000;
  return raw;
}

bool ADXL357Sensor::readXYZRaw(int32_t &x, int32_t &y, int32_t &z) {
  uint8_t buf[9];
  readRegs(REG_XDATA3, buf, 9);
  x = convert20Bit(buf[0], buf[1], buf[2]);
  y = convert20Bit(buf[3], buf[4], buf[5]);
  z = convert20Bit(buf[6], buf[7], buf[8]);
  return true;
}
