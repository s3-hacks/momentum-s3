// Infrared-App: Universal Remotes (Momentum-Layouts + Datenbanken), Lernen, gespeicherte Fernbedienungen
#pragma once
#include "views.h"
#include "ir.h"

// Momentum infrared_brute_force: alle Signale mit passendem Namen senden
static bool bruteForce(const UniRemote& u, const char* btn) {
  int idx = -1;
  for (int i = 0; u.names[i]; i++) {
    if (!strcmp(u.names[i], btn)) { idx = i; break; }
  }
  int total = 0;
  for (int i = 0; i < u.sigCount; i++) if (u.sigs[i].name == idx) total++;
  if (idx < 0 || total == 0) { return false; }
  int done = 0;
  irBegin();
  waitRelease();
  for (int i = 0; i < u.sigCount; i++) {
    if (u.sigs[i].name != idx) continue;
    Key k = readKey();
    if (k == KeyBack || k == KeyOk) break;
    keepAwake();
    // Fortschritt (Hochformat 64x128)
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStrAligned(32, 40, AlignCenter, AlignCenter, T("Sending","Sende"));
    cv.setFont(FontSecondary);
    cv.drawStrAligned(32, 54, AlignCenter, AlignCenter, btn);
    el_progress(4, 64, 56, (float)done / total);
    char b[16]; snprintf(b, sizeof b, "%d/%d", done, total);
    cv.drawStrAligned(32, 84, AlignCenter, AlignCenter, b);
    cv.drawStrAligned(32, 104, AlignCenter, AlignCenter, T("Hold M5","M5 halten"));
    cv.drawStrAligned(32, 114, AlignCenter, AlignCenter, T("to stop","zum Stop"));
    cv.flush();
    irSendDb(u.sigs[i]);
    done++;
    delay(30);
  }
  addXp(1);
  return true;
}

static void universalPanel(const UniRemote& u) {
  cv.setPortrait(true);
  int sel = 0;
  waitRelease();
  while (true) {
    Key k = readKey();
    if (k == KeyDown) { sel = (sel + 1) % u.itemCount; beep(3000, 5); }
    if (k == KeyUp) { sel = (sel + u.itemCount - 1) % u.itemCount; beep(3000, 5); }
    if (k == KeyBack) break;
    if (k == KeyOk) { beep(); if (!bruteForce(u, u.items[sel].name)) {} }
    cv.clear();
    cv.setFont(FontPrimary);
    cv.drawStr(u.tx, u.ty, u.title);
    for (int i = 0; i < u.iconCount; i++) if (u.icons[i].icon) cv.drawIcon(u.icons[i].x, u.icons[i].y, u.icons[i].icon);
    for (int i = 0; i < u.itemCount; i++) {
      const PanelItem& it = u.items[i];
      cv.drawIcon(it.x, it.y, i == sel ? it.hover : it.icon);
    }
    cv.flush();
    delay(20);
  }
  cv.setPortrait(false);
}

static void universalRemotes() {
  std::vector<String> items;
  for (int i = 0; i < UNI_REMOTE_COUNT; i++) items.push_back(UNI_REMOTES[i].title);
  int sel = 0;
  while (true) {
    sel = submenu(nullptr, items, sel);
    if (sel < 0) return;
    universalPanel(UNI_REMOTES[sel]);
  }
}

static String irDir() { if (!LittleFS.exists("/infrared")) LittleFS.mkdir("/infrared"); return "/infrared"; }
static std::vector<String> listFiles(const char* dir, const char* ext) {
  std::vector<String> out;
  File d = LittleFS.open(dir);
  if (!d) return out;
  File f = d.openNextFile();
  while (f) {
    String n = f.name();
    if (!f.isDirectory() && n.endsWith(ext)) out.push_back(n.substring(0, n.length() - strlen(ext)));
    f = d.openNextFile();
  }
  std::sort(out.begin(), out.end());
  return out;
}

// Signal senden mit kurzer "Sending"-Anzeige
static void sendWithUi(const IrFileSig& s) {
  irBegin();
  cv.clear();
  cv.drawIcon(0, 0, &I_InfraredLearnShort_128x31);
  cv.setFont(FontPrimary);
  cv.drawStrAligned(64, 44, AlignCenter, AlignCenter, T("Sending...","Sende..."));
  cv.setFont(FontSecondary);
  cv.drawStrAligned(64, 56, AlignCenter, AlignCenter, s.name.c_str());
  cv.flush();
  irSendFileSig(s);
  addXp(1);
}

// Lernen: wartet auf ein Signal; true wenn erhalten
static bool learnSignal(IrFileSig& out) {
  irRecvStart();
  waitRelease();
  bool got = false;
  while (true) {
    Key k = readKey();
    if (k == KeyBack) break;
    if (irRecvPoll(out)) { got = true; beep(2600, 40); break; }
    keepAwake();
    cv.clear();
    cv.drawIcon(0, 0, &I_InfraredLearnShort_128x31);
    cv.setFont(FontPrimary);
    cv.drawStrAligned(64, 38, AlignCenter, AlignCenter, T("Learning...","Lerne..."));
    cv.setFont(FontSecondary);
    cv.drawStrAligned(64, 50, AlignCenter, AlignCenter, T("Point the remote at IR port","Fernbedienung auf IR-Port"));
    cv.drawStrAligned(64, 60, AlignCenter, AlignCenter, T("and push the button","und Taste druecken"));
    if ((millis() / 500) % 2) cv.drawDisc(120, 40, 2);
    cv.flush();
    delay(20);
  }
  irRecvStop();
  return got;
}

static void saveLearned(IrFileSig sig, const char* remoteName = nullptr) {
  char rname[24] = "";
  if (remoteName) strlcpy(rname, remoteName, sizeof rname);
  else {
    auto files = listFiles(irDir().c_str(), ".ir");
    std::vector<String> opts = {T("+ New remote","+ Neue Fernbed.")};
    for (auto& f : files) opts.push_back(f);
    int c = submenu(T("Save to","Speichern unter"), opts);
    if (c < 0) return;
    if (c == 0) { snprintf(rname, sizeof rname, "Remote%d", (int)files.size() + 1); if (!textInput(T("Name the remote","Name der Fernbed."), rname, 20)) return; }
    else strlcpy(rname, opts[c].c_str(), sizeof rname);
  }
  char bname[24] = "";
  std::vector<IrFileSig> sigs;
  String path = irDir() + "/" + rname + ".ir";
  irLoadFile(path.c_str(), sigs);
  snprintf(bname, sizeof bname, "Button%d", (int)sigs.size() + 1);
  if (!textInput(T("Name the signal","Name des Signals"), bname, 20)) return;
  sig.name = bname;
  sigs.push_back(sig);
  if (irSaveFile(path.c_str(), sigs)) { addXp(3); popup(T("Saved!","Gespeichert!"), rname, &I_DolphinSaved_92x58, 0, 6, 1200); }
  else popup(T("Error","Fehler"), T("Could not save","Speichern fehlgeschlagen"));
}

static void learnNew(const char* remoteName = nullptr) {
  while (true) {
    IrFileSig sig;
    if (!learnSignal(sig)) return;
    while (true) {
      char info[40]; snprintf(info, sizeof info, T("RAW, %d samples","RAW, %d Werte"), (int)sig.raw.size());
      int c = submenu(info, {T("Send","Senden"), T("Save","Speichern"), T("Retry","Nochmal")});
      if (c < 0) return;
      if (c == 0) { sig.name = "Learned"; sendWithUi(sig); continue; }
      if (c == 1) { saveLearned(sig, remoteName); return; }
      if (c == 2) break;
    }
  }
}

static void openRemote(const String& name) {
  String path = irDir() + "/" + name + ".ir";
  int sel = 0;
  while (true) {
    std::vector<IrFileSig> sigs;
    irLoadFile(path.c_str(), sigs);
    std::vector<String> items;
    for (auto& s : sigs) items.push_back(s.name);
    items.push_back(T("+ Learn new button","+ Taste lernen"));
    items.push_back(T("Delete remote","Fernbed. loeschen"));
    sel = submenu(name.c_str(), items, sel);
    if (sel < 0) return;
    if (sel < (int)sigs.size()) { sendWithUi(sigs[sel]); continue; }
    if (sel == (int)sigs.size()) { learnNew(name.c_str()); continue; }
    if (dialogYesNo(T("Delete remote?","Loeschen?"), name.c_str(), T("Cancel","Abbrechen"), T("Delete","Loeschen"))) { LittleFS.remove(path); return; }
  }
}

static void savedRemotes() {
  while (true) {
    auto files = listFiles(irDir().c_str(), ".ir");
    if (files.empty()) { popup(T("No saved remotes","Keine gespeichert"), T("Learn one first or\nupload .ir via Web Files", "Erst lernen oder\n.ir per Web Files laden"), nullptr, 0, 0, 2500); return; }
    int c = submenu(T("Saved Remotes","Gespeicherte"), files);
    if (c < 0) return;
    openRemote(files[c]);
  }
}

static void appInfrared() {
  int sel = 0;
  while (true) {
    sel = submenu(nullptr, {T("Universal Remotes","Universal-Fernbed."), T("Learn New Remote","Neue lernen"), T("Saved Remotes","Gespeicherte"), T("Signal Repeat","Signal-Wiederhol.")}, sel);
    if (sel < 0) return;
    if (sel == 0) universalRemotes();
    if (sel == 1) learnNew();
    if (sel == 2) savedRemotes();
    if (sel == 3) {
      std::vector<VarItem> v = {{T("Repeat each signal","Signal wiederholen"), [] { return String(S.irRepeat) + "x"; }, [](int) { S.irRepeat = S.irRepeat % 3 + 1; }}};
      varItemList("IR", v);
    }
  }
}
