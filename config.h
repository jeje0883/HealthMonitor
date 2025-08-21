#pragma once
#include <Arduino.h>

// --------- I2C pins for Abrobot ESP32-C3 OLED board ----------
#define I2C_SDA 5
#define I2C_SCL 6

// --------- OLED window (inside 128x64 RAM) ----------
static const int OLED_XOFF = 30;
static const int OLED_YOFF = 12;
static const int OLED_W    = 70;   // was 72; we use 70 for nicer margins
static const int OLED_H    = 40;

// --------- Feature switches ----------
#define ENABLE_MAX30102
#define ENABLE_MAX30205

// --------- Wi-Fi / Web ----------
#define DEFAULT_AP_SSID     "ESP32C3-Health"
#define DEFAULT_AP_PASS     ""              // empty = open AP
#define DEFAULT_HOSTNAME    "esp32c3-health" // mDNS for STA mode

// --------- OTA ----------
#define OTA_PASSWORD        ""              // set a password before deploying!

// --------- PI + HR logic tuning ----------
static constexpr float DC_NOFINGER   = 10000.0f;  // DC gate for "finger present"
static constexpr float DC_PI_GUARD   = 12000.0f;  // DC min to compute PI
static constexpr float PI_BAR_FULL   = 10.0f;     // 10% => full bar
static constexpr float PI_CLAMP_MAX  = 30.0f;     // clamp absurd spikes for UI
static constexpr uint32_t PI_HOLD_MS = 1500;      // hold last PI when DC dips
