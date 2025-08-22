#pragma once
#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "sensor_max30102.h"
#include "sensor_max30205.h"

class BLEMetrics {
public:
  void attachSensors(Max30102Sensor* spo2, Max30205Sensor* tprobe) {
    _spo2 = spo2;
    _tp   = tprobe;
  }

  void begin(const char* deviceName);
  void handle();

private:
  Max30102Sensor* _spo2 = nullptr;
  Max30205Sensor* _tp   = nullptr;

  BLEServer*      _server = nullptr;
  BLEService*     _svc    = nullptr;
  BLECharacteristic* _ch  = nullptr;

  uint32_t _lastNotifyMs = 0;
  static constexpr uint32_t NOTIFY_PERIOD_MS = 1000; // 1 Hz updates
};

#endif // ENABLE_BLE


