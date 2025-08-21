#include "net_wifiweb.h"
#include <ESPmDNS.h>

void WiFiWeb::beginAP(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_AP);
  _apSSID = ssid;
  bool ok = WiFi.softAP(ssid, pass);
  delay(200);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP mode: %s  IP: %s\n", ssid, ip.toString().c_str());
  _setupRoutes();
}

void WiFiWeb::beginSTA(const char* ssid, const char* pass, const char* hostname) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, pass);
  Serial.printf("Connecting to %s ...\n", ssid);
  unsigned long t0=millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-t0<15000) { delay(300); Serial.print("."); }
  Serial.println();
  if (WiFi.status()==WL_CONNECTED) {
    Serial.printf("STA IP: %s\n", WiFi.localIP().toString().c_str());
    if (MDNS.begin(hostname)) Serial.printf("mDNS: http://%s.local/\n", hostname);
  } else {
    Serial.println("STA failed, staying offline.");
  }
  _setupRoutes();
}

// --- NEW ---
void WiFiWeb::beginAuto(Settings& settings,
                        const char* ap_ssid, const char* ap_pass,
                        const char* hostname) {
  _settings = &settings;
  WifiCreds w = settings.getWifi();
  if (w.valid) {
    Serial.printf("Saved Wi-Fi found: SSID='%s'\n", w.ssid.c_str());
    beginSTA(w.ssid.c_str(), w.pass.c_str(), hostname);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("STA failed, falling back to AP.");
      beginAP(ap_ssid, ap_pass);
    }
  } else {
    Serial.println("No saved Wi-Fi; starting AP.");
    beginAP(ap_ssid, ap_pass);
  }
}

void WiFiWeb::attachMetricsSource(Max30102Sensor* spo2, Max30205Sensor* tprobe) {
  _spo2 = spo2; _tp = tprobe;
}

void WiFiWeb::_setupRoutes() {
  _srv.on("/",            [this]{ _handleRoot(); });
  _srv.on("/api/metrics", [this]{ _handleMetrics(); });
  _srv.on("/config",      [this]{ _handleConfig(); });
  _srv.on("/save",        [this]{ _handleSave(); });
  _srv.on("/erase",       [this]{ _handleErase(); });
  _srv.begin();
  Serial.println("HTTP server started.");
}

void WiFiWeb::_handleRoot() {
  _srv.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _srv.send(200, "text/html", _html());
}

void WiFiWeb::_handleMetrics() {
  String json = "{";
  if (_spo2) {
    json += "\"pulse\":";
    int bpm = _spo2->bpmRounded();
    json += (bpm<0 ? String("null") : String(bpm));
    json += ",\"spo2\":";
    int o2 = _spo2->spo2Rounded();
    json += (o2<0 ? String("null") : String(o2));
    json += ",\"pi\":";
    json += String(_spo2->perfusionIndex(), 1);
    json += ",\"finger\":";
    json += (_spo2->hasFinger() ? "true" : "false");
  } else {
    json += "\"pulse\":null,\"spo2\":null,\"pi\":0,\"finger\":false";
  }
  if (_tp) {
    json += ",\"tempC\":";
    json += (_tp->hasTemp() ? String(_tp->tempC(),1) : String("null"));
  } else json += ",\"tempC\":null";
  json += "}";
  _srv.send(200, "application/json", json);
}

// --- Config portal pages ---
void WiFiWeb::_handleConfig() {
  if (!_settings) { _srv.send(500, "text/plain", "Settings not available"); return; }
  WifiCreds cur = _settings->getWifi();
  String html = _htmlConfig(_apSSID, cur);
  _srv.send(200, "text/html", html);
}

void WiFiWeb::_handleSave() {
  if (!_settings) { _srv.send(500, "text/plain", "Settings not available"); return; }
  String ssid = _srv.hasArg("ssid") ? _srv.arg("ssid") : "";
  String pass = _srv.hasArg("pass") ? _srv.arg("pass") : "";
  if (ssid.length()==0) {
    _srv.send(400, "text/plain", "SSID required");
    return;
  }
  _settings->saveWifi(ssid, pass);
  _srv.send(200, "text/html",
    "<html><body><h3>Saved. Rebooting...</h3><script>setTimeout(()=>{fetch('/reboot')},500);</script></body></html>");
  delay(500);
  ESP.restart();
}

void WiFiWeb::_handleErase() {
  if (!_settings) { _srv.send(500, "text/plain", "Settings not available"); return; }
  _settings->clearWifi();
  _srv.send(200, "text/html", "<html><body><h3>Credentials erased. Rebooting...</h3></body></html>");
  delay(400);
  ESP.restart();
}

const char* WiFiWeb::_html() {
  return
R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32C3 Health Monitor</title>
<style>
body{font-family:system-ui,Arial;margin:20px;background:#0b0e13;color:#eef}
.card{background:#141a22;border-radius:12px;padding:16px;max-width:420px;box-shadow:0 4px 16px rgba(0,0,0,.3)}
h1{font-size:20px;margin:0 0 10px}
.row{display:flex;justify-content:space-between;margin:6px 0}
.label{opacity:.8}
.value{font-weight:600}
.bar{height:10px;background:#222;border-radius:6px;overflow:hidden;margin-top:6px}
.fill{height:100%;width:0;background:#4caf50;transition:width .2s}
small{opacity:.7}
a{color:#8ab4ff;text-decoration:none}
</style></head>
<body>
<div class="card">
  <h1>ESP32-C3 Health Monitor</h1>
  <div class="row"><div class="label">Pulse</div><div class="value" id="bpm">-- bpm</div></div>
  <div class="row"><div class="label">SpO₂</div><div class="value" id="spo2">-- %</div></div>
  <div class="row"><div class="label">Temp</div><div class="value" id="temp">--.- °C</div></div>
  <div class="label">Signal</div>
  <div class="bar"><div class="fill" id="bar"></div></div>
  <div class="row"><small id="status">finger: false</small><small><a href="/config">config</a></small></div>
</div>
<script>
async function tick(){
  try{
    const r = await fetch('/api/metrics'); const j = await r.json();
    document.getElementById('bpm').textContent  = (j.pulse==null?'--':j.pulse)+' bpm';
    document.getElementById('spo2').textContent = (j.spo2==null?'--':j.spo2)+' %';
    document.getElementById('temp').textContent = (j.tempC==null?'--.-':(+j.tempC).toFixed(1))+' °C';
    const pi = Math.max(0, Math.min(10, j.pi || 0)); // 0..10% => 0..100%
    document.getElementById('bar').style.width = (pi*10)+'%';
    document.getElementById('status').textContent = 'finger: '+j.finger;
  }catch(e){ console.log(e); }
}
setInterval(tick,1000); tick();
</script>
</body></html>)HTML";
}

String WiFiWeb::_htmlConfig(const String& apSsid, const WifiCreds& cur) {
  String html =
R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wi-Fi Config</title>
<style>
body{font-family:system-ui,Arial;margin:24px;background:#0b0e13;color:#eef}
.card{background:#141a22;border-radius:12px;padding:16px;max-width:420px;box-shadow:0 4px 16px rgba(0,0,0,.3)}
label{display:block;margin:8px 0 4px}
input{width:100%;padding:8px;border-radius:8px;border:1px solid #333;background:#0f141b;color:#eef}
button{margin-top:12px;padding:10px 16px;border:0;border-radius:10px;background:#4caf50;color:white;font-weight:600}
a{color:#8ab4ff}
</style></head><body>
<div class="card">
<h2>Wi-Fi Configuration</h2>
<p>AP: <b>)HTML";
  html += apSsid + "</b></p>";
  html += "<form action=\"/save\" method=\"get\">";
  html += "<label>SSID</label><input name=\"ssid\" value=\"";
  html += cur.valid ? cur.ssid : "";
  html += "\" placeholder=\"YourWiFi\">";
  html += "<label>Password</label><input name=\"pass\" value=\"";
  html += cur.valid ? cur.pass : "";
  html += "\" placeholder=\"(leave empty for open)\">";
  html += "<button type=\"submit\">Save & Reboot</button>";
  html += "</form><p><a href=\"/erase\">Erase credentials</a> • <a href=\"/\">Back</a></p>";
  html += "</div></body></html>";
  return html;
}

void WiFiWeb::handle() { _srv.handleClient(); }
