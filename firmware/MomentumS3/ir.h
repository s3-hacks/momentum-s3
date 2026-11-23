// Infrarot: Senden der Momentum/Flipper-Protokolle + .ir-Dateien lesen/schreiben + Lernen
#pragma once
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <LittleFS.h>
#include "gen_ir.h"

#define IR_TX_PIN 46
#define IR_RX_PIN 42

static IRsend irsend(IR_TX_PIN);
static IRrecv* irrecv = nullptr;
static uint16_t irBuf[600];

static const char* PROTO_NAMES[] = {"RAW", "NEC", "NECext", "NEC42", "NEC42ext", "Samsung32", "RC6", "RC5", "RC5X",
                                    "SIRC", "SIRC15", "SIRC20", "Kaseikyo", "RCA", "Pioneer"};
static int protoFromName(const char* n) {
  for (int i = 1; i < 15; i++) if (!strcmp(n, PROTO_NAMES[i])) return i;
  return -1;
}

static void irBegin() {
  M5.Power.setExtOutput(true, m5::ext_none);
  irsend.begin();
}

// Puls-Abstands-Kodierung wie Flipper infrared_common_encoder (LSB zuerst)
static int pdEncode(uint16_t* b, int pos, uint16_t hm, uint16_t hs, uint16_t m1, uint16_t s1, uint16_t m0, uint16_t s0,
                    const uint8_t* data, int nbits, uint16_t stop) {
  b[pos++] = hm; b[pos++] = hs;
  for (int i = 0; i < nbits; i++) {
    bool bit = (data[i / 8] >> (i % 8)) & 1;
    b[pos++] = bit ? m1 : m0;
    b[pos++] = bit ? s1 : s0;
  }
  b[pos++] = stop;
  return pos;
}

// Sendet ein "parsed"-Signal (Adresse/Kommando wie im Flipper-.ir-Format)
static void irSendParsed(int proto, uint32_t address, uint32_t command) {
  uint8_t d[8] = {0};
  int n = 0;
  uint16_t khz = 38000;
  switch (proto) {
    case 1: case 2: case 3: case 4: {  // NEC-Familie
      uint32_t d1 = 0, d2 = 0; int bits = 32;
      if (proto == 1) { uint8_t a = address, c = command; d1 = a | ((uint8_t)~a << 8) | ((uint32_t)c << 16) | ((uint32_t)(uint8_t)~c << 24); }
      else if (proto == 2) { d1 = (uint16_t)address | ((command & 0xFFFF) << 16); }
      else if (proto == 3) { d1 = (address & 0x1FFF) | ((~address & 0x1FFF) << 13) | ((command & 0x3F) << 26);
                             d2 = ((command & 0xC0) >> 6) | ((~command & 0xFF) << 2); bits = 42; }
      else { d1 = (address & 0x3FFFFFF) | ((command & 0x3F) << 26); d2 = (command & 0xFFC0) >> 6; bits = 42; }
      memcpy(d, &d1, 4); memcpy(d + 4, &d2, 4);
      n = pdEncode(irBuf, 0, 9000, 4500, 560, 1690, 560, 560, d, bits, 560);
      // ein Wiederholungs-Code
      irBuf[n++] = 40000; irBuf[n++] = 9000; irBuf[n++] = 2250; irBuf[n++] = 560;
      break;
    }
    case 5: {  // Samsung32
      uint8_t a = address, c = command;
      d[0] = a; d[1] = a; d[2] = c; d[3] = ~c;
      n = pdEncode(irBuf, 0, 4500, 4500, 550, 1650, 550, 550, d, 32, 550);
      irBuf[n++] = 46000;
      n = pdEncode(irBuf, n, 4500, 4500, 550, 1650, 550, 550, d, 32, 550);
      break;
    }
    case 12: {  // Kaseikyo (48 Bit)
      uint8_t id = (address >> 24) & 3; uint16_t vendor = (address >> 8) & 0xFFFF;
      uint8_t g1 = (address >> 4) & 0xF, g2 = address & 0xF;
      d[0] = vendor & 0xFF; d[1] = vendor >> 8;
      uint8_t vp = d[0] ^ d[1]; vp = (vp & 0xF) ^ (vp >> 4);
      d[2] = (vp & 0xF) | (g1 << 4);
      d[3] = (g2 & 0xF) | ((uint8_t)(command & 0xF) << 4);
      d[4] = (id << 6) | (uint8_t)(command >> 4);
      d[5] = d[2] ^ d[3] ^ d[4];
      n = pdEncode(irBuf, 0, 3456, 1728, 432, 1296, 432, 432, d, 48, 432);
      break;
    }
    case 13: {  // RCA (24 Bit)
      uint8_t a = address, c = command;
      uint32_t v = (a & 0xF) | ((uint32_t)c << 4) | ((uint32_t)((uint8_t)~a & 0xF) << 12) | ((uint32_t)(uint8_t)~c << 16);
      memcpy(d, &v, 4);
      n = pdEncode(irBuf, 0, 4000, 4000, 500, 2000, 500, 1000, d, 24, 500);
      irBuf[n++] = 8000;
      n = pdEncode(irBuf, n, 4000, 4000, 500, 2000, 500, 1000, d, 24, 500);
      break;
    }
    case 14: {  // Pioneer (33 Bit, 40 kHz, 2x)
      d[0] = address & 0xFF; d[1] = ~(address & 0xFF); d[2] = command & 0xFF; d[3] = ~(command & 0xFF); d[4] = 0;
      n = pdEncode(irBuf, 0, 8500, 4225, 500, 1500, 500, 500, d, 33, 500);
      irBuf[n++] = 26000;
      n = pdEncode(irBuf, n, 8500, 4225, 500, 1500, 500, 500, d, 33, 500);
      khz = 40000;
      break;
    }
    case 6:  // RC6 Mode 0
      irsend.sendRC6(irsend.encodeRC6(address & 0xFF, command & 0xFF, kRC6Mode0Bits), kRC6Mode0Bits, 1);
      return;
    case 7:  // RC5
      irsend.sendRC5(irsend.encodeRC5(address & 0x1F, command & 0x3F), kRC5Bits, 1);
      return;
    case 8:  // RC5X
      irsend.sendRC5(irsend.encodeRC5X(address & 0x1F, command & 0x7F), kRC5XBits, 1);
      return;
    case 9:
      irsend.sendSony(irsend.encodeSony(12, command & 0x7F, address & 0x1F), 12, 2);
      return;
    case 10:
      irsend.sendSony(irsend.encodeSony(15, command & 0x7F, address & 0xFF), 15, 2);
      return;
    case 11:
      irsend.sendSony(irsend.encodeSony(20, command & 0x7F, address & 0x1F, (address >> 5) & 0xFF), 20, 2);
      return;
    default: return;
  }
  irsend.sendRaw(irBuf, n, khz);
}

static void irSendRaw(const uint16_t* t, int len, uint32_t freq) {
  if (freq < 10000 || freq > 60000) freq = 38000;
  irsend.sendRaw(t, len, freq);
}

// Signal aus der eingebauten Datenbank
static void irSendDb(const IrSig& s) {
  for (int r = 0; r < max(1, S.irRepeat); r++) {
    if (s.proto == 0) {
      static uint16_t tmp[1024];
      int len = min((int)s.len, 1024);
      for (int i = 0; i < len; i++) tmp[i] = pgm_read_word(&IR_RAW[s.off + i]);
      irSendRaw(tmp, len, s.a);
    } else {
      irSendParsed(s.proto, s.a, s.b);
    }
    if (r + 1 < S.irRepeat) delay(40);
  }
}

// ---------------- .ir-Dateien (Flipper-Format) ----------------
struct IrFileSig {
  String name;
  int proto = -1;           // 0 = raw
  uint32_t a = 0, b = 0;    // parsed: address/command ; raw: a = frequenz
  std::vector<uint16_t> raw;
};

static uint32_t parseLE(const String& s) {
  uint32_t v = 0; int i = 0, shift = 0;
  while (i < (int)s.length() && shift < 32) {
    while (i < (int)s.length() && s[i] == ' ') i++;
    if (i + 1 >= (int)s.length() + 1) break;
    String hx = s.substring(i, i + 2);
    if (!hx.length()) break;
    v |= (uint32_t)strtoul(hx.c_str(), nullptr, 16) << shift;
    shift += 8; i += 2;
  }
  return v;
}

static bool irLoadFile(const char* path, std::vector<IrFileSig>& out) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  out.clear();
  IrFileSig* cur = nullptr;
  while (f.available()) {
    String ln = f.readStringUntil('\n'); ln.trim();
    if (!ln.length() || ln[0] == '#') continue;
    int c = ln.indexOf(':'); if (c < 0) continue;
    String k = ln.substring(0, c), v = ln.substring(c + 1); k.trim(); v.trim();
    if (k == "name") { out.emplace_back(); cur = &out.back(); cur->name = v; }
    else if (!cur) continue;
    else if (k == "type") { if (v == "raw") cur->proto = 0; }
    else if (k == "protocol") cur->proto = protoFromName(v.c_str());
    else if (k == "address") cur->a = parseLE(v);
    else if (k == "command") cur->b = parseLE(v);
    else if (k == "frequency") cur->a = v.toInt();
    else if (k == "data") {
      int p = 0;
      while (p < (int)v.length()) {
        int e = v.indexOf(' ', p); if (e < 0) e = v.length();
        long t = v.substring(p, e).toInt();
        if (t > 0) cur->raw.push_back(t > 65535 ? 65535 : t);
        p = e + 1;
      }
    }
  }
  f.close();
  // ungueltige entfernen
  std::vector<IrFileSig> ok;
  for (auto& s : out) if (s.proto > 0 || (s.proto == 0 && s.raw.size())) ok.push_back(s);
  out.swap(ok);
  return true;
}

static void irSendFileSig(const IrFileSig& s) {
  for (int r = 0; r < max(1, S.irRepeat); r++) {
    if (s.proto == 0) irSendRaw(s.raw.data(), s.raw.size(), s.a ? s.a : 38000);
    else irSendParsed(s.proto, s.a, s.b);
    if (r + 1 < S.irRepeat) delay(40);
  }
}

static String hexLE(uint32_t v) {
  char b[16]; snprintf(b, sizeof b, "%02X %02X %02X %02X", v & 255, (v >> 8) & 255, (v >> 16) & 255, (v >> 24) & 255);
  return b;
}
static bool irSaveFile(const char* path, const std::vector<IrFileSig>& sigs) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.print("Filetype: IR signals file\nVersion: 1\n");
  for (auto& s : sigs) {
    f.print("#\nname: "); f.print(s.name); f.print("\n");
    if (s.proto == 0) {
      f.printf("type: raw\nfrequency: %u\nduty_cycle: 0.330000\ndata:", s.a ? s.a : 38000);
      for (auto t : s.raw) f.printf(" %u", t);
      f.print("\n");
    } else {
      f.printf("type: parsed\nprotocol: %s\naddress: %s\ncommand: %s\n", PROTO_NAMES[s.proto], hexLE(s.a).c_str(), hexLE(s.b).c_str());
    }
  }
  f.close();
  return true;
}

// ---------------- Lernen ----------------
static void irRecvStart() {
  if (!irrecv) irrecv = new IRrecv(IR_RX_PIN, 1024, 15, true);
  irrecv->enableIRIn(true);
}
static void irRecvStop() { if (irrecv) irrecv->disableIRIn(); }
// liefert true + raw timings, wenn ein Signal empfangen wurde
static bool irRecvPoll(IrFileSig& out) {
  decode_results res;
  if (!irrecv || !irrecv->decode(&res)) return false;
  bool ok = false;
  if (res.rawlen > 6 && !res.overflow) {
    uint16_t* arr = resultToRawArray(&res);
    uint16_t len = getCorrectedRawLength(&res);
    out.proto = 0; out.a = 38000; out.raw.assign(arr, arr + len);
    delete[] arr;
    ok = len > 4;
  }
  irrecv->resume();
  return ok;
}
