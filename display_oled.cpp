// display_oled.cpp
#include "display_oled.h"

void DisplayOLED::begin(TwoWire& bus) {
  (void)bus;
  u8g2.setBusClock(400000);
  u8g2.begin();
  u8g2.setContrast(255);
}

void DisplayOLED::splash() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(OLED_XOFF, OLED_YOFF + 8,  "ESP32-C3 Boot...");
  for (int i=0; i<=OLED_W; i+=5) {
    u8g2.drawFrame(OLED_XOFF, OLED_YOFF + 16, OLED_W, 10);
    u8g2.drawBox  (OLED_XOFF, OLED_YOFF + 16, i,      10);
    u8g2.drawStr  (OLED_XOFF, OLED_YOFF + 30, "Booting");
    u8g2.drawStr  (OLED_XOFF, OLED_YOFF + 38, "device");
    u8g2.sendBuffer();
    delay(120);
  }
  delay(300);
  u8g2.clearBuffer(); u8g2.sendBuffer();
}

void DisplayOLED::render(bool beatRecently, int bpm, int spo2, bool hasTemp, float tempC,
                         bool hasFinger, float perfIndex) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  char line[28];

  // Line 1: Pulse (hide numbers if no recent beat)
  if (!beatRecently) snprintf(line, sizeof(line), "Pulse: -- bpm");
  else               snprintf(line, sizeof(line), "Pulse: %3d bpm", bpm);
  u8g2.drawStr(OLED_XOFF, OLED_YOFF + 12, line);

  // Line 2: O2 + Temp (compact)
  if (spo2 < 0 && !hasTemp) {
    snprintf(line, sizeof(line), "O2:-- T:--.-");
  } else if (spo2 < 0) {
    snprintf(line, sizeof(line), "O2:-- T:%2.1f", tempC);
  } else if (!hasTemp) {
    snprintf(line, sizeof(line), "O2:%2d T:--.-", spo2);
  } else {
    snprintf(line, sizeof(line), "O2:%2d T:%2.1f", spo2, tempC);
  }
  u8g2.drawStr(OLED_XOFF, OLED_YOFF + 24, line);

  // Line 3: Signal bar only (full = good). Map 0..10% PI to empty..full
  u8g2.drawFrame(OLED_XOFF, OLED_YOFF + 40, OLED_W, 5);
  if (hasFinger && perfIndex > 0.0f) {
    float norm = perfIndex / PI_BAR_FULL;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    int barW = (int)(OLED_W * norm);
    u8g2.drawBox(OLED_XOFF, OLED_YOFF + 40, barW, 5);
  }

  u8g2.sendBuffer();
}
