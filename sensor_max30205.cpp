// sensor_max30205.cpp
#include "sensor_max30205.h"
#include <Wire.h>

void Max30205Sensor::begin(TwoWire& bus) {
  _bus = &bus;
  uint8_t a;
  _has = detect(a);
  if (_has) _addr = a;
  _lastMs = millis();
}

bool Max30205Sensor::detect(uint8_t& addrFound) {
  for (uint8_t a=0x48; a<=0x4F; a++) {
    _bus->beginTransmission(a);
    if (_bus->endTransmission()==0) {
      // try reading temp
      _bus->beginTransmission(a);
      _bus->write(0x00);
      if (_bus->endTransmission(false)==0 && _bus->requestFrom((int)a,2)==2) {
        addrFound = a; (void)_bus->read(); (void)_bus->read(); // drain
        return true;
      }
    }
  }
  return false;
}

bool Max30205Sensor::readTemp(float& outC) {
  _bus->beginTransmission(_addr);
  _bus->write(0x00);
  if (_bus->endTransmission(false)!=0) return false;
  if (_bus->requestFrom((int)_addr, 2)!=2) return false;
  uint8_t msb=_bus->read(), lsb=_bus->read();
  int16_t raw = (int16_t)((msb<<8)|lsb);
  outC = (float)raw * 0.00390625f; // 1/256 C per LSB
  return true;
}

void Max30205Sensor::update() {
  if (!_has) return;
  if (millis() - _lastMs < 500) return;
  float t;
  _has = readTemp(t);
  if (_has) _tempC = t;
  _lastMs = millis();
}
