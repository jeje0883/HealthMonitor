// HealthMonitor.ino
#include <Wire.h>      
#include "config.h"
#include "display_oled.h"
#include "sensor_max30102.h"
#include "sensor_max30205.h"
#include "net_wifiweb.h"
#include "net_ota.h"
#include "settings.h"

DisplayOLED     oled;
Max30102Sensor  spo2;
Max30205Sensor  tprobe;
WiFiWeb         web;
OTAUpdater      ota;
Settings        settings;

TwoWire& BUS = Wire;

uint32_t lastUiMs = 0;
const uint32_t UI_PERIOD_MS = 20;

void setup() {
  Serial.begin(115200);
  delay(200);

  settings.begin();

  BUS.begin(I2C_SDA, I2C_SCL);
  oled.begin(BUS);
  oled.splash();

#ifdef ENABLE_MAX30102
  spo2.begin(BUS);
#endif
#ifdef ENABLE_MAX30205
  tprobe.begin(BUS);
#endif

  // NEW: auto STA/AP (saved creds -> STA; else AP; fallback to AP on failure)
  web.attachMetricsSource(&spo2, &tprobe);
  web.beginAuto(settings, DEFAULT_AP_SSID, DEFAULT_AP_PASS, DEFAULT_HOSTNAME);

  ota.begin(DEFAULT_HOSTNAME, OTA_PASSWORD);

  Serial.println("Setup complete.");
}

void loop() {
#ifdef ENABLE_MAX30102
  spo2.update();
#endif
#ifdef ENABLE_MAX30205
  tprobe.update();
#endif

  web.handle();
  ota.handle();

  uint32_t now = millis();
  if (now - lastUiMs >= UI_PERIOD_MS) {
    lastUiMs = now;
#ifdef ENABLE_MAX30102
    bool    hasFinger   = spo2.hasFinger();
    bool    beatRecent  = spo2.beatRecently();
    int     bpm         = spo2.bpmRounded();
    int     o2          = spo2.spo2Rounded();
    float   pi          = spo2.perfusionIndex();
#else
    bool    hasFinger   = false;
    bool    beatRecent  = false;
    int     bpm         = -1;
    int     o2          = -1;
    float   pi          = 0.0f;
#endif

#ifdef ENABLE_MAX30205
    bool    hasTemp     = tprobe.hasTemp();
    float   tempC       = tprobe.tempC();
#else
    bool    hasTemp     = false;
    float   tempC       = NAN;
#endif

    oled.render(beatRecent, bpm, o2, hasTemp, tempC, hasFinger, pi);
  }
}
