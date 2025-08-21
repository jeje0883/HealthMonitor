// net_ota.cpp
#include "net_ota.h"
#include <ESPmDNS.h>

void OTAUpdater::begin(const char* hostname, const char* password){
  ArduinoOTA.setHostname(hostname);
  if (password && password[0]) ArduinoOTA.setPassword(password);
  ArduinoOTA
    .onStart([](){ Serial.println("OTA Start"); })
    .onEnd([](){ Serial.println("OTA End"); })
    .onProgress([](unsigned int p, unsigned int t){
      static uint8_t last=0; uint8_t pct = (p*100)/t;
      if (pct!=last){ last=pct; Serial.printf("OTA %u%%\n", pct); }
    })
    .onError([](ota_error_t e){ Serial.printf("OTA Error[%u]\n", e); });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}
void OTAUpdater::handle(){ ArduinoOTA.handle(); }
