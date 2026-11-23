// Momentum-artige 128x64-Canvas (1 Bit) -> skaliert auf das 240x135-Display des M5StickS3
#pragma once
#include <M5Unified.h>
#include "gfx_types.h"
#include "gen_fonts.h"
#include "gen_icons.h"

enum Font { FontPrimary, FontSecondary, FontKeyboard, FontBigNumbers, FontBatteryPercent };
enum Color { ColorWhite = 0, ColorBlack = 1 };
enum Align { AlignLeft, AlignRight, AlignTop, AlignBottom, AlignCenter };

static const lgfx::U8g2font F_PRIMARY(u8g2_font_helvB08_tr);
static const lgfx::U8g2font F_SECONDARY(u8g2_font_haxrcorp4089_tr);
static const lgfx::U8g2font F_KEYBOARD(u8g2_font_profont11_mr);
static const lgfx::U8g2font F_BIGNUM(u8g2_font_profont22_tn);
static const lgfx::U8g2font F_5X7(u8g2_font_5x7_tr);

struct Theme { const char* name; uint32_t bg, fg; };
static const Theme THEMES[] = {
  {"Orange", 0xFF8200, 0x000000}, {"Inverted", 0x000000, 0xFF8200}, {"White", 0xFFFFFF, 0x000000},
  {"Night", 0x000000, 0xFFFFFF},  {"Matrix", 0x000000, 0x00FF41},   {"Blue", 0x3D8BFF, 0x000000},
  {"Red", 0xFF3B3B, 0x000000},    {"Pink", 0xFF5FB4, 0x000000},     {"Purple", 0xA066FF, 0x000000},
  {"Lime", 0xB6FF3B, 0x000000},
};
static const int THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

class Canvas {
 public:
  M5Canvas land, port;
  M5Canvas* fb = nullptr;
  bool portrait = false;
  uint8_t color = ColorBlack;
  const lgfx::IFont* font = &F_PRIMARY;
  Font fontId = FontPrimary;
  int theme = 0;
  lgfx::rgb565_t cBg, cFg;
  lgfx::rgb565_t line[240 * 2];
  int16_t xmapL[240], ymapP[240];

  void begin() {
    land.setColorDepth(1); land.createSprite(128, 64);
    port.setColorDepth(1); port.createSprite(64, 128);
    for (int x = 0; x < 240; x++) xmapL[x] = x * 128 / 240;
    for (int y = 0; y < 240; y++) ymapP[y] = y * 128 / 240;
    fb = &land;
    setTheme(0);
    M5.Display.setRotation(3);
    M5.Display.fillScreen(M5.Display.color888(THEMES[0].bg >> 16, (THEMES[0].bg >> 8) & 255, THEMES[0].bg & 255));
  }
  void setTheme(int t) {
    theme = (t < 0 || t >= THEME_COUNT) ? 0 : t;
    uint32_t b = THEMES[theme].bg, f = THEMES[theme].fg;
    cBg = lgfx::rgb565_t(b >> 16, (b >> 8) & 255, b & 255);
    cFg = lgfx::rgb565_t(f >> 16, (f >> 8) & 255, f & 255);
  }
  uint32_t bg888() { return THEMES[theme].bg; }
  void setPortrait(bool p) {
    if (p == portrait) return;
    portrait = p;
    fb = p ? &port : &land;
    M5.Display.setRotation(p ? 0 : 3);
    uint32_t b = THEMES[theme].bg;
    M5.Display.fillScreen(M5.Display.color888(b >> 16, (b >> 8) & 255, b & 255));
  }
  int width() { return fb->width(); }
  int height() { return fb->height(); }

  // ---- Grundfunktionen im Flipper-Stil ----
  void clear() { fb->fillScreen(0); color = ColorBlack; }
  void setColor(Color c) { color = c; }
  void invert() { color = color ? 0 : 1; }
  void setFont(Font f) {
    fontId = f;
    switch (f) {
      case FontPrimary: font = &F_PRIMARY; break;
      case FontSecondary: font = &F_SECONDARY; break;
      case FontKeyboard: font = &F_KEYBOARD; break;
      case FontBigNumbers: font = &F_BIGNUM; break;
      default: font = &F_5X7; break;
    }
    fb->setFont(font);
  }
  int fontHeight() {
    fb->setFont(font);
    int h = fb->fontHeight();
    return h;
  }
  // Grossbuchstaben-Hoehe wie Flipper (fuer AlignCenter)
  int capHeight() {
    switch (fontId) { case FontPrimary: return 8; case FontSecondary: return 7; case FontKeyboard: return 8; case FontBigNumbers: return 15; default: return 7; }
  }
  int strWidth(const char* s) { fb->setFont(font); return fb->textWidth(s); }
  void drawStr(int x, int y, const char* s) {
    fb->setFont(font);
    fb->setTextColor(color);
    fb->setTextDatum(textdatum_t::baseline_left);
    fb->drawString(s, x, y);
  }
  void drawStrAligned(int x, int y, Align h, Align v, const char* s) {
    int w = strWidth(s);
    if (h == AlignCenter) x -= w / 2; else if (h == AlignRight) x -= w;
    int ch = capHeight();
    if (v == AlignTop) y += ch; else if (v == AlignCenter) y += ch / 2;
    drawStr(x, y, s);
  }
  void drawDot(int x, int y) { fb->drawPixel(x, y, color); }
  void drawLine(int x1, int y1, int x2, int y2) { fb->drawLine(x1, y1, x2, y2, color); }
  void drawBox(int x, int y, int w, int h) { if (w > 0 && h > 0) fb->fillRect(x, y, w, h, color); }
  void drawFrame(int x, int y, int w, int h) { if (w > 0 && h > 0) fb->drawRect(x, y, w, h, color); }
  void drawRFrame(int x, int y, int w, int h, int r) { fb->drawRoundRect(x, y, w, h, r, color); }
  void drawRBox(int x, int y, int w, int h, int r) { fb->fillRoundRect(x, y, w, h, r, color); }
  void drawCircle(int x, int y, int r) { fb->drawCircle(x, y, r, color); }
  void drawDisc(int x, int y, int r) { fb->fillCircle(x, y, r, color); }
  void drawXbm(int x, int y, int w, int h, const uint8_t* d) { fb->drawXBitmap(x, y, d, w, h, color); }
  void drawIcon(int x, int y, const Icon* ic, uint32_t frame = 0) {
    if (!ic) return;
    drawXbm(x, y, ic->w, ic->h, ic->data[frame % ic->frames]);
  }
  // Animiertes Icon nach Zeit
  void drawIconAnim(int x, int y, const Icon* ic) {
    uint32_t f = ic->fps ? (millis() * ic->fps / 1000) : 0;
    drawIcon(x, y, ic, f);
  }
  // Mehrzeiliger Text (\n)
  void drawMultiline(int x, int y, const char* t) {
    int fh = fontHeight();
    char buf[96];
    while (*t) {
      const char* e = strchr(t, '\n');
      int n = e ? (e - t) : strlen(t);
      if (n > 95) n = 95;
      memcpy(buf, t, n); buf[n] = 0;
      drawStr(x, y, buf);
      y += fh;
      if (!e) break;
      t = e + 1;
    }
  }

  // ---- auf das echte Display schieben ----
  void flush() {
    auto& d = M5.Display;
    d.startWrite();
    // Raender immer in der aktuellen Theme-Farbe (sonst bleibt alter Rand stehen)
    uint32_t b = THEMES[theme].bg;
    uint16_t bc = d.color565(b >> 16, (b >> 8) & 255, b & 255);
    if (!portrait) { d.fillRect(0, 0, 240, 3, bc); d.fillRect(0, 131, 240, 4, bc); }
    else { d.fillRect(0, 0, 3, 240, bc); d.fillRect(131, 0, 4, 240, bc); }
    if (!portrait) {
      // 128x64 -> 240x128 (x*1.875, y*2), Rand oben 3px
      for (int sy = 0; sy < 64; sy++) {
        for (int dx = 0; dx < 240; dx++) {
          lgfx::rgb565_t c = land.readPixel(xmapL[dx], sy) ? cFg : cBg;
          line[dx] = c; line[240 + dx] = c;
        }
        d.pushImage(0, 3 + sy * 2, 240, 2, line);
      }
    } else {
      // 64x128 -> 128x240 (x*2, y*1.875), Rand links 3px
      for (int dy = 0; dy < 240; dy++) {
        int sy = ymapP[dy];
        for (int dx = 0; dx < 128; dx++) line[dx] = port.readPixel(dx >> 1, sy) ? cFg : cBg;
        d.pushImage(3, dy, 128, 1, line);
      }
    }
    d.endWrite();
  }
};

static Canvas cv;

// ================= Elemente (portiert aus Momentum gui/elements.c) =================
static void el_frame(int x, int y, int width, int height) {
  cv.drawLine(x + 2, y, x + width - 2, y);
  cv.drawLine(x + 1, y + height - 1, x + width, y + height - 1);
  cv.drawLine(x + 2, y + height, x + width - 1, y + height);
  cv.drawLine(x, y + 2, x, y + height - 2);
  cv.drawLine(x + width - 1, y + 1, x + width - 1, y + height - 2);
  cv.drawLine(x + width, y + 2, x + width, y + height - 2);
  cv.drawDot(x + 1, y + 1);
}
static void el_scrollbar_pos(int x, int y, int height, int pos, int total) {
  cv.setColor(ColorWhite); cv.drawBox(x - 3, y, 3, height);
  cv.setColor(ColorBlack);
  for (int i = y; i < height + y; i += 2) cv.drawDot(x - 2, i);
  if (total) {
    float bh = (float)height / total;
    cv.drawBox(x - 3, y + (int)(bh * pos), 3, bh < 1 ? 1 : (int)ceilf(bh));
  }
}
static void el_scrollbar(int pos, int total) { el_scrollbar_pos(cv.width(), 0, cv.height(), pos, total); }

static void el_scrollbar_horizontal(int x, int y, int width, int pos, int total) {
  cv.setColor(ColorWhite); cv.drawBox(x, y - 3, width, 3);
  cv.setColor(ColorBlack);
  for (int i = x; i < width + x; i += 2) cv.drawDot(i, y - 2);
  if (total) {
    float bw = (float)width / total;
    cv.drawBox(x + (int)(bw * pos), y - 3, bw < 1 ? 1 : (int)bw, 3);
  }
}
static void el_slightly_rounded_frame(int x, int y, int w, int h) { cv.drawRFrame(x, y, w, h, 1); }
static void el_bold_rounded_frame(int x, int y, int width, int height) {
  cv.setColor(ColorWhite); cv.drawBox(x + 2, y + 2, width - 3, height - 3);
  cv.setColor(ColorBlack);
  cv.drawLine(x + 3, y, x + width - 3, y);
  cv.drawLine(x + 2, y + 1, x + width - 2, y + 1);
  cv.drawLine(x, y + 3, x, y + height - 3);
  cv.drawLine(x + 1, y + 2, x + 1, y + height - 2);
  cv.drawLine(x + width, y + 3, x + width, y + height - 3);
  cv.drawLine(x + width - 1, y + 2, x + width - 1, y + height - 2);
  cv.drawLine(x + 3, y + height, x + width - 3, y + height);
  cv.drawLine(x + 2, y + height - 1, x + width - 2, y + height - 1);
  cv.drawDot(x + 2, y + 2); cv.drawDot(x + 3, y + 2); cv.drawDot(x + 2, y + 3);
  cv.drawDot(x + width - 2, y + 2); cv.drawDot(x + width - 3, y + 2); cv.drawDot(x + width - 2, y + 3);
  cv.drawDot(x + 2, y + height - 2); cv.drawDot(x + 3, y + height - 2); cv.drawDot(x + 2, y + height - 3);
  cv.drawDot(x + width - 2, y + height - 2); cv.drawDot(x + width - 3, y + height - 2); cv.drawDot(x + width - 2, y + height - 3);
}

static void el_progress(int x, int y, int width, float p) {
  if (p < 0) p = 0; if (p > 1) p = 1;
  cv.setColor(ColorWhite); cv.drawBox(x + 1, y + 1, width - 2, 7);
  cv.setColor(ColorBlack); cv.drawRFrame(x, y, width, 9, 3);
  cv.drawBox(x + 1, y + 1, (int)roundf(p * (width - 2)), 7);
}

static void el_button_left(const char* s) {
  int bw = cv.strWidth(s) + 6 + 7, y = cv.height();
  cv.drawBox(0, y - 12, bw, 12);
  for (int i = 0; i < 3; i++) cv.drawLine(bw + i, y, bw + i, y - 12 + i);
  cv.invert(); cv.drawIcon(3, y - 10, &I_ButtonLeft_4x7); cv.drawStr(3 + 7, y - 3, s); cv.invert();
}
static void el_button_right(const char* s) {
  int bw = cv.strWidth(s) + 6 + 7, y = cv.height(), x = cv.width();
  cv.drawBox(x - bw, y - 12, bw, 12);
  for (int i = 0; i < 3; i++) cv.drawLine(x - bw - 1 - i, y, x - bw - 1 - i, y - 12 + i);
  cv.invert(); cv.drawStr(x - bw + 3, y - 3, s); cv.drawIcon(x - 3 - 4, y - 10, &I_ButtonRight_4x7); cv.invert();
}
static void el_button_center(const char* s) {
  int bw = cv.strWidth(s) + 2 + 10, y = cv.height(), x = (cv.width() - bw) / 2;
  cv.drawBox(x, y - 12, bw, 12);
  for (int i = 0; i < 3; i++) { cv.drawLine(x - 1 - i, y, x - 1 - i, y - 12 + i); cv.drawLine(x + bw + i, y, x + bw + i, y - 12 + i); }
  cv.invert(); cv.drawIcon(x + 1, y - 10, &I_ButtonCenter_7x7); cv.drawStr(x + 1 + 10, y - 3, s); cv.invert();
}

static void el_bubble_str(int x, int y, const char* text, Align h, Align v) {
  int fh = cv.fontHeight();
  if (cv.fontId == FontSecondary) fh += 1;
  int sw = 0, lines = 1;
  char buf[64]; const char* t = text;
  while (true) {
    const char* e = strchr(t, '\n'); int n = e ? e - t : strlen(t); if (n > 63) n = 63;
    memcpy(buf, t, n); buf[n] = 0; int w = cv.strWidth(buf); if (w > sw) sw = w;
    if (!e) break; lines++; t = e + 1;
  }
  int fx = x, fy = y, fw = sw + 8, fhh = fh * lines + 4;
  cv.setColor(ColorWhite); cv.drawBox(fx + 1, fy + 1, fw - 2, fhh - 2);
  cv.setColor(ColorBlack); cv.drawRFrame(fx, fy, fw, fhh, 1);
  { // multiline mit Flipper-Zeilenhoehe
    int yy = y - 1 + fh; const char* tt = text;
    while (true) { const char* e = strchr(tt, '\n'); int n = e ? e - tt : strlen(tt); if (n > 63) n = 63;
      memcpy(buf, tt, n); buf[n] = 0; cv.drawStr(x + 4, yy, buf); yy += fh; if (!e) break; tt = e + 1; }
  }
  int x1 = 0, x2 = 0, x3 = 0, y1 = 0, y2 = 0, y3 = 0;
  if (h == AlignLeft && v == AlignTop) { x1 = fx; y1 = fy; x2 = fx - 4; y2 = fy; x3 = fx; y3 = fy + 4; cv.setColor(ColorWhite); cv.drawBox(x2 + 2, y2 + 1, 2, 2); }
  else if (h == AlignLeft && v == AlignCenter) { x1 = fx; y1 = fy + (fhh - 1) / 2 - 4; x2 = fx - 4; y2 = fy + (fhh - 1) / 2; x3 = fx; y3 = fy + (fhh - 1) / 2 + 4; cv.setColor(ColorWhite); cv.drawBox(x2 + 2, y2 - 2, 2, 5); cv.drawDot(x2 + 1, y2); }
  else if (h == AlignLeft && v == AlignBottom) { x1 = fx; y1 = fy + fhh - 1 - 4; x2 = fx - 4; y2 = fy + fhh - 1; x3 = fx; y3 = fy + fhh - 1; cv.setColor(ColorWhite); cv.drawBox(x2 + 2, y2 - 2, 2, 2); }
  else if (h == AlignRight && v == AlignTop) { x1 = fx + fw - 1; y1 = fy; x2 = fx + fw - 1 + 4; y2 = fy; x3 = fx + fw - 1; y3 = fy + 4; cv.setColor(ColorWhite); cv.drawBox(x2 - 3, y2 + 1, 2, 2); }
  else if (h == AlignRight && v == AlignCenter) { x1 = fx + fw - 1; y1 = fy + (fhh - 1) / 2 - 4; x2 = fx + fw - 1 + 4; y2 = fy + (fhh - 1) / 2; x3 = fx + fw - 1; y3 = fy + (fhh - 1) / 2 + 4; cv.setColor(ColorWhite); cv.drawBox(x2 - 3, y2 - 2, 2, 5); cv.drawDot(x2 - 1, y2); }
  else if (h == AlignRight && v == AlignBottom) { x1 = fx + fw - 1; y1 = fy + fhh - 1 - 4; x2 = fx + fw - 1 + 4; y2 = fy + fhh - 1; x3 = fx + fw - 1; y3 = fy + fhh - 1; cv.setColor(ColorWhite); cv.drawBox(x2 - 3, y2 - 2, 2, 2); }
  else if (h == AlignCenter && v == AlignTop) { x1 = fx + (fw - 1) / 2 - 4; y1 = fy; x2 = fx + (fw - 1) / 2; y2 = fy - 4; x3 = fx + (fw - 1) / 2 + 4; y3 = fy; cv.setColor(ColorWhite); cv.drawBox(x2 - 2, y2 + 2, 5, 2); cv.drawDot(x2, y2 + 1); }
  else if (h == AlignCenter && v == AlignBottom) { x1 = fx + (fw - 1) / 2 - 4; y1 = fy + fhh - 1; x2 = fx + (fw - 1) / 2; y2 = fy + fhh - 1 + 4; x3 = fx + (fw - 1) / 2 + 4; y3 = fy + fhh - 1; cv.setColor(ColorWhite); cv.drawBox(x2 - 2, y2 - 3, 5, 2); cv.drawDot(x2, y2 - 1); }
  else { cv.setColor(ColorBlack); return; }
  cv.setColor(ColorWhite); cv.drawLine(x3, y3, x1, y1);
  cv.setColor(ColorBlack); cv.drawLine(x1, y1, x2, y2); cv.drawLine(x2, y2, x3, y3);
}

// Text auf Breite kuerzen ("...")
static String el_fit(const char* s, int width) {
  String r = s;
  if (cv.strWidth(r.c_str()) <= width) return r;
  while (r.length() > 0 && cv.strWidth((r + "...").c_str()) > width) r.remove(r.length() - 1);
  return r + "...";
}
// laufender Text fuer markierte Menuezeile
static void el_scroll_text(int x, int y, int width, const char* s, uint32_t scroll) {
  int w = cv.strWidth(s);
  if (w <= width) { cv.drawStr(x, y, s); return; }
  int len = strlen(s);
  int off = 0;
  if (scroll > 2) off = (scroll - 2) % (len + 4);
  String r;
  if (off < len) r = String(s + off); else r = "";
  if (off >= len - 3) r = r + "    " + String(s);
  cv.drawStr(x, y, el_fit(r.c_str(), width).c_str());
}
