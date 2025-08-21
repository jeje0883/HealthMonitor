// sensor_max30102.h
#pragma once
#include <Arduino.h>
#include <Wire.h>    
#include "config.h"

// keep SparkFun header isolated to avoid macro/size conflicts
#ifdef I2C_BUFFER_LENGTH
  #undef I2C_BUFFER_LENGTH
#endif
#include <MAX30105.h>

class Max30102Sensor {
public:
  void   begin(TwoWire& bus);
  void   update();

  bool   hasFinger() const { return _hasFinger; }
  bool   beatRecently() const;
  int    bpmRounded() const;      // -1 if not valid
  int    spo2Rounded() const;     // -1 if not valid
  float  perfusionIndex() const;  // smoothed PI (0..~30)

private:
  MAX30105 _dev;
  bool     _ok = false;

  static const int WIN = 100; // 2s @ 50Hz
  int32_t _red[WIN]  = {0};
  int32_t _ir[WIN]   = {0};
  int     _head      = 0;
  int     _filled    = 0;

  uint32_t _lastTick = 0;
  const uint32_t _periodMs = 1000/50; // 50 Hz compute
  bool     _hasFinger = false;

  // HR
  uint32_t _lastPeakMs = 0;
  float    _bpm = 0.0f;
  float    _bpmEMA = 0.0f;
  const float _bpmAlpha = 0.2f;

  // PI
  float    _piEMA = 0.0f;
  uint32_t _lastGoodPIMs = 0;

  // cache of outputs
  float    _spo2 = NAN;

  // helpers
  static inline float mean(const int32_t *a, int n);
  static inline float stddev(const int32_t *a, int n, float m);
  static inline float ac_rms(const int32_t *a, int n, float dc);

  void pushSample(int32_t red, int32_t ir);
  bool detectPeak(float meanIR, float sdIR, int idxPrev, int idxMid, int idxNext) const;
};
