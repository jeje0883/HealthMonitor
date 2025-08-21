#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "sensor_max30102.h"
#include "sensor_max30205.h"
#include "sensor_ad8232.h"

// Forward declarations:
class Settings;
struct WifiCreds;   // <-- ADD THIS

class WiFiWeb {
public:
  void beginAP(const char* ssid, const char* pass="");
  void beginSTA(const char* ssid, const char* pass, const char* hostname);
  void beginAuto(Settings& settings,
                 const char* ap_ssid, const char* ap_pass,
                 const char* hostname);

  void attachMetricsSource(Max30102Sensor* spo2, Max30205Sensor* tprobe);
  void attachECG(AD8232Sensor* ecg);
  void handle();

private:
  WebServer _srv {80};
  Max30102Sensor* _spo2 = nullptr;
  Max30205Sensor* _tp   = nullptr;
  AD8232Sensor*   _ecg  = nullptr;
  Settings*       _settings = nullptr;
  String          _apSSID;

  void _setupRoutes();
  void _handleRoot();
  void _handleMetrics();
  void _handleConfig();
  void _handleSave();
  void _handleErase();
  void _handleECG();
  static const char* _html();
  static String _htmlConfig(const String& apSsid, const WifiCreds& cur); // OK now
};
