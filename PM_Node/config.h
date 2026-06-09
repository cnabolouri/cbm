#pragma once

// ========================= ST7796 TFT =========================
#define TFT_CS    41
#define TFT_DC    42
#define TFT_RST   -1
#define TFT_SCLK  2
#define TFT_MOSI  1
#define TFT_MISO  -1

static const int TFT_W = 480;
static const int TFT_H = 320;

// ========================= JOYSTICK =========================
#define JOY_X_PIN   3
#define JOY_Y_PIN   14
#define JOY_SW_PIN  47

static const unsigned long JOY_DEBOUNCE_MS = 20;
static const unsigned long JOY_LONG_PRESS_MS = 450;

static const float JOY_DEADZONE_NORM = 0.05f;
static const float JOY_FILTER_ALPHA = 0.35f;
static const float POINTER_MAX_SPEED = 120.0f;

// ========================= IIS3DWB VIBRATION =========================
#define IIS_CS 10
#define IIS_SCK 12
#define IIS_MISO 13
#define IIS_MOSI 11

// ========================= TEMP / THERMAL =========================
#define I2C_SDA 8
#define I2C_SCL 9
#define PIN_DS18B20 7

static const int THERMAL_W = 32;
static const int THERMAL_H = 24;
static const int THERMAL_PIXELS = THERMAL_W * THERMAL_H;

static const int MLX90640_REFRESH_HZ = 2;

static const int THERMAL_DRAW_W_TFT = 64;
static const int THERMAL_DRAW_H_TFT = 48;
static const int THERMAL_DRAW_W_WEB = 96;
static const int THERMAL_DRAW_H_WEB = 72;

static const float THERMAL_FIXED_MIN_F = 70.0f;
static const float THERMAL_FIXED_MAX_F = 140.0f;

// ========================= MIC =========================
#define PIN_I2S_BCLK 5
#define PIN_I2S_WS 4
#define PIN_I2S_SD 6
static const int AUDIO_SAMPLE_RATE = 16000;
static const int AUDIO_BUFFER_SAMPLES = 1024;

// ========================= SD =========================
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0 40

// ========================= WIFI =========================
static const char *AP_SSID = "PM-Node";
static const char *AP_PASS = "pmnode123";

// ========================= RECORD =========================
static const unsigned long CSV_LOG_INTERVAL_MS = 50;
