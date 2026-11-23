// =====================================================================
//  Momentum S3 - Momentum-Firmware (Flipper Zero) fuer den M5StickS3
//  Portiert/nachgebaut aus Momentum-Firmware-dev (GPL-3.0):
//   - Momentum-Grafiken, Delfin-Animationen, u8g2-Schriften, Menue-Stil
//   - Infrared: Universal Remotes mit den Momentum-IR-Datenbanken
//   - Bad KB: DuckyScript-Interpreter (USB + Bluetooth) + Tastaturlayouts
//  Entfernt (Hardware fehlt): Sub-GHz, NFC, 125kHz RFID, iButton, U2F, GPIO
//  Tasten: M5 kurz = OK, M5 lang = Zurueck, Seite kurz = Runter, Seite lang = Hoch
// =====================================================================
#include <M5Unified.h>
#include <LittleFS.h>
#include <vector>
#include <algorithm>
#include "gui.h"
#include "input.h"
#include "views.h"
#include "desktop.h"
#include "app_infrared.h"
#include "app_badkb.h"
#include "app_tools.h"
#include "app_wireless.h"

#define FW_VERSION "MNTM-S3 1.2"

// ---------------- Momentum-App (Interface-Einstellungen) ----------------
static void appMomentum() {
  std::vector<VarItem> v = {
    {T("Language", "Sprache"), [] { return String(S.lang ? "Deutsch" : "English"); }, [](int) { S.lang ^= 1; }},
    {T("Menu Style", "Menue-Stil"), [] { return String(S.menuStyle == 0 ? "DSi" : T("List", "Liste")); }, [](int) { S.menuStyle ^= 1; }},
    {T("Color Theme", "Farb-Thema"), [] { return String(THEMES[S.theme].name); }, [](int) { S.theme = (S.theme + 1) % THEME_COUNT; cv.setTheme(S.theme); }},
    {T("Cycle Animations", "Animationen wechseln"), [] { return String(S.cycleAnims ? "ON" : "OFF"); }, [](int) { S.cycleAnims = !S.cycleAnims; }},
    {T("Anim Speed", "Anim-Tempo"), [] { const char* n[] = {"", T("Slow", "Langsam"), "Normal", T("Fast", "Schnell"), "Turbo"}; return String(n[constrain(S.animSpeed, 1, 4)]); }, [](int) { S.animSpeed = S.animSpeed % 4 + 1; }},
    {T("Dolphin Level", "Delfin-Level"), [] { return S.levelOverride ? String(S.levelOverride) : String(T("Real (", "Echt (")) + levelFromXp(S.xp) + ")"; }, [](int) { S.levelOverride = (S.levelOverride + 1) % 31; anim.a = nullptr; }},
    {T("Unlock All Anims", "Alle Anims frei"), [] { return String(S.unlockAll ? "ON" : "OFF"); }, [](int) { S.unlockAll = !S.unlockAll; anim.a = nullptr; }},
    {T("Battery Percent", "Akku-Prozent"), [] { return String(S.showBatPct ? "ON" : "OFF"); }, [](int) { S.showBatPct = !S.showBatPct; }},
    {T("Device Name", "Geraete-Name"), [] { return String(S.name); }, [](int) { char b[16]; strlcpy(b, S.name, sizeof b); if (textInput(T("Device name", "Geraete-Name"), b, 12)) strlcpy(S.name, b, sizeof S.name); }},
  };
  varItemList("Momentum", v);
}

// ---------------- Settings ----------------
static void aboutScreen() {
  int page = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyBack) return;
    if (k == KeyOk || k == KeyDown) page = (page + 1) % 2;
    cv.clear();
    cv.setFont(FontPrimary);
    if (page == 0) {
      cv.drawStr(2, 10, "Momentum S3");
      cv.setFont(FontSecondary);
      cv.drawStr(2, 22, FW_VERSION);
      cv.drawStr(2, 32, T("Based on Momentum Firmware", "Basiert auf Momentum"));
      cv.drawStr(2, 42, "(Flipper Zero, GPL-3.0)");
      cv.drawStr(2, 52, T("Port for M5StickS3", "Port fuer M5StickS3"));
    } else {
      cv.drawStr(2, 10, T("Device", "Geraet"));
      cv.setFont(FontSecondary);
      char b[40];
      snprintf(b, sizeof b, "Battery: %d%%  %.2fV", (int)M5.Power.getBatteryLevel(), M5.Power.getBatteryVoltage() / 1000.0f); cv.drawStr(2, 22, b);
      snprintf(b, sizeof b, "Heap: %u KB  PSRAM: %u KB", ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024); cv.drawStr(2, 32, b);
      snprintf(b, sizeof b, T("Storage: %u / %u KB", "Speicher: %u / %u KB"), LittleFS.usedBytes() / 1024, LittleFS.totalBytes() / 1024); cv.drawStr(2, 42, b);
      snprintf(b, sizeof b, "XP: %u  Level: %d", (unsigned)S.xp, levelFromXp(S.xp)); cv.drawStr(2, 52, b);
    }
    el_button_center(T("More", "Mehr"));
    cv.flush();
    delay(50);
  }
}

static void appSettings() {
  int sel = 0;
  while (true) {
    sel = submenu(T("Settings", "Einstellungen"), {T("Display & Sound", "Anzeige & Ton"), "Passport", T("Storage", "Speicher"), T("About", "Ueber"), T("Restart", "Neustart")}, sel);
    if (sel < 0) return;
    if (sel == 0) {
      std::vector<VarItem> v = {
        {T("Brightness", "Helligkeit"), [] { return String(S.brightness * 20) + "%"; }, [](int) { S.brightness = S.brightness % 5 + 1; applyBrightness(); }},
        {T("Screen Timeout", "Bildschirm aus"), [] { const char* n[] = {T("Never", "Nie"), "15s", "30s", "60s", "120s"}; return String(n[constrain(S.timeoutIdx, 0, 4)]); }, [](int) { S.timeoutIdx = (S.timeoutIdx + 1) % 5; }},
        {T("Sound", "Ton"), [] { return String(S.sound ? "ON" : "OFF"); }, [](int) { S.sound = !S.sound; }},
      };
      varItemList(T("Display", "Anzeige"), v);
    }
    if (sel == 1) passport();
    if (sel == 2) {
      char b[48]; snprintf(b, sizeof b, T("Used %u of %u KB", "%u von %u KB belegt"), LittleFS.usedBytes() / 1024, LittleFS.totalBytes() / 1024);
      int c = submenu(b, {T("Format storage", "Speicher formatieren"), T("Reset XP / Level", "XP / Level zuruecksetzen")});
      if (c == 0 && dialogYesNo(T("Format storage?", "Formatieren?"), T("Deletes all saved remotes\nand Bad KB scripts.", "Loescht alle Fernbedienungen\nund Bad-KB-Skripte."), T("Cancel", "Abbrechen"), T("Format", "Formatieren"))) {
        LittleFS.format(); popup(T("Done", "Fertig"), T("Storage formatted", "Speicher formatiert"));
      }
      if (c == 1 && dialogYesNo(T("Reset XP?", "XP zuruecksetzen?"), T("Dolphin goes back\nto level 1.", "Delfin faellt zurueck\nauf Level 1."), T("Cancel", "Abbrechen"), T("Reset", "Reset"))) { S.xp = 0; settingsSave(); anim.a = nullptr; }
    }
    if (sel == 3) aboutScreen();
    if (sel == 4 && dialogYesNo(T("Restart?", "Neustart?"), T("The stick restarts now.", "Der Stick startet neu."), T("Cancel", "Abbrechen"), T("Restart", "Neustart"))) ESP.restart();
  }
}

// ---------------- Hauptmenue (Momentum "List"-Stil) ----------------
struct MainItem { const char* name; const Icon* icon; void (*fn)(); };
static const MainItem MAIN_ITEMS[] = {
  {"Infrared", &A_Infrared_14, appInfrared},
  {"Bad KB", &A_BadUsb_14, appBadKb},
  {"Apps", &A_Plugins_14, appApps},
  {"Momentum", &A_Momentum_14, appMomentum},
  {"Settings", &A_Settings_14, appSettings},
};
static const int MAIN_COUNT = sizeof(MAIN_ITEMS) / sizeof(MAIN_ITEMS[0]);
static int mainPos = 0;

// Hauptmenue-Namen uebersetzen (Bad KB/Apps/Momentum bleiben gleich)
static const char* mainName(const char* n) {
  if (!strcmp(n, "Infrared")) return T("Infrared", "Infrarot");
  if (!strcmp(n, "Settings")) return T("Settings", "Einstellungen");
  return n;
}

static void mainMenu() {
  uint32_t scrollT = millis();
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyDown) { mainPos = (mainPos + 1) % MAIN_COUNT; scrollT = millis(); beep(3000, 5); }
    if (k == KeyUp) { mainPos = (mainPos + MAIN_COUNT - 1) % MAIN_COUNT; scrollT = millis(); beep(3000, 5); }
    if (k == KeyBack) return;
    if (k == KeyOk) { beep(); MAIN_ITEMS[mainPos].fn(); waitRelease(); scrollT = millis(); }
    cv.clear();
    if (S.menuStyle == 0) {
      // Momentum menu.c MenuStyleDsi: Karussell, Seite = naechstes Icon
      for (int i = -2; i <= 2; i++) {
        int sp = (mainPos + MAIN_COUNT * 2 + i) % MAIN_COUNT;
        const MainItem& it = MAIN_ITEMS[sp];
        int width = 24, height = 26, px = 64, py = 36;
        if (i == 0) {
          width += 6; height += 4;
          el_bold_rounded_frame(px - width / 2, py - height / 2, width, height + 5);
          cv.setFont(FontBatteryPercent);
          cv.drawStrAligned(px - 9, py + height / 2 + 1, AlignCenter, AlignBottom, "S");
          cv.drawStrAligned(px, py + height / 2 + 1, AlignCenter, AlignBottom, "TAR");
          cv.drawStrAligned(px + 9, py + height / 2 + 1, AlignCenter, AlignBottom, "T");
          cv.drawRFrame(0, 0, 128, 18, 3);
          drawMenuBattery(104, 5, false);
          cv.drawLine(60, 18, 64, 26);
          cv.drawLine(64, 26, 68, 18);
          cv.setColor(ColorWhite);
          cv.drawLine(60, 17, 68, 17);
          cv.drawBox(62, 21, 5, 2);
          cv.setColor(ColorBlack);
          cv.setFont(FontPrimary);
          cv.drawStrAligned(px, py - height / 2 - 8, AlignCenter, AlignBottom, mainName(it.name));
        } else {
          px += (width + 6) * i;
          py += 2;
          el_slightly_rounded_frame(px - width / 2, py - height / 2, width, height);
        }
        const Icon* ic = it.icon;
        if (i == 0) cv.drawIconAnim(px - 7, py - 7, ic); else cv.drawIcon(px - 7, py - 7, ic, 0);
      }
      el_scrollbar_horizontal(0, 64, 128, mainPos, MAIN_COUNT);
      cv.flush();
      delay(15);
      continue;
    }
    // Momentum menu.c MenuStyleList
    for (int i = 0; i < 3; i++) {
      cv.setFont(i == 1 ? FontPrimary : FontSecondary);
      int sp = (mainPos + MAIN_COUNT + i - 1) % MAIN_COUNT;
      const MainItem& it = MAIN_ITEMS[sp];
      const Icon* ic = it.icon;
      int ix = 4 + (14 - ic->w) / 2, iy = 3 + 22 * i + (14 - ic->h) / 2;
      if (i == 1) cv.drawIconAnim(ix, iy, ic); else cv.drawIcon(ix, iy, ic, 0);
      cv.drawStr(22, 14 + 22 * i, mainName(it.name));
    }
    el_frame(0, 21, 128 - 5, 21);
    el_scrollbar(mainPos, MAIN_COUNT);
    drawMenuBattery(104, 2, false);
    cv.flush();
    delay(15);
  }
}

// ---------------- Desktop ----------------
static void desktop() {
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyOk) { mainMenu(); waitRelease(); }
    else if (k == KeyDown) { passport(); waitRelease(); }
    else if (k == KeyUp) { anim.pick(true); }
    else if (k == KeyBack) { anim.activate(); }
    anim.tick();
    if (screenOff) { delay(30); continue; }
    cv.clear();
    anim.draw();
    drawStatusBar();
    cv.flush();
    delay(20);
  }
}

static void bootScreen() {
  // Momentum-Bootlogo: Delfin + Name
  uint32_t t0 = millis();
  while (millis() - t0 < 1400) {
    M5.update();
    cv.clear();
    cv.drawIcon(0, 6, &I_DolphinMafia_119x62);
    cv.setColor(ColorWhite); cv.drawBox(70, 2, 58, 12); cv.setColor(ColorBlack);
    cv.setFont(FontPrimary); cv.drawStr(72, 11, "Momentum");
    cv.flush();
    delay(30);
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.BtnA.setHoldThresh(550);
  M5.BtnB.setHoldThresh(450);
  settingsLoad();
  cv.begin();
  cv.setTheme(S.theme);
  applyBrightness();
  M5.Speaker.setVolume(90);
  if (!LittleFS.begin(true)) { /* formatiert beim ersten Start */ }
  randomSeed(esp_random());
  rtcToSystem();
  bootScreen();
  lastInput = millis();
}

void loop() { desktop(); }
