#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct WifiCreds {
  String ssid;
  String pass;
  bool   valid = false;
};

class Settings {
public:
  void begin();                                // open NVS
  WifiCreds getWifi();                         // read saved creds (if any)
  void saveWifi(const String& ssid, const String& pass); // write creds
  void clearWifi();                            // delete creds
private:
  Preferences _prefs;
  const char* NS = "cfg";
};
