// Desktop mit Momentum-Delfin-Animationen (bubble_animation_view) + Passport
#pragma once
#include "views.h"
#include "gen_dolphin.h"

struct AnimPlayer {
  const DolphinAnim* a = nullptr;
  int frame = 0;            // 0..passive-1 passiv, danach aktiv
  bool active = false;
  int activeLeft = 0;
  uint32_t nextFrame = 0, started = 0, cooldownUntil = 0;
  const Bubble* bubble = nullptr;
  int lastIdx = -1;

  bool allowed(const DolphinAnim& d) {
    if (S.unlockAll) return true;
    int lv = curLevel();
    return lv >= d.minLevel && lv <= d.maxLevel;
  }
  const Bubble* pickBubble(bool act) {
    if (!a || !a->bubbleCount) return nullptr;
    // Blasen-Sequenzen (gleicher Slot) passend zu passiv/aktiv sammeln
    std::vector<int> firsts;
    int lastSlot = -1;
    for (int i = 0; i < a->bubbleCount; i++) {
      if (a->bubbles[i].slot != lastSlot) {
        lastSlot = a->bubbles[i].slot;
        bool isActive = a->bubbles[i].start >= a->passive;
        if (isActive == act) firsts.push_back(i);
      }
    }
    if (firsts.empty()) return nullptr;
    return &a->bubbles[firsts[random(firsts.size())]];
  }
  void pick(bool next = false) {
    int tot = 0;
    for (int i = 0; i < DOLPHIN_ANIM_COUNT; i++) if (allowed(DOLPHIN_ANIMS[i]) && (!next || i != lastIdx)) tot += DOLPHIN_ANIMS[i].weight;
    int idx = 0;
    if (tot > 0) {
      int r = random(tot);
      for (int i = 0; i < DOLPHIN_ANIM_COUNT; i++) {
        if (!allowed(DOLPHIN_ANIMS[i]) || (next && i == lastIdx)) continue;
        r -= DOLPHIN_ANIMS[i].weight;
        if (r < 0) { idx = i; break; }
      }
    }
    lastIdx = idx;
    a = &DOLPHIN_ANIMS[idx];
    frame = 0; active = false; started = millis(); nextFrame = 0;
    bubble = pickBubble(false);
    cooldownUntil = 0;
  }
  uint32_t frameMs() {
    int fps = a->fps ? a->fps : 2;
    static const float SP[] = {1, 0.5f, 1, 1.5f, 2.5f};
    float sp = SP[constrain(S.animSpeed, 1, 4)];
    return (uint32_t)(1000.0f / (fps * sp));
  }
  void activate() {
    if (!a || a->active == 0 || active) return;
    if (millis() < cooldownUntil) return;
    active = true;
    activeLeft = a->active * max(1, (int)a->activeCycles);
    frame = a->passive;
    bubble = pickBubble(true);
    nextFrame = millis() + frameMs();
  }
  // Bubble-Sequenz: naechste Blase im gleichen Slot, wenn Frame darueber hinaus ist
  void advanceBubble() {
    if (!bubble) return;
    while (frame > bubble->end) {
      const Bubble* nx = bubble + 1;
      if (nx < a->bubbles + a->bubbleCount && nx->slot == bubble->slot) bubble = nx;
      else { bubble = nullptr; break; }
    }
  }
  void tick() {
    if (!a) pick();
    if (millis() < nextFrame) return;
    nextFrame = millis() + frameMs();
    if (!active) {
      frame++;
      if (frame >= a->passive) {
        frame = 0;
        if (bubble && bubble->start < a->passive) bubble = pickBubble(false);
      }
      if (S.cycleAnims && millis() - started > (uint32_t)a->duration * 1000UL) pick(true);
    } else {
      frame++;
      activeLeft--;
      if (activeLeft <= 0) {
        active = false; frame = 0;
        cooldownUntil = millis() + a->activeCooldown * 1000UL;
        bubble = pickBubble(false);
      }
    }
    advanceBubble();
  }
  void draw() {
    if (!a) return;
    int oi;
    if (frame < a->passive) oi = frame;
    else oi = (frame - a->passive) % max(1, (int)a->active) + a->passive;
    if (oi >= a->orderLen) oi = a->orderLen - 1;
    int fi = a->order[oi];
    cv.setColor(ColorBlack);
    cv.drawXbm(0, 64 - a->h, a->w, a->h, a->frames[fi]);
    if (bubble && frame >= bubble->start && frame <= bubble->end) {
      cv.setFont(FontSecondary);
      Align ah = bubble->alignH == 0 ? AlignLeft : bubble->alignH == 1 ? AlignCenter : AlignRight;
      Align av = bubble->alignV == 0 ? AlignTop : bubble->alignV == 1 ? AlignCenter : AlignBottom;
      el_bubble_str(bubble->x, bubble->y, bubble->text, ah, av);
    }
  }
};
static AnimPlayer anim;

static void passport() {
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k != KeyNone) return;
    int lv = curLevel();
    uint32_t xp = S.xp;
    cv.clear();
    cv.drawIcon(0, 0, &I_passport_128x64);
    cv.drawIcon(11, 2, &I_passport_happy_46x49);
    cv.setFont(FontSecondary);
    cv.drawStr(59, 10, S.name);
    cv.drawStr(59, 22, T("Mood: Happy","Laune: Gut"));
    char b[24]; snprintf(b, sizeof b, "Level: %d", lv);
    cv.drawStr(59, 34, b);
    int prog = 0;
    if (lv > DOLPHIN_LEVEL_COUNT) snprintf(b, sizeof b, T("Max Level!","Max Level!"));
    else {
      uint32_t lo = lv >= 2 ? DOLPHIN_LEVELS[lv - 2] : 0, hi = DOLPHIN_LEVELS[lv - 1];
      uint32_t have = xp > lo ? xp - lo : 0, target = hi - lo;
      if (S.levelOverride) { have = target; }
      snprintf(b, sizeof b, "%lu/%lu", (unsigned long)have, (unsigned long)target);
      prog = (target - min(have, target)) * 64 / target;
    }
    cv.setFont(FontBatteryPercent);
    cv.drawStr(59, 42, b);
    cv.setColor(ColorWhite);
    cv.drawBox(123 - prog, 45, prog + (prog > 0), 5);
    cv.setColor(ColorBlack);
    cv.flush();
    delay(40);
  }
}
