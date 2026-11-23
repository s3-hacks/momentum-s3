// Tasten: M5 (vorne) kurz = OK, M5 lang = Zurueck, Seite kurz = Runter, Seite lang = Hoch (wiederholt)
#pragma once
#include <M5Unified.h>
#include <Preferences.h>

enum Key { KeyNone, KeyUp, KeyDown, KeyOk, KeyBack };

struct Settings {
  int theme = 0;
  int brightness = 3;        // 1..5
  int timeoutIdx = 2;        // 0=aus,1=15s,2=30s,3=60s,4=120s
  bool cycleAnims = true;
  int animSpeed = 2;         // 1..4 (2 = normal)
  int levelOverride = 0;     // 0 = echtes Level
  bool unlockAll = false;
  bool sound = true;
  bool showBatPct = true;
  int irRepeat = 1;          // Wiederholungen pro Signal
  int kbLayout = -1;         // -1 = en-US
  bool kbBle = false;
  int menuStyle = 0;         // 0 = DSi-Karussell, 1 = Liste
  int lang = 0;              // 0 = English, 1 = Deutsch
  char name[16] = "Stick";
  uint32_t xp = 0;
};
static Settings S;
static Preferences prefs;

static void settingsLoad() {
  prefs.begin("mntm", true);
  S.theme = prefs.getInt("theme", 0);
  S.brightness = prefs.getInt("bright", 3);
  S.timeoutIdx = prefs.getInt("tmo", 2);
  S.cycleAnims = prefs.getBool("cycle", true);
  S.animSpeed = prefs.getInt("aspd", 2);
  S.levelOverride = prefs.getInt("lvlo", 0);
  S.unlockAll = prefs.getBool("unlock", false);
  S.sound = prefs.getBool("snd", true);
  S.showBatPct = prefs.getBool("batp", true);
  S.irRepeat = prefs.getInt("irrep", 1);
  S.kbLayout = prefs.getInt("kbl", -1);
  S.kbBle = prefs.getBool("kbble", false);
  S.menuStyle = prefs.getInt("mstyle", 0);
  S.lang = prefs.getInt("lang", 0);
  prefs.getString("name", S.name, sizeof(S.name));
  if (!S.name[0]) strcpy(S.name, "Stick");
  S.xp = prefs.getUInt("xp", 0);
  prefs.end();
}
static void settingsSave() {
  prefs.begin("mntm", false);
  prefs.putInt("theme", S.theme); prefs.putInt("bright", S.brightness); prefs.putInt("tmo", S.timeoutIdx);
  prefs.putBool("cycle", S.cycleAnims); prefs.putInt("aspd", S.animSpeed); prefs.putInt("lvlo", S.levelOverride);
  prefs.putBool("unlock", S.unlockAll); prefs.putBool("snd", S.sound); prefs.putBool("batp", S.showBatPct);
  prefs.putInt("irrep", S.irRepeat); prefs.putInt("kbl", S.kbLayout); prefs.putBool("kbble", S.kbBle);
  prefs.putInt("mstyle", S.menuStyle);
  prefs.putInt("lang", S.lang);
  prefs.putString("name", S.name); prefs.putUInt("xp", S.xp);
  prefs.end();
}

// Uebersetzungs-Helfer: Englisch (Standard) oder Deutsch je nach S.lang
static inline const char* T(const char* en, const char* de) { return S.lang ? de : en; }

static const uint8_t BRIGHT_LV[] = {0, 20, 50, 90, 150, 255};
static const uint32_t TIMEOUT_MS[] = {0, 15000, 30000, 60000, 120000};

static uint32_t lastInput = 0;
static bool screenOff = false;
static uint32_t bHoldRepeat = 0;

static void beep(int f = 2000, int ms = 15) {
  if (S.sound) M5.Speaker.tone(f, ms);
}
static void applyBrightness() { M5.Display.setBrightness(BRIGHT_LV[constrain(S.brightness, 1, 5)]); }

static void wakeScreen() {
  lastInput = millis();
  if (screenOff) { screenOff = false; applyBrightness(); }
}
static void keepAwake() { lastInput = millis(); }

// liefert ein Tasten-Ereignis (nicht blockierend)
static Key readKey() {
  M5.update();
  Key k = KeyNone;
  // Beide Tasten lang halten = Zurueck (kein Dauer-Durchblaettern mehr)
  if (M5.BtnA.wasHold() || M5.BtnB.wasHold()) k = KeyBack;
  else if (M5.BtnA.wasClicked()) k = KeyOk;
  else if (M5.BtnB.wasClicked()) k = KeyDown;

  if (k != KeyNone) {
    if (screenOff) { wakeScreen(); return KeyNone; }  // erster Druck weckt nur
    lastInput = millis();
  } else if (M5.BtnA.isPressed() || M5.BtnB.isPressed()) {
    if (screenOff) wakeScreen();
  }
  uint32_t to = TIMEOUT_MS[constrain(S.timeoutIdx, 0, 4)];
  if (to && !screenOff && millis() - lastInput > to) { screenOff = true; M5.Display.setBrightness(0); }
  return k;
}
static void waitRelease() {
  while (M5.BtnA.isPressed() || M5.BtnB.isPressed()) { M5.update(); delay(5); }
}
