// Apps: Snake, Clock, Flashlight, WiFi Scanner, BLE Scanner, Web Files (Upload + Zeit-Sync), Dice
#pragma once
#include "views.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <sys/time.h>
#include <time.h>

// ---------------- Snake (2 Tasten: M5 = links drehen, Seite = rechts drehen) ----------------
static void appSnake() {
  const int CW = 31, CH = 13, CS = 4, OX = 2, OY = 10;
  struct P { int8_t x, y; };
  static P body[CW * CH];
  int len, dir;
  P food;
  int score = 0, best = prefs.begin("mntm", true) ? prefs.getInt("snake", 0) : 0; prefs.end();
  auto place = [&]() {
    while (true) {
      food = {(int8_t)random(CW), (int8_t)random(CH)};
      bool hit = false; for (int i = 0; i < len; i++) if (body[i].x == food.x && body[i].y == food.y) hit = true;
      if (!hit) return;
    }
  };
  auto reset = [&]() { len = 4; dir = 0; for (int i = 0; i < len; i++) body[i] = {(int8_t)(10 - i), 6}; score = 0; place(); };
  reset();
  bool dead = false;
  uint32_t next = millis();
  waitRelease();
  while (true) {
    Key k = readKey();
    keepAwake();
    if (k == KeyBack) return;
    if (dead) { if (k == KeyOk || k == KeyDown) { reset(); dead = false; } }
    else {
      if (k == KeyOk) dir = (dir + 3) % 4;
      if (k == KeyDown) dir = (dir + 1) % 4;
    }
    if (!dead && millis() >= next) {
      next = millis() + max(70, 170 - score * 4);
      P h = body[0];
      if (dir == 0) h.x++; else if (dir == 1) h.y++; else if (dir == 2) h.x--; else h.y--;
      bool hit = h.x < 0 || h.y < 0 || h.x >= CW || h.y >= CH;
      for (int i = 0; i < len - 1 && !hit; i++) if (body[i].x == h.x && body[i].y == h.y) hit = true;
      if (hit) {
        dead = true; beep(300, 200);
        if (score > best) { best = score; prefs.begin("mntm", false); prefs.putInt("snake", best); prefs.end(); }
        addXp(1);
      } else {
        bool eat = h.x == food.x && h.y == food.y;
        if (eat && len < CW * CH) len++;
        for (int i = len - 1; i > 0; i--) body[i] = body[i - 1];
        body[0] = h;
        if (eat) { score++; beep(2600, 15); place(); }
      }
    }
    cv.clear();
    cv.setFont(FontSecondary);
    char b[32]; snprintf(b, sizeof b, "Score: %d", score); cv.drawStr(2, 8, b);
    snprintf(b, sizeof b, "Best: %d", best); cv.drawStr(126 - cv.strWidth(b), 8, b);
    cv.drawFrame(OX - 1, OY - 1, CW * CS + 2, CH * CS + 2);
    for (int i = 0; i < len; i++) cv.drawBox(OX + body[i].x * CS, OY + body[i].y * CS, CS - 1, CS - 1);
    cv.drawFrame(OX + food.x * CS, OY + food.y * CS, CS - 1, CS - 1);
    if (dead) {
      cv.setColor(ColorWhite); cv.drawBox(24, 22, 80, 26); cv.setColor(ColorBlack); cv.drawRFrame(24, 22, 80, 26, 3);
      cv.setFont(FontPrimary); cv.drawStrAligned(64, 32, AlignCenter, AlignCenter, "Game Over");
      cv.setFont(FontSecondary); cv.drawStrAligned(64, 42, AlignCenter, AlignCenter, "OK = again");
    }
    cv.flush();
    delay(10);
  }
}

// ---------------- Uhr ----------------
static bool timeValid() { return time(nullptr) > 1700000000; }
static void rtcToSystem() {
  if (!M5.Rtc.isEnabled()) return;
  auto dt = M5.Rtc.getDateTime();
  if (dt.date.year < 2024) return;
  struct tm t = {};
  t.tm_year = dt.date.year - 1900; t.tm_mon = dt.date.month - 1; t.tm_mday = dt.date.date;
  t.tm_hour = dt.time.hours; t.tm_min = dt.time.minutes; t.tm_sec = dt.time.seconds;
  time_t e = mktime(&t);
  struct timeval tv = {e, 0}; settimeofday(&tv, nullptr);
}
static void appClock() {
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k != KeyNone) return;
    cv.clear();
    if (!timeValid()) {
      cv.setFont(FontPrimary); cv.drawStrAligned(64, 20, AlignCenter, AlignCenter, "Time not set");
      cv.setFont(FontSecondary);
      cv.drawStrAligned(64, 36, AlignCenter, AlignCenter, "Open Apps > Web Files");
      cv.drawStrAligned(64, 46, AlignCenter, AlignCenter, "with your phone to sync");
    } else {
      time_t n = time(nullptr); struct tm t; localtime_r(&n, &t);
      char b[16]; snprintf(b, sizeof b, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
      cv.setFont(FontBigNumbers); cv.drawStrAligned(64, 30, AlignCenter, AlignCenter, b);
      cv.setFont(FontSecondary);
      static const char* WD[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      snprintf(b, sizeof b, "%s %02d.%02d.%04d", WD[t.tm_wday], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
      cv.drawStrAligned(64, 50, AlignCenter, AlignCenter, b);
    }
    cv.flush();
    delay(100);
  }
}

// ---------------- Taschenlampe ----------------
static void appFlashlight() {
  waitRelease();
  M5.Display.setBrightness(255);
  M5.Display.fillScreen(TFT_WHITE);
  while (true) { Key k = readKey(); keepAwake(); if (k != KeyNone) break; delay(20); }
  applyBrightness();
  M5.Display.fillScreen(M5.Display.color888(cv.bg888() >> 16, (cv.bg888() >> 8) & 255, cv.bg888() & 255));
}

// ---------------- Wuerfel ----------------
static void appDice() {
  int sides[] = {4, 6, 8, 10, 12, 20, 100};
  int si = 1, val = 0;
  uint32_t rollUntil = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyBack) return;
    if (k == KeyDown) si = (si + 1) % 7;
    if (k == KeyOk) { rollUntil = millis() + 700; addXp(1); }
    if (millis() < rollUntil) { val = random(sides[si]) + 1; beep(1800 + random(600), 4); }
    cv.clear();
    cv.setFont(FontPrimary);
    char b[16]; snprintf(b, sizeof b, "d%d", sides[si]);
    cv.drawStr(4, 12, b);
    cv.drawRFrame(39, 16, 50, 34, 4);
    cv.setFont(FontBigNumbers);
    snprintf(b, sizeof b, "%d", val);
    if (val) cv.drawStrAligned(64, 33, AlignCenter, AlignCenter, b);
    cv.setFont(FontSecondary);
    el_button_left("Dice"); el_button_center("Roll");
    cv.flush();
    delay(25);
  }
}

// ---------------- WiFi Scanner ----------------
static void appWifiScan() {
  while (true) {
    cv.clear(); cv.drawIcon(52, 12, &A_Loading_24); cv.setFont(FontSecondary);
    cv.drawStrAligned(64, 50, AlignCenter, AlignCenter, "Scanning WiFi..."); cv.flush();
    WiFi.mode(WIFI_STA); WiFi.disconnect();
    int n = WiFi.scanNetworks(false, true);
    if (n <= 0) { popup("No networks", "found", nullptr, 0, 0, 1500); WiFi.mode(WIFI_OFF); return; }
    std::vector<String> items;
    for (int i = 0; i < n; i++) {
      String s = WiFi.SSID(i); if (!s.length()) s = "(hidden)";
      items.push_back(String(WiFi.RSSI(i)) + "  " + s);
    }
    items.push_back("Rescan");
    int sel = 0;
    while (true) {
      sel = submenu(nullptr, items, sel);
      if (sel < 0) { WiFi.scanDelete(); WiFi.mode(WIFI_OFF); return; }
      if (sel == n) break;
      char b[120];
      const char* enc[] = {"Open", "WEP", "WPA", "WPA2", "WPA/WPA2", "WPA2-Ent", "WPA3", "WPA2/WPA3", "WAPI", "OWE"};
      int e = WiFi.encryptionType(sel);
      snprintf(b, sizeof b, "Ch %d  %d dBm\n%s\n%s", (int)WiFi.channel(sel), (int)WiFi.RSSI(sel), WiFi.BSSIDstr(sel).c_str(), e >= 0 && e < 10 ? enc[e] : "?");
      popup(el_fit(WiFi.SSID(sel).c_str(), 120).c_str(), b, nullptr, 0, 0, 8000);
    }
    WiFi.scanDelete();
  }
}

// ---------------- BLE Scanner ----------------
static void appBleScan() {
  while (true) {
    cv.clear(); cv.drawIcon(52, 12, &A_Loading_24); cv.setFont(FontSecondary);
    cv.drawStrAligned(64, 50, AlignCenter, AlignCenter, "Scanning BLE (5s)..."); cv.flush();
    BLEDevice::init("Momentum S3 KB");
    BLEScan* sc = BLEDevice::getScan();
    sc->setActiveScan(true); sc->setInterval(100); sc->setWindow(99);
    BLEScanResults* res = sc->start(5, false);
    int n = res ? res->getCount() : 0;
    std::vector<String> items, info;
    for (int i = 0; i < n; i++) {
      BLEAdvertisedDevice d = res->getDevice(i);
      String nm = d.haveName() ? String(d.getName().c_str()) : String(d.getAddress().toString().c_str());
      items.push_back(String(d.getRSSI()) + "  " + nm);
      info.push_back(String(d.getAddress().toString().c_str()) + "\n" + String(d.getRSSI()) + " dBm" + (d.haveManufacturerData() ? "\nManufacturer data" : ""));
    }
    sc->clearResults();
    if (!n) { popup("No devices", "found", nullptr, 0, 0, 1500); return; }
    items.push_back("Rescan");
    int sel = 0;
    while (true) {
      sel = submenu(nullptr, items, sel);
      if (sel < 0) return;
      if (sel == n) break;
      popup(el_fit(items[sel].c_str(), 120).c_str(), info[sel].c_str(), nullptr, 0, 0, 8000);
    }
  }
}

// ---------------- Web Files (Access Point + Datei-Upload + Zeit-Sync) ----------------
static WebServer* web = nullptr;
static File upFile;
static const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>Momentum S3</title><style>body{font-family:system-ui;background:#111;color:#eee;margin:0;padding:16px;max-width:640px}
h1{color:#ff8200;font-size:22px}section{background:#1d1d1d;border-radius:12px;padding:12px;margin:12px 0}a{color:#ff8200}
li{margin:6px 0;display:flex;justify-content:space-between;gap:8px}button,select,input{font-size:15px}button{background:#ff8200;border:0;border-radius:8px;padding:8px 12px}</style></head>
<body><h1>Momentum S3 Files</h1><section><b>Upload</b><form method=POST action=/upload enctype=multipart/form-data>
<p><select name=dir id=dir><option value=infrared>Infrared (.ir)</option><option value=badusb>Bad KB (.txt)</option></select></p>
<p><input type=file name=f id=f multiple></p><button>Upload</button></form></section><section id=list>Loading...</section>
<script>
fetch('/time?t='+Math.floor(Date.now()/1000)+'&tz='+(-new Date().getTimezoneOffset()));
document.querySelector('form').onsubmit=async e=>{e.preventDefault();const d=document.getElementById('dir').value;for(const f of document.getElementById('f').files){const fd=new FormData();fd.append('f',f,f.name);await fetch('/upload?dir='+d,{method:'POST',body:fd});}load();};
async function load(){const j=await (await fetch('/list')).json();let h='';for(const d in j){h+='<b>'+d+'</b><ul>';for(const f of j[d])h+='<li><a href="/dl?p=/'+d+'/'+encodeURIComponent(f)+'">'+f+'</a><button onclick="del(\'/'+d+'/'+f+'\')">Delete</button></li>';h+='</ul>';}document.getElementById('list').innerHTML=h||'No files';}
async function del(p){if(confirm('Delete '+p+'?')){await fetch('/del?p='+encodeURIComponent(p));load();}}load();
</script></body></html>)HTML";

static String jsonEsc(const String& s) { String o; for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; } return o; }
static bool safePath(const String& p) { return (p.startsWith("/infrared/") || p.startsWith("/badusb/")) && p.indexOf("..") < 0; }

static void appWebFiles() {
  const char* SSID = "Momentum-S3";
  const char* PASS = "momentum123";
  if (!LittleFS.exists("/infrared")) LittleFS.mkdir("/infrared");
  if (!LittleFS.exists("/badusb")) LittleFS.mkdir("/badusb");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASS);
  web = new WebServer(80);
  web->on("/", [] { web->send_P(200, "text/html", WEB_PAGE); });
  web->on("/list", [] {
    String j = "{";
    const char* dirs[] = {"infrared", "badusb"};
    for (int i = 0; i < 2; i++) {
      j += String(i ? "," : "") + "\"" + dirs[i] + "\":[";
      File d = LittleFS.open(String("/") + dirs[i]); bool first = true;
      if (d) { File f = d.openNextFile(); while (f) { if (!f.isDirectory()) { j += String(first ? "" : ",") + "\"" + jsonEsc(f.name()) + "\""; first = false; } f = d.openNextFile(); } }
      j += "]";
    }
    j += "}";
    web->send(200, "application/json", j);
  });
  web->on("/dl", [] {
    String p = web->arg("p");
    if (!safePath(p) || !LittleFS.exists(p)) { web->send(404, "text/plain", "not found"); return; }
    File f = LittleFS.open(p, "r"); web->streamFile(f, "application/octet-stream"); f.close();
  });
  web->on("/del", [] {
    String p = web->arg("p");
    if (safePath(p)) LittleFS.remove(p);
    web->send(200, "text/plain", "ok");
  });
  web->on("/time", [] {
    long t = web->arg("t").toInt(); long tz = web->arg("tz").toInt();
    if (t > 1700000000) {
      t += tz * 60;  // Lokalzeit direkt speichern (keine TZ-Regeln noetig)
      struct timeval tv = {(time_t)t, 0}; settimeofday(&tv, nullptr);
      if (M5.Rtc.isEnabled()) { time_t n = t; struct tm tt; gmtime_r(&n, &tt); M5.Rtc.setDateTime(&tt); }
    }
    web->send(200, "text/plain", "ok");
  });
  web->on("/upload", HTTP_POST, [] { web->send(200, "text/plain", "ok"); }, [] {
    HTTPUpload& u = web->upload();
    String dir = web->arg("dir"); if (dir != "badusb") dir = "infrared";
    if (u.status == UPLOAD_FILE_START) {
      String fn = u.filename; fn.replace("/", "_"); fn.replace("..", "_");
      upFile = LittleFS.open("/" + dir + "/" + fn, "w");
    } else if (u.status == UPLOAD_FILE_WRITE) { if (upFile) upFile.write(u.buf, u.currentSize); }
    else if (u.status == UPLOAD_FILE_END) { if (upFile) upFile.close(); }
  });
  web->begin();
  waitRelease();
  while (true) {
    Key k = readKey();
    keepAwake();
    if (k == KeyBack) break;
    web->handleClient();
    cv.clear();
    cv.setFont(FontPrimary); cv.drawStr(2, 10, "Web Files");
    cv.setFont(FontSecondary);
    cv.drawStr(2, 22, "WiFi:  Momentum-S3");
    cv.drawStr(2, 32, "Pass:  momentum123");
    cv.drawStr(2, 42, "Open:  192.168.4.1");
    char b[32]; snprintf(b, sizeof b, "Clients: %d", WiFi.softAPgetStationNum()); cv.drawStr(2, 52, b);
    cv.drawStr(2, 62, timeValid() ? "Time synced" : "Time: open page to sync");
    cv.flush();
    delay(5);
  }
  web->stop(); delete web; web = nullptr;
  WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF);
}

static void appBleSpam();  // in app_wireless.h
static void appWifiSpam();

static void appApps() {
  int sel = 0;
  while (true) {
    sel = submenu("Apps", {"Snake", "Dice", "Clock", "Flashlight", "WiFi Scanner", "WiFi Spam", "BLE Scanner", "BLE Spam", "Web Files"}, sel);
    if (sel < 0) return;
    switch (sel) {
      case 0: appSnake(); break;
      case 1: appDice(); break;
      case 2: appClock(); break;
      case 3: appFlashlight(); break;
      case 4: appWifiScan(); break;
      case 5: appWifiSpam(); break;
      case 6: appBleScan(); break;
      case 7: appBleSpam(); break;
      case 8: appWebFiles(); break;
    }
  }
}
