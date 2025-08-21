#include "sensor_max30205.h"


void Max30205Sensor::begin(TwoWire& bus) {
  _bus = &bus;
  uint8_t a;
  _present = detect(a);
  if (_present) _addr = a;
  _lastMs = millis();
  _lastGoodMs = 0;
}

bool Max30205Sensor::detect(uint8_t& addrFound) {
  for (uint8_t a=0x48; a<=0x4F; a++) {
    _bus->beginTransmission(a);
    if (_bus->endTransmission()==0) {
      // Try a simple read of temp register 0x00
      _bus->beginTransmission(a);
      _bus->write(0x00);
      if (_bus->endTransmission(true)==0 && _bus->requestFrom((int)a,2,true)==2) {
        // drain the two bytes
        (void)_bus->read(); (void)_bus->read();
        addrFound = a;
        return true;
      }
    }
  }
  return false;
}

bool Max30205Sensor::readTemp(float& outC) {
  // Write pointer 0x00, then STOP (true), then read 2 bytes
  _bus->beginTransmission(_addr);
  _bus->write(0x00);
  if (_bus->endTransmission(true)!=0) return false;       // STOP between phases
  if (_bus->requestFrom((int)_addr, 2, true)!=2) return false;  // STOP after read

  uint8_t msb=_bus->read(), lsb=_bus->read();
  int16_t raw = (int16_t)((msb<<8)|lsb);   // two's complement
  outC = (float)raw * 0.00390625f;         // 1/256 °C per LSB
  return true;
}

void Max30205Sensor::update() {
  if (!_present) return;

  // poll every 500 ms
  if (millis() - _lastMs < 500) return;
  _lastMs = millis();

  float t;
  if (readTemp(t)) {
    _tempC = t;
    _valid = true;
    _lastGoodMs = millis();
  } else {
    // don't drop presence on a transient error; only mark invalid after a grace period
    if (_lastGoodMs == 0 || (millis() - _lastGoodMs) > 5000) {
      _valid = false;
    }
    // keep last _tempC value for UI continuity
  }

    // #define TEMP_DEBUG 1
  #ifdef TEMP_DEBUG
    if (_valid) {
      Serial.print("TempC="); Serial.println(_tempC, 2);
    } else {
      Serial.println("MAX30205: read failed (holding last or showing --)");
    }
  #endif
}
