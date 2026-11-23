#pragma once
#include <stdint.h>
#include <pgmspace.h>

// Momentum-Icon (XBM, LSB zuerst), evtl. animiert
struct Icon {
  uint8_t w, h;
  uint8_t frames;
  uint8_t fps;
  const uint8_t* const* data;
};

struct Bubble {
  uint8_t slot;
  int16_t x, y;
  const char* text;
  uint8_t alignH, alignV;  // 0=Left/Top 1=Center 2=Right/Bottom
  uint8_t start, end;
};

struct DolphinAnim {
  const char* name;
  uint8_t w, h;
  const uint8_t* const* frames;
  const uint8_t* order;
  uint8_t orderLen;
  uint8_t passive, active;
  uint8_t fps;
  uint16_t duration;
  uint8_t activeCycles;
  uint8_t activeCooldown;
  uint8_t minLevel;
  const Bubble* bubbles;
  uint8_t bubbleCount;
  uint8_t maxLevel;
  uint8_t weight;
};

struct PanelItem {
  int16_t x, y;
  const Icon* icon;
  const Icon* hover;
  const char* name;
};
struct PanelIcon {
  int16_t x, y;
  const Icon* icon;
};

// proto 0 = raw (a = Frequenz, off/len in IR_RAW)
struct IrSig {
  uint16_t name;
  uint8_t proto;
  uint32_t a, b;
  uint32_t off;
  uint16_t len;
};

struct UniRemote {
  const char* title;
  int16_t tx, ty;
  const char* db;
  const PanelItem* items;
  uint8_t itemCount;
  const PanelIcon* icons;
  uint8_t iconCount;
  const char* const* names;
  const IrSig* sigs;
  uint16_t sigCount;
};
