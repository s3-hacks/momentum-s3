// DuckyScript-Interpreter, portiert aus Momentum applications/main/bad_usb/helpers/ducky_script*.c
#pragma once
#include "hid.h"
#include "gen_badusb.h"


#define HID_KEYBOARD_NONE 0
#define HID_MOUSE_NONE 0
#define HID_MOUSE_INVALID 0
#define HID_CONSUMER_UNASSIGNED 0
#define HID_KEYBOARD_RETURN 0x28
#define HID_KEYBOARD_PRINT_SCREEN 0x46
#define HID_KEYBOARD_LOCK_NUM_LOCK 0x53
#define HID_CONSUMER_FN_GLOBE 0x29D

#define SCRIPT_STATE_ERROR (-1)
#define SCRIPT_STATE_END (-2)
#define SCRIPT_STATE_NEXT_LINE (-3)
#define SCRIPT_STATE_CMD_UNKNOWN (-4)
#define SCRIPT_STATE_STRING_START (-5)
#define SCRIPT_STATE_WAIT_FOR_BTN (-6)

struct DuckyKey { const char* name; uint16_t keycode; };
static const DuckyKey ducky_modifier_keys[] = {
  {"CTRL", KEY_MOD_LEFT_CTRL}, {"CONTROL", KEY_MOD_LEFT_CTRL}, {"SHIFT", KEY_MOD_LEFT_SHIFT},
  {"ALT", KEY_MOD_LEFT_ALT},   {"GUI", KEY_MOD_LEFT_GUI},      {"WINDOWS", KEY_MOD_LEFT_GUI},
};
static const DuckyKey ducky_keys[] = {
  {"DOWNARROW", 0x51}, {"DOWN", 0x51}, {"LEFTARROW", 0x50}, {"LEFT", 0x50}, {"RIGHTARROW", 0x4F}, {"RIGHT", 0x4F},
  {"UPARROW", 0x52}, {"UP", 0x52}, {"ENTER", 0x28}, {"BREAK", 0x48}, {"PAUSE", 0x48}, {"CAPSLOCK", 0x39},
  {"DELETE", 0x4C}, {"BACKSPACE", 0x2A}, {"END", 0x4D}, {"ESC", 0x29}, {"ESCAPE", 0x29}, {"HOME", 0x4A},
  {"INSERT", 0x49}, {"NUMLOCK", 0x53}, {"PAGEUP", 0x4B}, {"PAGEDOWN", 0x4E}, {"PRINTSCREEN", 0x46},
  {"SCROLLLOCK", 0x47}, {"SPACE", 0x2C}, {"TAB", 0x2B}, {"MENU", 0x65}, {"APP", 0x65},
  {"F1", 0x3A}, {"F2", 0x3B}, {"F3", 0x3C}, {"F4", 0x3D}, {"F5", 0x3E}, {"F6", 0x3F}, {"F7", 0x40}, {"F8", 0x41},
  {"F9", 0x42}, {"F10", 0x43}, {"F11", 0x44}, {"F12", 0x45}, {"F13", 0x68}, {"F14", 0x69}, {"F15", 0x6A},
  {"F16", 0x6B}, {"F17", 0x6C}, {"F18", 0x6D}, {"F19", 0x6E}, {"F20", 0x6F}, {"F21", 0x70}, {"F22", 0x71},
  {"F23", 0x72}, {"F24", 0x73},
};
static const DuckyKey ducky_media_keys[] = {
  {"POWER", 0x30}, {"REBOOT", 0x31}, {"SLEEP", 0x32}, {"LOGOFF", 0x19C}, {"EXIT", 0x204}, {"HOME", 0x223},
  {"BACK", 0x224}, {"FORWARD", 0x225}, {"REFRESH", 0x227}, {"SNAPSHOT", 0x65}, {"PLAY", 0xB0}, {"PAUSE", 0xB1},
  {"PLAY_PAUSE", 0xCD}, {"NEXT_TRACK", 0xB5}, {"PREV_TRACK", 0xB6}, {"STOP", 0xB7}, {"EJECT", 0xB8},
  {"MUTE", 0xE2}, {"VOLUME_UP", 0xE9}, {"VOLUME_DOWN", 0xEA}, {"FN", 0x29D}, {"BRIGHT_UP", 0x6F},
  {"BRIGHT_DOWN", 0x70},
};
static const DuckyKey ducky_mouse_keys[] = {
  {"LEFTCLICK", 1}, {"LEFT_CLICK", 1}, {"RIGHTCLICK", 2}, {"RIGHT_CLICK", 2},
  {"MIDDLECLICK", 4}, {"MIDDLE_CLICK", 4}, {"WHEELCLICK", 4}, {"WHEEL_CLICK", 4},
};
static const uint8_t numpad_keys[10] = {0x62, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61};

struct BadKbScript {
  const char* text = nullptr;   // gesamtes Skript
  String owned;                 // falls aus Datei geladen
  size_t pos = 0;
  uint32_t defdelay = 0, stringdelay = 0, defstringdelay = 0;
  uint16_t layout[128];
  String line, line_prev;
  uint32_t repeat_cnt = 0;
  uint8_t key_hold_nb = 0;
  String string_print;
  size_t string_print_pos = 0;
  int line_cur = 0, line_nb = 0, error_line = 0;
  char error[64] = "";
};
static BadKbScript bk;

#define BADUSB_ASCII_TO_KEY(x) (((uint8_t)(x) < 128) ? (bk.layout[(uint8_t)(x)]) : HID_KEYBOARD_NONE)
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

static void ducky_set_layout(int idx) {
  if (idx < 0 || idx >= KB_LAYOUT_COUNT) {
    for (int i = 0; i < 128; i++) {
      uint8_t v = KeyboardLayout_en_US[i];
      bk.layout[i] = (v & 0x7F) | ((v & 0x80) ? KEY_MOD_LEFT_SHIFT : 0);
      if (v == 0) bk.layout[i] = 0;
    }
  } else {
    memcpy(bk.layout, KB_LAYOUTS[idx].map, sizeof(bk.layout));
  }
}

static bool ducky_is_line_end(const char chr) { return chr == ' ' || chr == '\0' || chr == '\r' || chr == '\n'; }
static uint32_t ducky_get_command_len(const char* line) { const char* s = strchr(line, ' '); return s ? (s - line) : 0; }

static uint16_t ducky_get_next_modifier_keycode_by_name(const char** param) {
  const char* in = *param;
  for (size_t i = 0; i < COUNT_OF(ducky_modifier_keys); i++) {
    size_t l = strlen(ducky_modifier_keys[i].name);
    if (strncmp(in, ducky_modifier_keys[i].name, l) == 0) {
      char nx = in[l];
      if (ducky_is_line_end(nx) || nx == '-') { *param = &in[l]; return ducky_modifier_keys[i].keycode; }
    }
  }
  return HID_KEYBOARD_NONE;
}
static uint16_t lookup(const DuckyKey* t, size_t n, const char* p, uint16_t none) {
  for (size_t i = 0; i < n; i++) {
    size_t l = strlen(t[i].name);
    if (strncmp(p, t[i].name, l) == 0 && ducky_is_line_end(p[l])) return t[i].keycode;
  }
  return none;
}
static uint16_t ducky_get_modifier_keycode_by_name(const char* p) { return lookup(ducky_modifier_keys, COUNT_OF(ducky_modifier_keys), p, 0); }
static uint16_t ducky_get_keycode_by_name(const char* p) { return lookup(ducky_keys, COUNT_OF(ducky_keys), p, 0); }
static uint16_t ducky_get_media_keycode_by_name(const char* p) { return lookup(ducky_media_keys, COUNT_OF(ducky_media_keys), p, 0); }
static uint8_t ducky_get_mouse_keycode_by_name(const char* p) { return lookup(ducky_mouse_keys, COUNT_OF(ducky_mouse_keys), p, 0); }

static uint16_t ducky_get_keycode(const char* param, bool accept_modifiers) {
  uint16_t k = ducky_get_keycode_by_name(param);
  if (k) return k;
  if (accept_modifiers) { k = ducky_get_modifier_keycode_by_name(param); if (k) return k; }
  if (strlen(param) > 0) return BADUSB_ASCII_TO_KEY(param[0]) & 0xFF;
  return 0;
}
static bool ducky_get_number(const char* p, uint32_t* val) {
  char* e; while (*p == ' ') p++;
  if (!isdigit((unsigned char)*p)) return false;
  unsigned long v = strtoul(p, &e, 10);
  *val = v; return true;
}
static bool ducky_get_int(const char* p, int32_t* val) {
  char* e; long v = strtol(p, &e, 10);
  if (e == p) return false; *val = v; return true;
}
static int32_t ducky_error(const char* fmt, const char* a = "") {
  snprintf(bk.error, sizeof bk.error, fmt, a);
  return SCRIPT_STATE_ERROR;
}
static void ducky_numlock_on() {
  if ((hid.leds() & HID_KB_LED_NUM) == 0) { hid.kbPress(HID_KEYBOARD_LOCK_NUM_LOCK); hid.kbRelease(HID_KEYBOARD_LOCK_NUM_LOCK); }
}
static bool ducky_numpad_press(const char num) {
  if (num < '0' || num > '9') return false;
  uint16_t k = numpad_keys[num - '0'];
  hid.kbPress(k); hid.kbRelease(k);
  return true;
}
static bool ducky_altchar(const char* code) {
  uint8_t i = 0; bool st = false;
  hid.kbPress(KEY_MOD_LEFT_ALT);
  while (!ducky_is_line_end(code[i])) { st = ducky_numpad_press(code[i]); if (!st) break; i++; }
  hid.kbRelease(KEY_MOD_LEFT_ALT);
  return st;
}
static bool ducky_altstring(const char* p) {
  bool st = false;
  for (uint32_t i = 0; p[i]; i++) {
    if (p[i] < ' ' || p[i] > '~') continue;
    char t[4]; snprintf(t, 4, "%u", (uint8_t)p[i]);
    st = ducky_altchar(t); if (!st) break;
  }
  return st;
}
static void ducky_type_char(char c) {
  if (c != '\n') {
    uint16_t k = BADUSB_ASCII_TO_KEY(c);
    if (k != HID_KEYBOARD_NONE) { hid.kbPress(k); hid.kbRelease(k); }
  } else { hid.kbPress(HID_KEYBOARD_RETURN); hid.kbRelease(HID_KEYBOARD_RETURN); }
}
static bool ducky_string(const char* p) {
  for (uint32_t i = 0; p[i]; i++) ducky_type_char(p[i]);
  bk.stringdelay = 0;
  return true;
}
static bool ducky_string_next() {  // true = fertig
  if (bk.string_print_pos >= bk.string_print.length()) return true;
  ducky_type_char(bk.string_print[bk.string_print_pos++]);
  return false;
}

// ---------------- Kommandos (ducky_script_commands.c) ----------------
typedef int32_t (*DuckyCmdCallback)(const char* line, int32_t param);
static int32_t f_delay(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1]; uint32_t v = 0;
  if (ducky_get_number(l, &v) && v > 0) return (int32_t)v;
  return ducky_error("Invalid number %s", l);
}
static int32_t f_defdelay(const char* l, int32_t) { l = &l[ducky_get_command_len(l) + 1]; return ducky_get_number(l, &bk.defdelay) ? 0 : ducky_error("Invalid number %s", l); }
static int32_t f_strdelay(const char* l, int32_t) { l = &l[ducky_get_command_len(l) + 1]; return ducky_get_number(l, &bk.stringdelay) ? 0 : ducky_error("Invalid number %s", l); }
static int32_t f_defstrdelay(const char* l, int32_t) { l = &l[ducky_get_command_len(l) + 1]; return ducky_get_number(l, &bk.defstringdelay) ? 0 : ducky_error("Invalid number %s", l); }
static int32_t f_string(const char* l, int32_t param) {
  l = &l[ducky_get_command_len(l) + 1];
  bk.string_print = l;
  if (param == 1) bk.string_print += "\n";
  if (bk.stringdelay == 0 && bk.defstringdelay == 0) { ducky_string(bk.string_print.c_str()); return 0; }
  return SCRIPT_STATE_STRING_START;
}
static int32_t f_repeat(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  if (!ducky_get_number(l, &bk.repeat_cnt) || bk.repeat_cnt == 0) return ducky_error("Invalid number %s", l);
  return 0;
}
static int32_t f_sysrq(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  uint16_t k = ducky_get_keycode(l, false);
  hid.kbPress(KEY_MOD_LEFT_ALT | HID_KEYBOARD_PRINT_SCREEN); hid.kbPress(k); hid.releaseAll();
  return 0;
}
static int32_t f_altchar(const char* l, int32_t) { l = &l[ducky_get_command_len(l) + 1]; ducky_numlock_on(); return ducky_altchar(l) ? 0 : ducky_error("Invalid altchar %s", l); }
static int32_t f_altstring(const char* l, int32_t) { l = &l[ducky_get_command_len(l) + 1]; ducky_numlock_on(); return ducky_altstring(l) ? 0 : ducky_error("Invalid altstring %s", l); }
static int32_t f_hold(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  if (bk.key_hold_nb > 5) return ducky_error("Too many keys are held");
  uint16_t k = ducky_get_mouse_keycode_by_name(l);
  if (k) { bk.key_hold_nb++; hid.mousePress(k); return 0; }
  k = ducky_get_keycode(l, true);
  if (k) { bk.key_hold_nb++; hid.kbPress(k); return 0; }
  return ducky_error("Unknown keycode for %s", l);
}
static int32_t f_release(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  if (bk.key_hold_nb == 0) return ducky_error("No keys are held");
  uint16_t k = ducky_get_mouse_keycode_by_name(l);
  if (k) { bk.key_hold_nb--; hid.mouseRelease(k); return 0; }
  k = ducky_get_keycode(l, true);
  if (k) { bk.key_hold_nb--; hid.kbRelease(k); return 0; }
  return ducky_error("No keycode defined for %s", l);
}
static int32_t f_media(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  uint16_t k = ducky_get_media_keycode_by_name(l);
  if (!k) return ducky_error("No keycode defined for %s", l);
  hid.consumerPress(k); hid.consumerRelease(k);
  return 0;
}
static int32_t f_globe(const char* l, int32_t) {
  l = &l[ducky_get_command_len(l) + 1];
  uint16_t k = ducky_get_keycode(l, false);
  if (!k) return ducky_error("No keycode defined for %s", l);
  hid.consumerPress(HID_CONSUMER_FN_GLOBE); hid.kbPress(k); hid.kbRelease(k); hid.consumerRelease(HID_CONSUMER_FN_GLOBE);
  return 0;
}
static int32_t f_waitbtn(const char*, int32_t) { return SCRIPT_STATE_WAIT_FOR_BTN; }
static int32_t f_mscroll(const char* l, int32_t) {
  l = &l[strcspn(l, " ") + 1]; int32_t d = 0;
  if (!ducky_get_int(l, &d)) return ducky_error("Invalid Number %s", l);
  hid.mouseScroll(d); return 0;
}
static int32_t f_mmove(const char* l, int32_t) {
  l = &l[strcspn(l, " ") + 1]; int32_t x = 0, y = 0;
  if (!ducky_get_int(l, &x)) return ducky_error("Invalid Number %s", l);
  l = &l[strcspn(l, " ") + 1];
  if (!ducky_get_int(l, &y)) return ducky_error("Invalid Number %s", l);
  hid.mouseMove(x, y); return 0;
}
struct DuckyCmd { const char* name; DuckyCmdCallback cb; int32_t param; };
static const DuckyCmd ducky_commands[] = {
  {"REM", nullptr, -1}, {"ID", nullptr, -1}, {"BT_ID", nullptr, -1}, {"BLE_ID", nullptr, -1},
  {"DELAY", f_delay, -1}, {"STRING", f_string, 0}, {"STRINGLN", f_string, 1},
  {"DEFAULT_DELAY", f_defdelay, -1}, {"DEFAULTDELAY", f_defdelay, -1},
  {"STRINGDELAY", f_strdelay, -1}, {"STRING_DELAY", f_strdelay, -1},
  {"DEFAULT_STRING_DELAY", f_defstrdelay, -1}, {"DEFAULTSTRINGDELAY", f_defstrdelay, -1},
  {"REPEAT", f_repeat, -1}, {"SYSRQ", f_sysrq, -1}, {"ALTCHAR", f_altchar, -1},
  {"ALTSTRING", f_altstring, -1}, {"ALTCODE", f_altstring, -1}, {"HOLD", f_hold, -1}, {"RELEASE", f_release, -1},
  {"WAIT_FOR_BUTTON_PRESS", f_waitbtn, -1}, {"MEDIA", f_media, -1}, {"GLOBE", f_globe, -1},
  {"MOUSEMOVE", f_mmove, -1}, {"MOUSE_MOVE", f_mmove, -1}, {"MOUSESCROLL", f_mscroll, -1}, {"MOUSE_SCROLL", f_mscroll, -1},
};
static int32_t ducky_execute_cmd(const char* line) {
  size_t wl = strcspn(line, " ");
  for (size_t i = 0; i < COUNT_OF(ducky_commands); i++) {
    size_t l = strlen(ducky_commands[i].name);
    if (l != wl) continue;
    if (strncmp(line, ducky_commands[i].name, l) == 0) {
      if (!ducky_commands[i].cb) return 0;
      return ducky_commands[i].cb(line, ducky_commands[i].param);
    }
  }
  return SCRIPT_STATE_CMD_UNKNOWN;
}

static int32_t ducky_parse_line(const String& lineS) {
  if (lineS.length() == 0) return SCRIPT_STATE_NEXT_LINE;
  const char* line = lineS.c_str();
  int32_t r = ducky_execute_cmd(line);
  if (r != SCRIPT_STATE_CMD_UNKNOWN) return r;
  uint16_t key = ducky_get_mouse_keycode_by_name(line);
  if (key) { hid.mousePress(key); hid.mouseRelease(key); return 0; }
  uint16_t mods = 0;
  while (true) {
    key = ducky_get_next_modifier_keycode_by_name(&line);
    if (!key) break;
    mods |= key;
    if (*line == ' ' || *line == '-') line++;
  }
  char nc = *line;
  key = mods | ducky_get_keycode(line, false);
  if (key == 0 && nc) ducky_error("No keycode defined for %s", line);
  hid.kbPress(key); hid.kbRelease(key);
  return 0;
}

// ---------------- Ablauf ----------------
static void ducky_load(const char* text) {
  bk.text = text; bk.pos = 0;
  bk.defdelay = bk.stringdelay = bk.defstringdelay = 0;
  bk.line = ""; bk.line_prev = ""; bk.repeat_cnt = 0; bk.key_hold_nb = 0;
  bk.string_print = ""; bk.string_print_pos = 0; bk.line_cur = 0; bk.error[0] = 0; bk.error_line = 0;
  // Zeilen zaehlen
  int n = 0; bool any = false;
  for (const char* p = text; *p; p++) { if (*p == '\n') { if (any) n++; any = false; } else any = true; }
  if (any) n++;
  bk.line_nb = n;
}
static String ducky_first_line() {
  const char* e = strchr(bk.text, '\n');
  String s = e ? String(bk.text).substring(0, e - bk.text) : String(bk.text);
  s.trim();
  return s;
}
static void ducky_rewind() { const char* t = bk.text; ducky_load(t); }

static int32_t ducky_execute_next() {
  int32_t dv = 0;
  if (bk.repeat_cnt > 0) {
    bk.repeat_cnt--;
    dv = ducky_parse_line(bk.line_prev);
    if (dv == SCRIPT_STATE_NEXT_LINE) return 0;
    if (dv == SCRIPT_STATE_STRING_START || dv == SCRIPT_STATE_WAIT_FOR_BTN) return dv;
    if (dv < 0) { bk.error_line = bk.line_cur - 1; return SCRIPT_STATE_ERROR; }
    return dv + bk.defdelay;
  }
  bk.line_prev = bk.line;
  bk.line = "";
  while (true) {
    char c = bk.text[bk.pos];
    if (c == 0) {
      if (bk.line.length() == 0) return SCRIPT_STATE_END;
      c = '\n';
    } else bk.pos++;
    if (c == '\n' && bk.line.length() > 0) {
      bk.line_cur++;
      bk.line.trim();
      dv = ducky_parse_line(bk.line);
      if (dv == SCRIPT_STATE_NEXT_LINE) return 0;
      if (dv == SCRIPT_STATE_STRING_START || dv == SCRIPT_STATE_WAIT_FOR_BTN) return dv;
      if (dv < 0) { bk.error_line = bk.line_cur; return SCRIPT_STATE_ERROR; }
      return dv + bk.defdelay;
    } else if (c != '\n' && c != '\r') {
      bk.line += c;
    } else if (c == '\r') {
    }
  }
}
