#include "settings.h"

void Settings::begin() {
  _prefs.begin(NS, /*readOnly=*/false);
}

WifiCreds Settings::getWifi() {
  WifiCreds w;
  String ssid = _prefs.getString("ssid", "");
  String pass = _prefs.getString("pass", "");
  w.valid = ssid.length() > 0;
  w.ssid  = ssid;
  w.pass  = pass;
  return w;
}

void Settings::saveWifi(const String& ssid, const String& pass) {
  _prefs.putString("ssid", ssid);
  _prefs.putString("pass", pass);
}

void Settings::clearWifi() {
  _prefs.remove("ssid");
  _prefs.remove("pass");
}
