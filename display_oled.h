#pragma once
#include <Wire.h>
#include <U8g2lib.h>
#include "config.h"

class DisplayOLED {
public:
  void begin(TwoWire& bus);
  void splash();

  // NEW: brief sensor status screen after splash
  void detectSummary(bool has30102, bool has30205);

  void render(bool beatRecently, int bpm, int spo2, bool hasTemp, float tempC,
              bool hasFinger, float perfIndex);
private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2 {U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA};
};
