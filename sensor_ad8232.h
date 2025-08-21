#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Simple ECG capture with ring buffer + high-pass filter
class AD8232Sensor {
public:
  void   begin(uint8_t adcPin = ECG_PIN, uint16_t fs = ECG_SAMPLE_HZ,
               int8_t loPlusPin = ECG_LOP_PIN, int8_t loMinusPin = ECG_LON_PIN);
  void   update();                         // call from loop()
  bool   present() const { return _present; }   // always true when begun
  float  sampleRate() const { return _fs; }

  // Read back the most recent N samples (oldest->newest). Returns count copied.
  size_t getRecent(int16_t* out, size_t maxCount, bool& leadsOff) const;

  // Quick state
  bool   leadsOff() const;                 // based on LO pins or saturation
  int8_t pin() const { return _adcPin; }

private:
  // Config
  uint8_t _adcPin = ECG_PIN;
  int8_t  _loPlusPin  = ECG_LOP_PIN;
  int8_t  _loMinusPin = ECG_LON_PIN;

  // Sampling
  volatile size_t _head = 0;
  volatile size_t _count = 0;
  uint16_t _fs = ECG_SAMPLE_HZ;
  uint32_t _dtMicros = 1000000UL / ECG_SAMPLE_HZ;
  uint32_t _nextMicros = 0;

  // Ring buffer (raw and filtered)
  int16_t  _ring[ECG_RING_SAMPLES];
  int16_t  _lastRaw = 0;
  float    _hpPrevY = 0.0f;
  float    _hpPrevX = 0.0f;

  bool     _present = false;

  // Helpers
  inline int16_t _readADC() const;
  inline int16_t _highpass(int16_t x);
  inline void    _push(int16_t v);
};
