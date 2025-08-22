#include <Wire.h>
#include "config.h"
#include "display_oled.h"
#include "sensor_max30102.h"
#include "sensor_max30205.h"
#include "sensor_ad8232.h"
#include "settings.h"     // <-- ensure this is included
#include "net_wifiweb.h"
#include "net_ota.h"
#include "net_ble.h"

DisplayOLED     oled;
Max30102Sensor  spo2;
Max30205Sensor  tprobe;
AD8232Sensor    ecg;          // <-- add
WiFiWeb         web;
OTAUpdater      ota;
Settings        settings;
#ifdef ENABLE_BLE
BLEMetrics      ble;
#endif

TwoWire& BUS = Wire;

uint32_t lastUiMs = 0;
const uint32_t UI_PERIOD_MS = 20;

void setup() {
  Serial.begin(115200);
  delay(200);

  settings.begin();

  BUS.begin(I2C_SDA, I2C_SCL);
  BUS.setClock(100000);       // keep bus gentle for multiple devices

  oled.begin(BUS);
  oled.splash();

  // (optional) scanI2C(BUS);

#ifdef ENABLE_MAX30102
  spo2.begin(BUS);
#endif
#ifdef ENABLE_MAX30205
  tprobe.begin(BUS);
#endif
#ifdef ENABLE_AD8232
  ecg.begin(ECG_PIN, ECG_SAMPLE_HZ, ECG_LOP_PIN, ECG_LON_PIN);
#endif

  // Show detect summary (existing method)
  bool has30102 =
#ifdef ENABLE_MAX30102
    spo2.present();
#else
    false;
#endif
  bool has30205 =
#ifdef ENABLE_MAX30205
    tprobe.present();
#else
    false;
#endif
  oled.detectSummary(has30102, has30205);

  // Wi-Fi + Web + OTA
  web.attachMetricsSource(&spo2, &tprobe);
  web.attachECG(&ecg);                      // <-- add
  web.beginAuto(settings, DEFAULT_AP_SSID, DEFAULT_AP_PASS, DEFAULT_HOSTNAME);
  ota.begin(DEFAULT_HOSTNAME, OTA_PASSWORD);

#ifdef ENABLE_BLE
  ble.attachSensors(&spo2, &tprobe);
  ble.begin(DEFAULT_HOSTNAME);
#endif

  Serial.println("Setup complete.");
}

void loop() {
#ifdef ENABLE_MAX30102
  spo2.update();
#endif
#ifdef ENABLE_MAX30205
  tprobe.update();
#endif
#ifdef ENABLE_AD8232
  ecg.update();         // <-- add
#endif

  web.handle();
  ota.handle();
#ifdef ENABLE_BLE
  ble.handle();
#endif

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
