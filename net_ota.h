// net_ota.h
#pragma once
#include <ArduinoOTA.h>

class OTAUpdater {
public:
  void begin(const char* hostname, const char* password);
  void handle();
};
