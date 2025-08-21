#pragma once
#include <Arduino.h>
#include <Wire.h>

class Max30205Sensor {
public:
  void  begin(TwoWire& bus);
  void  update();                 // call periodically

  // presence vs. validity
  bool  present() const { return _present; }     // detected at startup
  bool  hasTemp() const { return _valid; }       // recent valid reading
  float tempC()   const { return _tempC; }

private:
  TwoWire* _bus = nullptr;
  bool     _present = false;   // device found at boot
  bool     _valid   = false;   // have a recent valid sample
  uint8_t  _addr    = 0x48;
  uint32_t _lastMs  = 0;
  uint32_t _lastGoodMs = 0;
  float    _tempC   = NAN;

  bool detect(uint8_t& addrFound);
  bool readTemp(float& outC);
};
