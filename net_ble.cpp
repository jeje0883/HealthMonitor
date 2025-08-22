#include "net_ble.h"

#ifdef ENABLE_BLE

// Simple GATT: Service UUID and characteristic UUID (custom 128-bit UUIDs)
static const char* SVC_UUID = "a7c9b9b8-6a7e-4f2f-9f9c-2b1a3d8c1234";
static const char* CHR_UUID = "b1e2c3d4-5a6b-7081-92a3-b4c5d6e7f890";

void BLEMetrics::begin(const char* deviceName) {
  BLEDevice::init(deviceName);
  _server = BLEDevice::createServer();
  _svc    = _server->createService(SVC_UUID);
  _ch     = _svc->createCharacteristic(
      CHR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  _ch->addDescriptor(new BLE2902());
  _svc->start();
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID);
  adv->setScanResponse(false);
  adv->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void BLEMetrics::handle() {
  uint32_t now = millis();
  if (now - _lastNotifyMs < NOTIFY_PERIOD_MS) return;
  _lastNotifyMs = now;

  if (!_ch) return;

  // Compose a compact JSON payload (keep under MTU ~ 185 for BLE)
  // {"pulse":78,"spo2":97,"pi":3.2,"tempC":36.6}
  int bpm = -1;
  int o2  = -1;
  float pi = 0.0f;
  bool hasFinger = false;
  bool hasTemp = false;
  float tempC = NAN;

#ifdef ENABLE_MAX30102
  if (_spo2) {
    bpm = _spo2->bpmRounded();
    o2  = _spo2->spo2Rounded();
    pi  = _spo2->perfusionIndex();
    hasFinger = _spo2->hasFinger();
  }
#endif
#ifdef ENABLE_MAX30205
  if (_tp) {
    hasTemp = _tp->hasTemp();
    tempC   = _tp->tempC();
  }
#endif

  char buf[96];
  int n = snprintf(buf, sizeof(buf),
                   "{\"pulse\":%d,\"spo2\":%d,\"pi\":%.2f,\"tempC\":%.2f}",
                   bpm, o2, pi, hasTemp ? tempC : NAN);
  if (n < 0) return;
  _ch->setValue((uint8_t*)buf, (size_t)min(n, (int)sizeof(buf)));
  _ch->notify();
}

#endif // ENABLE_BLE


