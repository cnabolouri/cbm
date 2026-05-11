#pragma once

// ========================= TFT + TOUCH =========================
#define TFT_MOSI   1
#define TFT_SCLK   2
#define TFT_CS     41
#define TFT_DC     42
#define TFT_RST    -1

#define TOUCH_MISO 21
#define TOUCH_CS   47
#define TOUCH_IRQ  48

// Touch calibration from your tested setup
static const int TS_MINX = 200;
static const int TS_MAXX = 3800;
static const int TS_MINY = 200;
static const int TS_MAXY = 3800;

static const bool TOUCH_SWAP_XY  = false;
static const bool TOUCH_INVERT_X = true;
static const bool TOUCH_INVERT_Y = true;

// ========================= BUTTON =========================
#define BUTTON_PIN 3
static const unsigned long BUTTON_DEBOUNCE_MS = 35;
static const unsigned long SHORT_PRESS_MS = 40;   // record
static const unsigned long LONG_PRESS_MS  = 700;  // next page

// ========================= ADXL357 =========================
#define ADXL_CS   10
#define ADXL_SCK  12
#define ADXL_MISO 13
#define ADXL_MOSI 11

static const float LSB_PER_G_10G = 51200.0f;
static const float G_TO_MPS2 = 9.80665f;
static const float MPS_TO_INS = 39.3701f;

// ========================= TEMP =========================
#define I2C_SDA 8
#define I2C_SCL 9
#define PIN_DS18B20 7

// ========================= MIC =========================
#define PIN_I2S_BCLK 5
#define PIN_I2S_WS   4
#define PIN_I2S_SD   6
static const int AUDIO_SAMPLE_RATE = 16000;
static const int AUDIO_BUFFER_SAMPLES = 1024;

// ========================= SD =========================
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

// ========================= WIFI =========================
static const char* AP_SSID = "PM-Node";
static const char* AP_PASS = "pmnode123";

// ========================= UI =========================
static const int TFT_W = 320;
static const int TFT_H = 240;

// ========================= RECORD =========================
static const unsigned long CSV_LOG_INTERVAL_MS = 50;
