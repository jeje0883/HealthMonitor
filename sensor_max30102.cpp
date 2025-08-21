// sensor_max30102.cpp
#include "sensor_max30102.h"

inline float Max30102Sensor::mean(const int32_t *a, int n){
  long double s=0; for(int i=0;i<n;i++) s += a[i]; return (float)(s/n);
}
inline float Max30102Sensor::stddev(const int32_t *a, int n, float m){
  long double s=0; for(int i=0;i<n;i++){ long double d=a[i]-m; s+=d*d; }
  return (float)sqrt((double)(s/(n>1?n-1:1)));
}
inline float Max30102Sensor::ac_rms(const int32_t *a, int n, float dc){
  long double s=0; for(int i=0;i<n;i++){ long double d=a[i]-dc; s+=d*d; }
  return (float)sqrt((double)(s/n));
}

void Max30102Sensor::pushSample(int32_t red, int32_t ir){
  _red[_head] = red;
  _ir [_head] = ir;
  _head = (_head + 1) % WIN;
  if (_filled < WIN) _filled++;
}

bool Max30102Sensor::detectPeak(float meanIR, float sdIR, int p, int m, int n) const {
  int32_t a=_ir[p], b=_ir[m], c=_ir[n];
  float thr = meanIR + 0.5f * sdIR;
  return (b>a) && (b>c) && (b>thr);
}

void Max30102Sensor::begin(TwoWire& bus) {
  (void)bus;
  _ok = _dev.begin(Wire, I2C_SPEED_STANDARD);
  if (_ok) {
    // ledBrightness, sampleAverage, ledMode(2=RED+IR), sampleRate, pulseWidth, adcRange
    _dev.setup(80, 4, 2, 50, 411, 16384);
  }
  _lastTick = millis();
}

void Max30102Sensor::update() {
  if (!_ok) return;

  // drain FIFO
  _dev.check();
  while (_dev.available()) {
    long red = _dev.getFIFORed();
    long ir  = _dev.getFIFOIR();
    _dev.nextSample();
    pushSample((int32_t)red, (int32_t)ir);
  }

  // compute cadence
  if (millis() - _lastTick < _periodMs) return;
  _lastTick += _periodMs;

  if (_filled < WIN/2) return;

  float dcRed = mean(_red, _filled);
  float dcIR  = mean(_ir,  _filled);
  float sdIR  = stddev(_ir, _filled, dcIR);
  float acRed = ac_rms(_red, _filled, dcRed);
  float acIR  = ac_rms(_ir,  _filled, dcIR);

  _hasFinger = !(dcIR < DC_NOFINGER || dcRed < DC_NOFINGER);
  if (!_hasFinger) { _bpm=0; _bpmEMA=0; _lastPeakMs=0; }

  // SpO2 via ratio-of-ratios
  float R = (acRed/dcRed) / (acIR/dcIR);
  float spo2 = -45.060f*R*R + 30.354f*R + 94.845f;
  if (spo2 > 100) spo2 = 100;
  if (spo2 < 0 || !_hasFinger) spo2 = NAN;
  _spo2 = spo2;

  // HR peak detect
  if (_hasFinger && _filled >= 3) {
    int mid  = (_head - 2 + WIN) % WIN;
    int prev = (mid - 1 + WIN) % WIN;
    int next = (mid + 1) % WIN;
    if (detectPeak(dcIR, sdIR, prev, mid, next)) {
      uint32_t now = millis();
      uint32_t dt  = now - _lastPeakMs;
      if (dt > 300 && dt < 2000 && _lastPeakMs != 0) {
        _bpm = 60000.0f / (float)dt;
        _bpmEMA = (_bpmEMA == 0.0f) ? _bpm : (_bpmAlpha*_bpm + (1.0f - _bpmAlpha)*_bpmEMA);
      }
      _lastPeakMs = now;
    }
  }

  // PI with guard + clamp + smoothing + hold
  float piRaw = (_hasFinger && dcIR > DC_PI_GUARD) ? (acIR / dcIR) * 100.0f : 0.0f;
  if (piRaw < 0.0f) piRaw = 0.0f;
  if (piRaw > PI_CLAMP_MAX) piRaw = PI_CLAMP_MAX;

  _piEMA = (_piEMA == 0.0f) ? piRaw : (0.25f * piRaw + 0.75f * _piEMA);
  if (piRaw > 0.0f) _lastGoodPIMs = millis();
  if (millis() - _lastGoodPIMs > PI_HOLD_MS) _piEMA = 0.0f;

  // Serial debug (optional)
  // int lastIdx = (_head - 1 + WIN) % WIN;
  // Serial.printf("IR:%ld RED:%ld SpO2:%s BPM:%s PI:%.1f%%\n",
  //   (long)_ir[lastIdx], (long)_red[lastIdx],
  //   isnan(_spo2)?"--":String(_spo2,1).c_str(),
  //   (_bpmEMA==0.0f)?"--":String((int)(_bpmEMA+0.5f)).c_str(),
  //   _piEMA);
}

bool  Max30102Sensor::beatRecently() const {
  return _hasFinger && _lastPeakMs != 0 && (millis() - _lastPeakMs) <= 2000;
}
int   Max30102Sensor::bpmRounded() const {
  if (!beatRecently()) return -1;
  return (int)(_bpmEMA + 0.5f);
}
int   Max30102Sensor::spo2Rounded() const {
  if (isnan(_spo2)) return -1;
  return (int)(_spo2 + 0.5f);
}
float Max30102Sensor::perfusionIndex() const {
  return _piEMA; // already smoothed & held
}
