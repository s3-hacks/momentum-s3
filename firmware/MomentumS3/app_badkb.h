// Bad KB (Momentum Bad USB + BLE): DuckyScript ueber USB oder Bluetooth tippen
#pragma once
#include "views.h"
#include "ducky.h"
#include <LittleFS.h>

static String layoutName() { return S.kbLayout < 0 || S.kbLayout >= KB_LAYOUT_COUNT ? String("en-US") : String(KB_LAYOUTS[S.kbLayout].name); }

static void badkbConfig() {
  int sel = 0;
  while (true) {
    sel = submenu(T("Config","Konfig"), {String(T("Connection: ","Verbindung: ")) + (S.kbBle ? "BLE" : "USB"), String(T("Keyboard layout: ","Tastatur: ")) + layoutName()}, sel);
    if (sel < 0) { settingsSave(); return; }
    if (sel == 0) S.kbBle = !S.kbBle;
    if (sel == 1) {
      std::vector<String> l = {"en-US"};
      for (int i = 0; i < KB_LAYOUT_COUNT; i++) l.push_back(KB_LAYOUTS[i].name);
      int c = submenu(T("Keyboard layout","Tastatur-Layout"), l, S.kbLayout + 1);
      if (c >= 0) S.kbLayout = c - 1;
    }
    settingsSave();
  }
}

enum BkState { BK_READY, BK_CONNECT, BK_RUN, BK_WAITBTN, BK_PAUSED, BK_DONE, BK_ERROR };

static void badkbRun(const String& title, const char* text) {
  ducky_load(text);
  ducky_set_layout(S.kbLayout);
  BkState st = BK_READY;
  uint32_t wakeAt = 0;
  bool printing = false;
  bool bleNow = S.kbBle;
  waitRelease();
  while (true) {
    Key k = readKey();
    if (st == BK_RUN || st == BK_CONNECT || st == BK_WAITBTN) keepAwake();
    if (k == KeyBack) {
      if (st == BK_RUN || st == BK_WAITBTN || st == BK_PAUSED || st == BK_CONNECT) { hid.releaseAll(); st = BK_READY; ducky_rewind(); printing = false; }
      else { hid.end(); return; }
    }
    if (k == KeyDown && (st == BK_READY || st == BK_DONE || st == BK_ERROR)) {
      badkbConfig(); bleNow = S.kbBle; ducky_set_layout(S.kbLayout); st = BK_READY; ducky_rewind();
    }
    if (k == KeyOk) {
      if (st == BK_READY || st == BK_DONE || st == BK_ERROR) {
        ducky_rewind(); ducky_set_layout(S.kbLayout);
        hid.begin(bleNow); st = BK_CONNECT; printing = false;
      } else if (st == BK_WAITBTN) { st = BK_RUN; wakeAt = millis(); }
      else if (st == BK_RUN) { st = BK_PAUSED; hid.releaseAll(); }
      else if (st == BK_PAUSED) { st = BK_RUN; wakeAt = millis(); }
    }
    // Ablauf
    if (st == BK_CONNECT && hid.connected()) { st = BK_RUN; wakeAt = millis() + 800; }
    if (st == BK_RUN && !hid.connected()) st = BK_CONNECT;
    uint32_t t0 = millis();
    while (st == BK_RUN && millis() >= wakeAt && millis() - t0 < 60) {
      if (printing) {
        if (ducky_string_next()) { printing = false; bk.stringdelay = 0; wakeAt = millis() + bk.defdelay; }
        else { uint32_t d = bk.stringdelay ? bk.stringdelay : bk.defstringdelay; wakeAt = millis() + d; }
        continue;
      }
      int32_t r = ducky_execute_next();
      if (r >= 0) wakeAt = millis() + r;
      else if (r == SCRIPT_STATE_STRING_START) { printing = true; bk.string_print_pos = 0; wakeAt = millis(); }
      else if (r == SCRIPT_STATE_WAIT_FOR_BTN) st = BK_WAITBTN;
      else if (r == SCRIPT_STATE_END) { st = BK_DONE; hid.releaseAll(); addXp(2); beep(2400, 60); }
      else { st = BK_ERROR; hid.releaseAll(); beep(600, 150); }
    }
    // Zeichnen (wie Momentum bad_usb_view)
    cv.clear();
    cv.setFont(FontSecondary);
    cv.drawIcon(2, 2, &I_badusb_10px);
    cv.drawStr(16, 10, el_fit(title.c_str(), 108).c_str());
    String sub = String(bleNow ? "BLE" : "USB") + "  [" + layoutName() + "]";
    cv.drawStr(16, 20, sub.c_str());
    int pct = bk.line_nb ? (bk.line_cur * 100 / bk.line_nb) : 0;
    if (st == BK_DONE) pct = 100;
    if (st == BK_READY) {
      cv.drawIcon(4, 26, bleNow ? &I_Bad_BLE_48x22 : &I_UsbTree_48x22);
      cv.drawStr(58, 36, T("Ready","Bereit"));
      cv.drawStr(58, 46, bleNow ? T("Pair 'Momentum S3 KB'","Mit 'Momentum S3 KB'") : T("Plug into a PC","In PC stecken"));
      el_button_left(T("Config","Konfig")); el_button_center(T("Run","Start"));
    } else if (st == BK_CONNECT) {
      cv.drawIcon(4, 26, bleNow ? &I_Bad_BLE_48x22 : &I_UsbTree_48x22);
      cv.drawStr(58, 36, T("Connect to","Verbinde"));
      cv.drawStr(58, 46, bleNow ? T("device (BLE)","Geraet (BLE)") : T("device (USB)","Geraet (USB)"));
      el_button_center("Stop");
    } else if (st == BK_RUN || st == BK_PAUSED || st == BK_WAITBTN) {
      bool f = (millis() / 500) % 2;
      cv.drawIcon(4, 23, st == BK_RUN ? (f ? &I_EviSmile1_18x21 : &I_EviSmile2_18x21) : (f ? &I_EviWaiting1_18x21 : &I_EviWaiting2_18x21));
      cv.setFont(FontBigNumbers);
      char b[8]; snprintf(b, sizeof b, "%d", pct);
      int w = cv.strWidth(b);
      cv.drawStr(114 - w, 42, b);
      cv.drawIcon(116, 28, &I_Percent_10x14);
      cv.setFont(FontSecondary);
      cv.drawStr(28, 36, st == BK_RUN ? T("Running","Laeuft") : st == BK_PAUSED ? T("Paused","Pause") : T("Press OK","OK druecken"));
      if (st == BK_RUN && millis() + 50 < wakeAt) { cv.drawIcon(28, 38, &I_Clock_18x18); }
      el_button_center(st == BK_RUN ? T("Pause","Pause") : T("Resume","Weiter"));
    } else if (st == BK_DONE) {
      cv.drawIcon(4, 26, &I_Smile_18x18);
      cv.setFont(FontPrimary);
      cv.drawStr(28, 40, T("Done","Fertig"));
      el_button_left(T("Config","Konfig")); el_button_center(T("Run","Start"));
    } else if (st == BK_ERROR) {
      cv.drawIcon(4, 26, &I_Error_18x18);
      cv.setFont(FontPrimary);
      char b[24]; snprintf(b, sizeof b, T("ERROR line %d","FEHLER Zeile %d"), bk.error_line);
      cv.drawStr(28, 33, b);
      cv.setFont(FontSecondary);
      cv.drawStr(28, 44, el_fit(bk.error, 98).c_str());
      el_button_left(T("Config","Konfig")); el_button_center(T("Run","Start"));
    }
    cv.flush();
    delay(10);
  }
}

static void appBadKb() {
  if (!LittleFS.exists("/badusb")) LittleFS.mkdir("/badusb");
  int sel = 0;
  while (true) {
    std::vector<String> items;
    std::vector<String> files;
    File d = LittleFS.open("/badusb");
    if (d) { File f = d.openNextFile(); while (f) { String n = f.name(); if (!f.isDirectory() && n.endsWith(".txt")) files.push_back(n); f = d.openNextFile(); } }
    std::sort(files.begin(), files.end());
    for (auto& f : files) items.push_back(f);
    for (int i = 0; i < DEMO_SCRIPT_COUNT; i++) items.push_back(String("Demos/") + DEMO_SCRIPTS[i].name);
    items.push_back(T("Config","Konfig"));
    sel = submenu("Bad KB", items, sel);
    if (sel < 0) return;
    if (sel == (int)items.size() - 1) { badkbConfig(); continue; }
    if (sel < (int)files.size()) {
      File f = LittleFS.open("/badusb/" + files[sel], "r");
      if (!f) continue;
      bk.owned = f.readString(); f.close();
      badkbRun(files[sel], bk.owned.c_str());
    } else {
      int i = sel - files.size();
      badkbRun(DEMO_SCRIPTS[i].name, DEMO_SCRIPTS[i].text);
    }
  }
}
