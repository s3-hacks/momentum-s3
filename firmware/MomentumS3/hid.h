// HID-Schnittstelle fuer Bad KB: USB (nativer ESP32-S3-USB) oder Bluetooth (BLE HID)
#pragma once
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "USBHIDConsumerControl.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>

// Flipper-Keycodes: untere 8 Bit = HID-Usage, obere 8 Bit = Modifier-Maske
#define KEY_MOD_LEFT_CTRL  (1 << 8)
#define KEY_MOD_LEFT_SHIFT (1 << 9)
#define KEY_MOD_LEFT_ALT   (1 << 10)
#define KEY_MOD_LEFT_GUI   (1 << 11)
#define HID_KB_LED_NUM 1

static USBHIDKeyboard usbKb;
static USBHIDMouse usbMouse;
static USBHIDConsumerControl usbConsumer;
static bool usbStarted = false;
static volatile uint8_t usbLeds = 0;

static BLEHIDDevice* bleHid = nullptr;
static BLECharacteristic *bleKbIn = nullptr, *bleMouseIn = nullptr, *bleConsIn = nullptr, *bleKbOut = nullptr;
static BLEServer* bleServer = nullptr;
static volatile bool bleConnected = false;
static volatile uint8_t bleLeds = 0;
static bool bleStarted = false;

static const uint8_t BLE_REPORT_MAP[] = {
  // Tastatur (ID 1)
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
  0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0xFF, 0x05, 0x07, 0x19, 0x00, 0x29, 0xFF, 0x81, 0x00,
  0xC0,
  // Maus (ID 2)
  0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x02, 0x09, 0x01, 0xA1, 0x00,
  0x05, 0x09, 0x19, 0x01, 0x29, 0x05, 0x15, 0x00, 0x25, 0x01, 0x95, 0x05, 0x75, 0x01, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x03, 0x81, 0x01,
  0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
  0xC0, 0xC0,
  // Consumer (ID 3)
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x03, 0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03,
  0x75, 0x10, 0x95, 0x01, 0x81, 0x00, 0xC0,
};

class HidSrvCB : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { bleConnected = true; }
  void onDisconnect(BLEServer*) override { bleConnected = false; BLEDevice::startAdvertising(); }
};
class HidLedCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String v = c->getValue();
    if (v.length()) bleLeds = (uint8_t)v[0];
  }
};

static void usbLedCb(void*, esp_event_base_t, int32_t id, void* data) {
  if (id == ARDUINO_USB_HID_KEYBOARD_LED_EVENT) usbLeds = ((arduino_usb_hid_keyboard_event_data_t*)data)->leds;
}

struct HidIf {
  bool ble = false;
  KeyReport rep = {0, 0, {0}};
  uint8_t mouseBtn = 0;

  void begin(bool useBle) {
    ble = useBle;
    if (!ble && !usbStarted) {
      usbKb.onEvent(ARDUINO_USB_HID_KEYBOARD_LED_EVENT, usbLedCb);
      usbKb.begin(); usbMouse.begin(); usbConsumer.begin();
      USB.productName("Momentum S3 KB");
      USB.manufacturerName("Momentum");
      USB.begin();
      usbStarted = true;
    }
    if (ble) {
      if (!bleStarted) {
        BLEDevice::init("Momentum S3 KB");
        bleServer = BLEDevice::createServer();
        bleServer->setCallbacks(new HidSrvCB());
        bleHid = new BLEHIDDevice(bleServer);
        bleKbIn = bleHid->inputReport(1);
        bleKbOut = bleHid->outputReport(1);
        bleKbOut->setCallbacks(new HidLedCB());
        bleMouseIn = bleHid->inputReport(2);
        bleConsIn = bleHid->inputReport(3);
        bleHid->manufacturer()->setValue("Momentum");
        bleHid->pnp(0x02, 0x05AC, 0x820A, 0x0210);
        bleHid->hidInfo(0x00, 0x01);
        bleHid->reportMap((uint8_t*)BLE_REPORT_MAP, sizeof(BLE_REPORT_MAP));
        bleHid->startServices();
        bleHid->setBatteryLevel(100);
        BLESecurity* sec = new BLESecurity();
        sec->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
        sec->setCapability(ESP_IO_CAP_NONE);
        sec->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->setAppearance(0x03C1);  // Tastatur
        adv->addServiceUUID(bleHid->hidService()->getUUID());
        adv->setScanResponse(true);
        bleStarted = true;
      }
      BLEDevice::startAdvertising();
    }
  }
  void end() {
    releaseAll();
    if (ble && bleStarted && !bleConnected) BLEDevice::stopAdvertising();
  }
  bool connected() {
    if (ble) return bleConnected;
    return usbStarted && (bool)USB;
  }
  uint8_t leds() { return ble ? bleLeds : usbLeds; }

  void sendKb() {
    if (ble) { if (bleConnected) { bleKbIn->setValue((uint8_t*)&rep, 8); bleKbIn->notify(); } }
    else usbKb.sendReport(&rep);
    delay(ble ? 12 : 5);
  }
  bool kbPress(uint16_t key) {
    rep.modifiers |= (key >> 8) & 0xFF;
    uint8_t k = key & 0xFF;
    if (k) {
      bool have = false;
      for (int i = 0; i < 6; i++) if (rep.keys[i] == k) have = true;
      if (!have) for (int i = 0; i < 6; i++) if (!rep.keys[i]) { rep.keys[i] = k; break; }
    }
    sendKb();
    return true;
  }
  bool kbRelease(uint16_t key) {
    rep.modifiers &= ~((key >> 8) & 0xFF);
    uint8_t k = key & 0xFF;
    if (k) for (int i = 0; i < 6; i++) if (rep.keys[i] == k) rep.keys[i] = 0;
    sendKb();
    return true;
  }
  void sendMouse(int8_t dx, int8_t dy, int8_t wh) {
    if (ble) { if (bleConnected) { uint8_t r[4] = {mouseBtn, (uint8_t)dx, (uint8_t)dy, (uint8_t)wh}; bleMouseIn->setValue(r, 4); bleMouseIn->notify(); } }
    else { usbMouse.move(dx, dy, wh); }
    delay(ble ? 12 : 5);
  }
  bool mousePress(uint8_t b) {
    mouseBtn |= b;
    if (!ble) usbMouse.press(b); else sendMouse(0, 0, 0);
    return true;
  }
  bool mouseRelease(uint8_t b) {
    mouseBtn &= ~b;
    if (!ble) usbMouse.release(b); else sendMouse(0, 0, 0);
    return true;
  }
  bool mouseMove(int dx, int dy) {
    while (dx || dy) {
      int8_t sx = constrain(dx, -127, 127), sy = constrain(dy, -127, 127);
      sendMouse(sx, sy, 0); dx -= sx; dy -= sy;
    }
    return true;
  }
  bool mouseScroll(int d) { sendMouse(0, 0, constrain(d, -127, 127)); return true; }
  bool consumerPress(uint16_t u) {
    if (ble) { if (bleConnected) { uint8_t r[2] = {(uint8_t)(u & 0xFF), (uint8_t)(u >> 8)}; bleConsIn->setValue(r, 2); bleConsIn->notify(); } }
    else usbConsumer.press(u);
    delay(10);
    return true;
  }
  bool consumerRelease(uint16_t) {
    if (ble) { if (bleConnected) { uint8_t r[2] = {0, 0}; bleConsIn->setValue(r, 2); bleConsIn->notify(); } }
    else usbConsumer.release();
    delay(10);
    return true;
  }
  bool releaseAll() {
    memset(&rep, 0, sizeof rep);
    if (usbStarted || (ble && bleConnected)) sendKb();
    if (mouseBtn) { mouseBtn = 0; if (!ble) usbMouse.release(0x1F); else sendMouse(0, 0, 0); }
    return true;
  }
};
static HidIf hid;
