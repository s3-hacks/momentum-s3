// Momentum-Views: Statusleiste, Submenu, Einstellungs-Liste, Dialog, Popup, Tastatur, Level/XP
#pragma once
#include "gui.h"
#include "input.h"
#include <vector>
#include <functional>

// ---------------- Level / XP (wie Momentum dolphin_state.c) ----------------
static const uint32_t DOLPHIN_LEVELS[] = {100, 200, 300, 450, 600, 750, 950, 1150, 1350, 1600, 1850, 2100, 2400, 2700, 3000,
                                          3350, 3700, 4050, 4450, 4850, 5250, 5700, 6150, 6600, 7100, 7600, 8100, 8650, 9999};
static const int DOLPHIN_LEVEL_COUNT = 29;
static int levelFromXp(uint32_t xp) {
  for (int i = 0; i < DOLPHIN_LEVEL_COUNT; i++) if (xp < DOLPHIN_LEVELS[i]) return i + 1;
  return DOLPHIN_LEVEL_COUNT + 1;
}
static int curLevel() { return S.levelOverride > 0 ? S.levelOverride : levelFromXp(S.xp); }
static void levelUpAnimation();
static void addXp(int n) {
  int before = levelFromXp(S.xp);
  S.xp += n;
  if (S.xp > 9999) S.xp = 9999;
  settingsSave();
  if (levelFromXp(S.xp) > before && S.levelOverride == 0) levelUpAnimation();
}

// ---------------- Statusleiste ----------------
static void drawBattery(int x, int y) {
  int pct = M5.Power.getBatteryLevel();
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  bool chg = M5.Power.isCharging() == m5::Power_Class::is_charging;
  cv.drawIcon(x, y, &I_Battery_25x8);
  if (chg) {
    cv.drawBox(x + 1, y + 1, 22, 6);
    cv.setColor(ColorWhite); cv.drawIcon(x + 2, y - 1, &I_Charging_lightning_mask_9x10);
    cv.setColor(ColorBlack); cv.drawIcon(x + 2, y - 1, &I_Charging_lightning_9x10);
  } else if (S.showBatPct) {
    cv.drawBox(x + 1, y + 1, (pct * 22) / 100, 6);
    char b[6]; snprintf(b, sizeof b, "%d", pct);
    // Prozent-Text per XOR ueber den Balken (wie Momentum)
    static M5Canvas tmp;
    if (!tmp.width()) { tmp.setColorDepth(1); tmp.createSprite(24, 8); }
    tmp.fillScreen(0); tmp.setFont(&F_5X7); tmp.setTextColor(1);
    tmp.setTextDatum(textdatum_t::baseline_center); tmp.drawString(b, 11, 7);
    for (int py = 0; py < 8; py++)
      for (int px = 0; px < 24; px++)
        if (tmp.readPixel(px, py)) { int X = x + px, Y = y + py; cv.fb->drawPixel(X, Y, cv.fb->readPixel(X, Y) ? 0 : 1); }
    cv.setColor(ColorBlack);
  } else {
    cv.drawBox(x + 2, y + 2, (pct + 4) / 5, 4);
  }
}
static void drawStatusBar() {
  cv.setColor(ColorWhite);
  cv.drawBox(1, 1, 9, 7); cv.drawBox(7, 3, 58, 6); cv.drawBox(61, 1, 32, 7); cv.drawBox(89, 3, 38, 6);
  cv.setColor(ColorBlack);
  cv.drawIcon(0, 0, &I_Background_128x11);
  drawBattery(100, 2);
}

// ---------------- Submenu (Flipper "submenu") ----------------
struct MenuItem { String label; const Icon* icon; };
// liefert Index oder -1 (Zurueck)
static int submenu(const char* header, const std::vector<String>& items, int start = 0) {
  int pos = constrain(start, 0, (int)items.size() - 1), top = 0;
  uint32_t scrollT = millis();
  waitRelease();
  while (true) {
    Key k = readKey();
    int n = items.size();
    if (k == KeyDown) { pos = (pos + 1) % n; scrollT = millis(); beep(3000, 5); }
    if (k == KeyUp) { pos = (pos + n - 1) % n; scrollT = millis(); beep(3000, 5); }
    if (k == KeyOk) { beep(); return pos; }
    if (k == KeyBack) return -1;
    const int rows = header ? 3 : 4;
    if (pos < top) top = pos;
    if (pos >= top + rows) top = pos - rows + 1;
    cv.clear();
    int y0 = 0;
    if (header) {
      cv.setFont(FontPrimary);
      cv.drawStr(4, 11, header);
      y0 = 16;
    }
    cv.setFont(FontSecondary);
    for (int i = 0; i < rows && top + i < n; i++) {
      int idx = top + i, y = y0 + i * 16;
      if (idx == pos) {
        cv.setColor(ColorBlack); cv.drawRBox(0, y, 123, 16, 2); cv.setColor(ColorWhite);
        el_scroll_text(6, y + 12, 114, items[idx].c_str(), (millis() - scrollT) / 300);
      } else {
        cv.setColor(ColorBlack);
        cv.drawStr(6, y + 12, el_fit(items[idx].c_str(), 114).c_str());
      }
    }
    cv.setColor(ColorBlack);
    el_scrollbar_pos(128, y0, 64 - y0, pos, n);
    cv.flush();
    delay(15);
  }
}

// ---------------- Variable Item List (Einstellungen) ----------------
struct VarItem {
  String label;
  std::function<String()> value;       // aktueller Wert als Text (leer = Aktion)
  std::function<void(int)> change;     // +1 / -1 / 0 = OK
};
static void varItemList(const char* title, std::vector<VarItem>& items) {
  int pos = 0, top = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    int n = items.size();
    if (k == KeyDown) { pos = (pos + 1) % n; beep(3000, 5); }
    if (k == KeyUp) { pos = (pos + n - 1) % n; beep(3000, 5); }
    if (k == KeyOk) { beep(); if (items[pos].change) items[pos].change(+1); settingsSave(); }
    if (k == KeyBack) { settingsSave(); return; }
    const int rows = 4;
    if (pos < top) top = pos;
    if (pos >= top + rows) top = pos - rows + 1;
    cv.clear();
    cv.setFont(FontSecondary);
    for (int i = 0; i < rows && top + i < n; i++) {
      int idx = top + i, y = i * 16;
      bool sel = idx == pos;
      cv.setColor(ColorBlack);
      if (sel) { cv.drawRBox(0, y, 123, 16, 2); cv.setColor(ColorWhite); }
      String v = items[idx].value ? items[idx].value() : "";
      int vw = v.length() ? cv.strWidth(v.c_str()) : 0;
      cv.drawStr(6, y + 12, el_fit(items[idx].label.c_str(), v.length() ? 116 - vw - 14 : 112).c_str());
      if (v.length()) {
        int vx = 118 - vw;
        cv.drawStr(vx, y + 12, v.c_str());
        if (sel) { cv.drawIcon(vx - 6, y + 5, &I_ButtonLeft_4x7); }
      }
      cv.setColor(ColorBlack);
    }
    el_scrollbar(pos, n);
    cv.flush();
    delay(15);
  }
}

// ---------------- Dialog / Popup ----------------
static void popup(const char* header, const char* text, const Icon* icon = nullptr, int ix = 0, int iy = 0, uint32_t ms = 1500) {
  uint32_t t0 = millis();
  waitRelease();
  while (millis() - t0 < ms) {
    Key k = readKey();
    if (k == KeyOk || k == KeyBack) break;
    cv.clear();
    if (icon) cv.drawIconAnim(ix, iy, icon);
    cv.setFont(FontPrimary);
    if (header) cv.drawStrAligned(icon ? 96 : 64, 18, AlignCenter, AlignCenter, header);
    cv.setFont(FontSecondary);
    if (text) {
      int fh = 10, y = 32; const char* t = text; char buf[48];
      while (true) { const char* e = strchr(t, '\n'); int n = e ? e - t : strlen(t); if (n > 47) n = 47;
        memcpy(buf, t, n); buf[n] = 0; cv.drawStrAligned(icon ? 96 : 64, y, AlignCenter, AlignCenter, buf); y += fh; if (!e) break; t = e + 1; }
    }
    cv.flush();
    delay(20);
  }
}
// Ja/Nein-Dialog (OK wechselt Auswahl? -> Runter wechselt, OK bestaetigt)
static bool dialogYesNo(const char* header, const char* text, const char* no = "Cancel", const char* yes = "OK") {
  int sel = 1;
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyDown || k == KeyUp) sel ^= 1;
    if (k == KeyOk) return sel == 1;
    if (k == KeyBack) return false;
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStrAligned(64, 4, AlignCenter, AlignTop, header);
    cv.setFont(FontSecondary);
    cv.drawMultiline(4, 26, text);
    cv.setFont(FontSecondary);
    // Knopf-Leiste: markierte Option schwarz hinterlegt
    int w1 = cv.strWidth(no) + 12, w2 = cv.strWidth(yes) + 12;
    cv.setColor(ColorBlack);
    if (sel == 0) { cv.drawRBox(2, 51, w1, 12, 2); cv.setColor(ColorWhite); } else cv.drawRFrame(2, 51, w1, 12, 2);
    cv.drawStr(8, 60, no); cv.setColor(ColorBlack);
    if (sel == 1) { cv.drawRBox(126 - w2, 51, w2, 12, 2); cv.setColor(ColorWhite); } else cv.drawRFrame(126 - w2, 51, w2, 12, 2);
    cv.drawStr(132 - w2, 60, yes); cv.setColor(ColorBlack);
    cv.flush();
    delay(20);
  }
}

// ---------------- Tastatur (Flipper text_input) ----------------
// Seite = naechste Taste, Seite lang = vorherige, M5 = tippen, M5 lang = fertig
static bool textInput(const char* header, char* buf, int maxLen) {
  static const char* rows[] = {"qwertyuiop0123", "asdfghjkl_456\x01", "zxcvbnm-.!789\x02"};
  // \x01 = Loeschen, \x02 = Fertig ; Grossschreibung via '^' Feld
  const int ROWS = 3;
  int r = 0, c = 0;
  bool upper = false;
  waitRelease();
  int len = strlen(buf);
  while (true) {
    Key k = readKey();
    int rl = strlen(rows[r]);
    if (k == KeyDown) { c++; if (c >= rl) { c = 0; r = (r + 1) % ROWS; } }
    if (k == KeyUp) { c--; if (c < 0) { r = (r + ROWS - 1) % ROWS; c = strlen(rows[r]) - 1; } }
    if (k == KeyBack) return len > 0;
    if (k == KeyOk) {
      char ch = rows[r][c];
      if (ch == '\x01') { if (len > 0) buf[--len] = 0; else upper = !upper; }
      else if (ch == '\x02') return len > 0;
      else if (len < maxLen) {
        if (upper || len == 0) ch = toupper(ch);
        buf[len++] = ch; buf[len] = 0;
      }
      beep(2500, 5);
    }
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStr(2, 8, header);
    cv.drawRFrame(1, 12, 126, 15, 2);
    cv.setFont(FontSecondary);
    String shown = String(buf);
    while (cv.strWidth(shown.c_str()) > 116) shown.remove(0, 1);
    cv.drawStr(4, 23, shown.c_str());
    int cx = 4 + cv.strWidth(shown.c_str());
    if ((millis() / 400) % 2) cv.drawLine(cx + 1, 15, cx + 1, 24);
    cv.setFont(FontKeyboard);
    for (int rr = 0; rr < ROWS; rr++) {
      int n = strlen(rows[rr]);
      for (int cc = 0; cc < n; cc++) {
        int x = 1 + cc * 9, y = 38 + rr * 12;
        char ch = rows[rr][cc];
        bool sel = rr == r && cc == c;
        cv.setColor(ColorBlack);
        if (sel) { cv.drawBox(x - 1, y - 9, 9, 11); cv.setColor(ColorWhite); }
        if (ch == '\x01') cv.drawStr(x, y, "<");
        else if (ch == '\x02') cv.drawStr(x, y, "*");
        else { char s[2] = {(char)((upper || len == 0) && isalpha(ch) ? toupper(ch) : ch), 0}; cv.drawStr(x, y, s); }
        cv.setColor(ColorBlack);
      }
    }
    cv.flush();
    delay(15);
  }
}

// ---------------- Level-Up Animation (Momentum A_Levelup_128x64) ----------------
static void levelUpAnimation() {
  const Icon* ic = &A_Levelup_128x64;
  uint32_t t0 = millis();
  int total = ic->frames * 1000 / (ic->fps ? ic->fps : 2);
  beep(1500, 60); delay(70); beep(2200, 80);
  while (millis() - t0 < (uint32_t)total + 1200) {
    readKey();
    cv.clear();
    int f = min((int)((millis() - t0) * ic->fps / 1000), ic->frames - 1);
    cv.drawIcon(0, 0, ic, f);
    cv.flush();
    delay(30);
  }
}
