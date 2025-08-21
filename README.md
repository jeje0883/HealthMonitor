# ESP32-C3 Health Monitor (MAX30102 + MAX30205 + AD8232 ECG + OLED + Wi‑Fi + OTA)

A modular ESP32-C3 health monitor that reads BPM/SpO₂/Signal strength (PI) from a MAX30102, temperature from a MAX30205, and ECG waveform from an AD8232 analog front-end. It shows metrics on a 0.42" SH1106 OLED (72×40 visible area) and serves a web dashboard + JSON API (including `/api/metrics` and `/api/ecg`) over Wi‑Fi. Includes OTA firmware updates. Sensors are plug-in modules so you can add more later with minimal code changes.

## Hardware & Wiring

Board: ESP32-C3 (e.g., ESP32C3 Dev Module).
OLED: 0.42" SH1106, 72×40 visible (we draw inside a 70×40 window at offset (30,12)).
Bus: I²C shared by all devices.

| Signal | ESP32-C3 | OLED | MAX30102 | MAX30205 |
| ------ | -------- | ---- | -------- | -------- |
| SDA    | GPIO 5   | SDA  | SDA      | SDA      |
| SCL    | GPIO 6   | SCL  | SCL      | SCL      |
| VCC    | 3V3      | 3V3  | 3V3      | 3V3      |
| GND    | GND      | GND  | GND      | GND      |

Notes
• MAX30102 default I²C address: 0x57
• MAX30205 address auto-detected in 0x48–0x4F range
• Keep everything at 3.3V.
• OLED uses U8g2 with a 128×64 buffer; we render inside a 70×40 area at (30,12).

AD8232 (ECG) wiring (not I²C):

- OUT → GPIO 4 (`ECG_PIN`)
- LOP → optional digital pin (`ECG_LOP_PIN`, default disabled)
- LON → optional digital pin (`ECG_LON_PIN`, default disabled)
- VCC → 3V3, GND → GND
  Tip: If you don't wire LO pins, the firmware falls back to saturation-based lead-off detection.

## Project Layout

```
HealthMonitor/
├─ HealthMonitor.ino           # main: boot, init, loop; wires modules together
├─ config.h                    # pins, OLED window, feature flags, thresholds
├─ display_oled.h/.cpp         # U8g2 OLED driver + boot splash + layout
├─ sensor_max30102.h/.cpp      # MAX30102 (BPM/SpO₂/PI) with smoothing/hold
├─ sensor_max30205.h/.cpp      # MAX30205 (temperature), autodetect address
├─ sensor_ad8232.h/.cpp        # AD8232 ECG capture (ADC), HP filter, ring buffer
├─ net_wifiweb.h/.cpp          # Wi-Fi AP/STA + web UI + JSON API + config portal
├─ net_ota.h/.cpp              # OTA update (ArduinoOTA)
├─ settings.h/.cpp             # persistent Wi-Fi creds (Preferences/NVS)
└─ README.md                   # this file
```

## External libraries (Arduino IDE → Library Manager / Boards Manager)

- ESP32 core by Espressif (Board: ESP32C3 Dev Module)
- U8g2 by olikraus
- SparkFun MAX3010x by SparkFun
- Built-ins used: `WiFi.h`, `WebServer.h`, `ESPmDNS.h`, `ArduinoOTA.h`, `Preferences.h`, `Wire.h`.

## Arduino IDE Setup

- Install ESP32 core (Boards Manager)
- Install libraries: U8g2, SparkFun MAX3010x
- Board: ESP32C3 Dev Module
- Upload speed: 115200 (or 921600 if stable)
- USB CDC On Boot: Enabled
- Open `HealthMonitor.ino`

If you see 'TwoWire' has not been declared, make sure `#include <Wire.h>` is at the top of:

- `HealthMonitor.ino`
- `display_oled.h`
- `sensor_max30102.h`
- `sensor_max30205.h`

## Quick Start (first run)

Wire the sensors and OLED as shown above.

Build & Upload `HealthMonitor.ino`.

On boot, device starts in:

AP mode if no saved Wi-Fi creds

AP SSID: `ESP32C3-Health` (default open AP; set in `config.h`)

Connect and open `http://192.168.4.1/`

STA mode if Wi-Fi creds are saved (see next section)

The web dashboard shows Pulse, SpO₂, Temp, and a signal bar (PI).
An ECG waveform canvas is also displayed if AD8232 is enabled and wired.

Put a finger on the MAX30102; Pulse/SpO₂ will appear once stable.

## Wi‑Fi Modes & Config Portal

Auto STA/AP: the app tries STA using saved creds; if it fails or none saved, it starts AP.

Config portal (works in AP or STA):

Visit `/config` (e.g., `http://192.168.4.1/config` in AP mode)

Enter SSID/PASS → Save & Reboot → device comes up in STA mode

`http://esp32c3-health.local/` is available on networks that support mDNS

Erase credentials: open /erase then reboot (it happens automatically).

Defaults (edit in `config.h`):

```c
#define DEFAULT_AP_SSID "ESP32C3-Health"
#define DEFAULT_AP_PASS "" // open AP by default
#define DEFAULT_HOSTNAME "esp32c3-health"
#define OTA_PASSWORD "" // set this before real deployment!
```

ECG (edit in `config.h`):

```c
// Analog input and optional lead-off pins
#define ECG_PIN 4
#define ECG_LOP_PIN -1 // set to a GPIO if wired
#define ECG_LON_PIN -1 // set to a GPIO if wired
// Sampling and filtering
#define ECG_SAMPLE_HZ 250
#define ECG_RING_SAMPLES 1000 // ~4s window
#define ECG_HP_ALPHA 0.995f
```

## Web UI & API

### Endpoints

| Path                    | Method | Content-Type       | Description                                        |
| ----------------------- | ------ | ------------------ | -------------------------------------------------- |
| `/`                     | GET    | `text/html`        | Web dashboard (vitals + ECG canvas)                |
| `/api/metrics`          | GET    | `application/json` | Pulse, SpO₂, PI, finger, temperature               |
| `/api/ecg`              | GET    | `application/json` | Recent ECG samples; query `n=1..800` (default 300) |
| `/config`               | GET    | `text/html`        | Wi‑Fi configuration portal                         |
| `/save?ssid=..&pass=..` | GET    | `text/html`        | Save Wi‑Fi credentials and reboot                  |
| `/erase`                | GET    | `text/html`        | Erase saved Wi‑Fi credentials and reboot           |

Dashboard: /

JSON metrics: `/api/metrics`

```json
{
  "pulse": 78,
  "spo2": 97,
  "pi": 3.4,
  "finger": true,
  "tempC": 36.6
}
```

Config: `/config`

Save Wi-Fi: `/save?ssid=MyWiFi&pass=MyPass`

Erase Wi-Fi: `/erase`

ECG stream: `/api/ecg`

Query params:

- n: number of recent samples (1–800), default 300

Response:

```json
{
  "fs": 250,
  "off": false,
  "samples": [1, 2, 3]
}
```

## OTA Updates

With device on your Wi‑Fi (STA), Arduino IDE → Ports → Network → select `esp32c3-health.local`

Click Upload to perform OTA.

Set `OTA_PASSWORD` in `config.h` before deploying to shared networks.

## OLED Layout

Line 1: Pulse: 075 bpm (hidden as -- if no recent beat)

Line 2: O2:97 T:36.6 (SpO₂ + temperature)

Line 3: Signal bar only (no % text). Full bar = good signal.

Rendering area: `(x=30, y=12, w=70, h=40)` inside a `128×64` buffer.

## Sensor Processing (high-level)

MAX30102

Reads RED+IR via FIFO

Computes DC (mean) and AC (RMS around DC)

SpO₂ via ratio-of-ratios → polynomial fit (clamped 0–100%)

BPM via dynamic-threshold peak detection on IR + EMA smoothing

PI = (AC(IR)/DC(IR))×100%, smoothed; briefly held when DC dips (debounce)

Hides BPM when no recent beat (≤2s) or no finger (low DC)

MAX30205

Reads °C from register `0x00` (LSB = 1/256 °C)

Autodetects address 0x48–0x4F; updates every 500 ms.

Tune in `config.h`:

```c++
static constexpr float DC_NOFINGER = 10000.0f; // lower -> more sensitive
static constexpr float DC_PI_GUARD = 12000.0f; // min DC to compute PI
static constexpr float PI_BAR_FULL = 10.0f; // 10% -> full bar
static constexpr float PI_CLAMP_MAX = 30.0f; // cap serial/UI spikes
static constexpr uint32_t PI_HOLD_MS = 1500; // hold PI after a dip
```

AD8232 (ECG)

Samples analog ECG at ECG_SAMPLE_HZ into a ring buffer (ECG_RING_SAMPLES)

Applies a one-pole high-pass filter (alpha = ECG_HP_ALPHA) to remove baseline drift

Lead-off detection via LO pins (if configured) or ADC saturation fallback

Web UI renders a rolling waveform from /api/ecg samples

## Quick Tests

1. API sanity

```bash
curl http://192.168.4.1/api/metrics
```

# or, on STA/mDNS:

```bash
curl http://esp32c3-health.local/api/metrics
```

2. Check pulse rendering: press firmly, keep still; BPM should appear within a couple of seconds and stabilize.

3. Temperature: touch the MAX30205; temp should move slowly upward.

4. ECG: open the dashboard and verify the waveform updates; or fetch raw samples:

```bash
curl http://192.168.4.1/api/ecg?n=300
```

## Troubleshooting

No OLED output: ensure SDA=5/SCL=6 wiring; panel uses SH1106 w/ visible window at (30,12).

I²C not found: run an I²C scanner at pins 5/6; verify 3.3V power and pullups (most modules include them).

BPM shows --: poor contact/motion → improve placement; watch the signal bar.

PI jumps/drops to 0: brief DC dips are debounced; if still frequent, increase LED current in sensor*max30102.cpp (\_dev.setup(… 80 …) → 100) but avoid clipping; or lower DC*\* guards slightly.

OTA not visible: ensure the ESP32 is in STA mode and on the same network as your PC; some routers block mDNS—use the printed STA IP.

ECG flatline or "leads off": wire LO pins or ensure good electrode contact; if using saturation fallback only, verify ECG_PIN is correct and reduce noise; adjust ECG_HP_ALPHA if baseline drift is excessive.

## Adding a New Sensor (pattern)

Create sensor_newchip.h/.cpp exposing:

void begin(TwoWire& bus);
void update();
// getters for whatever values you want to show/serve

Include and instantiate it in HealthMonitor.ino, call begin() in setup() and update() in loop().

Extend WiFiWeb::\_handleMetrics() to add JSON fields, and DisplayOLED::render() to draw them (or create a second page).
