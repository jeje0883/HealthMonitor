#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "sensor_max30102.h"
#include "sensor_max30205.h"
#include "settings.h"

class WiFiWeb {
public:
  // Keep these available, but you'll use beginAuto() now
  void beginAP(const char* ssid, const char* pass="");
  void beginSTA(const char* ssid, const char* pass, const char* hostname);

  // NEW: auto STA/AP based on saved creds; fall back to AP
  void beginAuto(Settings& settings,
                 const char* ap_ssid, const char* ap_pass,
                 const char* hostname);

  void attachMetricsSource(Max30102Sensor* spo2, Max30205Sensor* tprobe);
  void handle();

private:
  WebServer _srv {80};
  Max30102Sensor* _spo2 = nullptr;
  Max30205Sensor* _tp   = nullptr;
  Settings*       _settings = nullptr;
  String          _apSSID;

  void _setupRoutes();
  void _handleRoot();
  void _handleMetrics();
  void _handleConfig();      // GET config form
  void _handleSave();        // POST/GET save form
  void _handleErase();       // GET erase creds
  static const char* _html();
  static String _htmlConfig(const String& apSsid, const WifiCreds& cur);
};
