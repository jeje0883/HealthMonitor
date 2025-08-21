#include "sensor_ad8232.h"

void AD8232Sensor::begin(uint8_t adcPin, uint16_t fs, int8_t loPlusPin, int8_t loMinusPin) {
  _adcPin = adcPin;
  _fs = fs;
  _dtMicros = 1000000UL / (fs ? fs : 250);
  _loPlusPin  = loPlusPin;
  _loMinusPin = loMinusPin;

  if (_loPlusPin >= 0)  pinMode(_loPlusPin, INPUT_PULLUP);
  if (_loMinusPin >= 0) pinMode(_loMinusPin, INPUT_PULLUP);

  analogReadResolution(12); // ESP32-C3 ADC: 0..4095
  // Optional: you can call analogSetAttenuation(ADC_11db) if using ESP32 classic; not on C3.

  _head = 0; _count = 0; _hpPrevY = 0; _hpPrevX = 0; _lastRaw = 0;
  _present = true;
  _nextMicros = micros() + _dtMicros;
}

inline int16_t AD8232Sensor::_readADC() const {
  // raw 12-bit
  int v = analogRead(_adcPin);         // 0..4095
  if (v < 0) v = 0; if (v > 4095) v = 4095;
  return (int16_t)v;
}

// One-pole digital high-pass: y[n] = α*(y[n-1] + x[n] - x[n-1])
inline int16_t AD8232Sensor::_highpass(int16_t x) {
  float y = ECG_HP_ALPHA * (_hpPrevY + (float)x - _hpPrevX);
  _hpPrevY = y;
  _hpPrevX = (float)x;

  // Center around 0 and scale down to int16
  if (y > 2047.0f) y = 2047.0f;
  if (y < -2048.0f) y = -2048.0f;
  return (int16_t)y;
}

inline void AD8232Sensor::_push(int16_t v) {
  _ring[_head] = v;
  _head = (_head + 1) % ECG_RING_SAMPLES;
  if (_count < ECG_RING_SAMPLES) _count++;
}

void AD8232Sensor::update() {
  if (!_present) return;

  // Fixed-rate sampling via micros()
  uint32_t now = micros();
  if ((int32_t)(now - _nextMicros) < 0) return; // not yet time
  // catch up if delayed
  while ((int32_t)(now - _nextMicros) >= 0) _nextMicros += _dtMicros;

  int16_t raw = _readADC();
  _lastRaw = raw;

  // Optional simple lead-off via saturation if no LO pins
  bool off = leadsOff();

  int16_t y = off ? 0 : _highpass(raw);  // flatline if leads off
  _push(y);
}

bool AD8232Sensor::leadsOff() const {
  // If LO pins are wired, any low indicates off (depends on breakout; invert if needed)
  if (_loPlusPin >= 0 || _loMinusPin >= 0) {
    bool lop = (_loPlusPin  >= 0) ? (digitalRead(_loPlusPin)  == LOW) : false;
    bool lon = (_loMinusPin >= 0) ? (digitalRead(_loMinusPin) == LOW) : false;
    if (lop || lon) return true;
  }
  // Fallback: if ADC saturates very high/low frequently, treat as off
  return (_lastRaw < 30 || _lastRaw > 4060);
}

size_t AD8232Sensor::getRecent(int16_t* out, size_t maxCount, bool& leadsOff) const {
  leadsOff = this->leadsOff();
  size_t n = _count;
  if (n > maxCount) n = maxCount;
  // copy oldest -> newest
  size_t start = (_head + ECG_RING_SAMPLES + ECG_RING_SAMPLES - n) % ECG_RING_SAMPLES;
  for (size_t i = 0; i < n; i++) {
    out[i] = _ring[(start + i) % ECG_RING_SAMPLES];
  }
  return n;
}
