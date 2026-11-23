# Wandelt Momentum-Assets (Icons, Animationen, Fonts, IR-DBs, BadUSB) in C-Header fuer MomentumS3 um.
import os, re, sys, glob
from PIL import Image

SRC = sys.argv[1]   # .../Momentum-Firmware-dev
OUT = sys.argv[2]   # Sketch-Ordner

def cname(s):
    s = re.sub(r'[^A-Za-z0-9_]', '_', s)
    if s[0].isdigit(): s = '_' + s
    return s

def img_bits(path, w=None, h=None):
    im = Image.open(path).convert('RGBA')
    W, H = im.size
    px = im.load()
    bpr = (W + 7) // 8
    out = bytearray(bpr * H)
    for y in range(H):
        for x in range(W):
            r, g, b, a = px[x, y]
            on = a >= 128 and (r + g + b) < 384
            if on:
                out[y * bpr + x // 8] |= (1 << (x & 7))  # XBM: LSB first
    return W, H, bytes(out)

def carr(data, per=24):
    s = []
    for i in range(0, len(data), per):
        s.append(','.join('0x%02x' % b for b in data[i:i + per]))
    return ',\n'.join(s)

# ---------------- Icons ----------------
def gen_icons():
    base = os.path.join(SRC, 'assets', 'icons')
    lines = ['#pragma once', '#include "gfx_types.h"', '']
    names = set()
    for root, dirs, files in os.walk(base):
        frames = sorted(f for f in files if re.match(r'frame_\d+\.png', f))
        if frames:
            nm = cname('A_' + os.path.basename(root))
            fps = 4
            fr = os.path.join(root, 'frame_rate')
            if os.path.exists(fr):
                try: fps = int(open(fr).read().strip())
                except: pass
            ptrs = []
            W = H = 0
            for i, f in enumerate(frames):
                W, H, d = img_bits(os.path.join(root, f))
                lines.append('static const uint8_t %s_f%d[] PROGMEM = {%s};' % (nm, i, carr(d)))
                ptrs.append('%s_f%d' % (nm, i))
            lines.append('static const uint8_t* const %s_frames[] = {%s};' % (nm, ','.join(ptrs)))
            lines.append('static const Icon %s = {%d,%d,%d,%d,%s_frames};' % (nm, W, H, len(frames), fps, nm))
            continue
        for f in files:
            if not f.endswith('.png'): continue
            nm = cname('I_' + f[:-4])
            if nm in names: continue
            names.add(nm)
            W, H, d = img_bits(os.path.join(root, f))
            lines.append('static const uint8_t %s_d[] PROGMEM = {%s};' % (nm, carr(d)))
            lines.append('static const uint8_t* const %s_fr[] = {%s_d};' % (nm, nm))
            lines.append('static const Icon %s = {%d,%d,1,0,%s_fr};' % (nm, W, H, nm))
    open(os.path.join(OUT, 'gen_icons.h'), 'w').write('\n'.join(lines) + '\n')

# ---------------- Dolphin-Animationen ----------------
def parse_kv(text):
    blocks, cur = [], {}
    for ln in text.splitlines():
        if ':' not in ln:
            continue
        k, v = ln.split(':', 1)
        cur.setdefault('_order', []).append((k.strip(), v.strip()))
    return cur.get('_order', [])

def esc(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')

def gen_dolphin():
    base = os.path.join(SRC, 'assets', 'dolphin', 'external')
    man = open(os.path.join(base, 'manifest.txt')).read()
    entries = []
    cur = None
    for k, v in parse_kv(man):
        if k == 'Name':
            cur = {'name': v}; entries.append(cur)
        elif cur is not None:
            cur[k] = v
    lines = ['#pragma once', '#include "gfx_types.h"', '']
    recs = []
    for e in entries:
        d = os.path.join(base, e['name'])
        if not os.path.isdir(d): continue
        kv = parse_kv(open(os.path.join(d, 'meta.txt')).read())
        meta = {}
        bubbles = []
        b = None
        for k, v in kv:
            if k == 'Slot':
                b = {'slot': int(v)}; bubbles.append(b)
            elif b is not None and k in ('X', 'Y', 'Text', 'AlignH', 'AlignV', 'StartFrame', 'EndFrame'):
                b[k] = v
            else:
                meta[k] = v
        W = int(meta['Width']); H = int(meta['Height'])
        order = [int(x) for x in meta['Frames order'].split()]
        nfr = max(order) + 1
        nm = cname('D_' + e['name'])
        ptrs = []
        for i in range(nfr):
            p = os.path.join(d, 'frame_%d.png' % i)
            if not os.path.exists(p):
                p = os.path.join(d, 'frame_%02d.png' % i)
            fw, fh, data = img_bits(p)
            lines.append('static const uint8_t %s_f%d[] PROGMEM = {%s};' % (nm, i, carr(data)))
            ptrs.append('%s_f%d' % (nm, i))
        lines.append('static const uint8_t* const %s_frames[] = {%s};' % (nm, ','.join(ptrs)))
        lines.append('static const uint8_t %s_order[] = {%s};' % (nm, ','.join(map(str, order))))
        bl = []
        for bb in bubbles:
            ah = {'Left': 0, 'Center': 1, 'Right': 2}.get(bb.get('AlignH', 'Left'), 0)
            av = {'Top': 0, 'Center': 1, 'Bottom': 2}.get(bb.get('AlignV', 'Top'), 0)
            txt = bb.get('Text', '').replace('\\n', '\n')
            bl.append('{%d,%d,%d,"%s",%d,%d,%d,%d}' % (bb['slot'], int(bb.get('X', 0)), int(bb.get('Y', 0)),
                      esc(txt).replace('\n', '\\n'), ah, av, int(bb.get('StartFrame', 0)), int(bb.get('EndFrame', 0))))
        if bl:
            lines.append('static const Bubble %s_bub[] = {%s};' % (nm, ','.join(bl)))
        recs.append('{"%s",%d,%d,%s_frames,%s_order,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d}' % (
            e['name'], W, H, nm, nm, len(order), int(meta['Passive frames']), int(meta['Active frames']),
            int(meta['Frame rate']), int(meta['Duration']), int(meta['Active cycles']), int(meta['Active cooldown']),
            int(e.get('Min level', 1)), (nm + '_bub') if bl else 'nullptr', len(bl), int(e.get('Max level', 30)),
            int(e.get('Weight', 3))))
    lines.append('extern const DolphinAnim DOLPHIN_ANIMS[] = {\n' + ',\n'.join(recs) + '};')
    lines.append('extern const int DOLPHIN_ANIM_COUNT = %d;' % len(recs))
    lines[0] = '#include "gen_dolphin.h"'
    open(os.path.join(OUT, 'gen_dolphin.cpp'), 'w').write('\n'.join(lines) + '\n')
    open(os.path.join(OUT, 'gen_dolphin.h'), 'w').write('#pragma once\n#include "gfx_types.h"\nextern const DolphinAnim DOLPHIN_ANIMS[];\nextern const int DOLPHIN_ANIM_COUNT;\n')

# ---------------- Fonts ----------------
def gen_fonts():
    src = open(os.path.join(SRC, 'lib', 'u8g2', 'u8g2_fonts.c'), encoding='latin-1').read()
    lines = ['#pragma once', '#include <stdint.h>', '']
    for f in ['u8g2_font_helvB08_tr', 'u8g2_font_haxrcorp4089_tr', 'u8g2_font_profont11_mr',
              'u8g2_font_profont22_tn', 'u8g2_font_5x7_tr', 'u8g2_font_profont12_tr', 'u8g2_font_helvB10_tr']:
        m = re.search(r'const uint8_t ' + f + r'\[(\d+)\] U8G2_FONT_SECTION\("' + f + r'"\) =\s*\n(.*?);\n', src, re.S)
        if not m:
            print('font missing', f); continue
        body = m.group(2)
        lines.append('static const uint8_t %s[%s] = \n%s;' % (f, m.group(1), body))
    open(os.path.join(OUT, 'gen_fonts.h'), 'w', encoding='latin-1').write('\n'.join(lines) + '\n')

# ---------------- IR ----------------
PROTO = ['NEC', 'NECext', 'NEC42', 'NEC42ext', 'Samsung32', 'RC6', 'RC5', 'RC5X', 'SIRC', 'SIRC15', 'SIRC20',
         'Kaseikyo', 'RCA', 'Pioneer']

def le32(s):
    b = [int(x, 16) for x in s.split()]
    v = 0
    for i, x in enumerate(b): v |= x << (8 * i)
    return v

def parse_ir(text):
    sigs = []
    cur = None
    for ln in text.splitlines():
        ln = ln.strip()
        if not ln or ln.startswith('#') or ':' not in ln: continue
        k, v = ln.split(':', 1); k = k.strip(); v = v.strip()
        if k == 'name':
            cur = {'name': v}; sigs.append(cur)
        elif cur is not None:
            cur[k] = v
    return sigs

def gen_ir():
    base = os.path.join(SRC, 'applications', 'main', 'infrared')
    scenes = os.path.join(base, 'scenes')
    lines = ['#pragma once', '#include "gfx_types.h"', '#include "gen_icons.h"', '']
    remotes = []
    order = ['tv', 'ac', 'audio', 'projector', 'fan', 'leds', 'monitor', 'bluray', 'digital_sign']
    raw_all = []
    for key in order:
        fn = os.path.join(scenes, 'infrared_scene_universal_%s.c' % key)
        if key == 'fan': fn = os.path.join(scenes, 'infrared_scene_universal_fan.c')
        src = open(fn).read()
        # AC: nur metrischen Zweig (Celsius) behalten
        src = re.sub(r'if\(furi_hal_rtc_get_locale_units\(\) == FuriHalRtcLocaleUnitsMetric\) \{(.*?)\} else \{.*?\n    \}',
                     r'\1', src, flags=re.S)
        db = re.search(r'EXT_PATH\("infrared/assets/([^"]+)"\)', src).group(1)
        label = re.search(r'button_panel_add_label\(\s*button_panel,\s*(\d+),\s*(\d+),\s*\w+,\s*"([^"]+)"\)', src)
        items = re.findall(r'button_panel_add_item\(\s*button_panel,\s*i,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*&(\w+),\s*&(\w+),', src)
        recs = re.findall(r'infrared_brute_force_add_record\(brute_force,\s*i\+\+,\s*"([^"]+)"\)', src)
        icons = re.findall(r'button_panel_add_icon\(\s*button_panel,\s*(\d+),\s*(\d+),\s*&(\w+)\)', src)
        if len(items) != len(recs):
            print('WARN', key, len(items), len(recs))
        nm = cname('U_' + key)
        it = ['{%s,%s,&%s,&%s,"%s"}' % (x, y, ic, ich, r) for (c, rw, x, y, ic, ich), r in zip(items, recs)]
        ics = ['{%s,%s,&%s}' % (x, y, ic) for x, y, ic in icons]
        lines.append('static const PanelItem %s_items[] = {%s};' % (nm, ','.join(it)))
        lines.append('static const PanelIcon %s_icons[] = {%s};' % (nm, ','.join(ics) if ics else '{0,0,nullptr}'))
        # DB
        sigs = parse_ir(open(os.path.join(base, 'resources', 'infrared', 'assets', db)).read())
        names = []
        sl = []
        for s in sigs:
            if s['name'] not in names: names.append(s['name'])
            ni = names.index(s['name'])
            if s.get('type') == 'parsed':
                if s['protocol'] not in PROTO:
                    print('unknown proto', s['protocol']); continue
                sl.append('{%d,%d,%du,%du,0,0}' % (ni, PROTO.index(s['protocol']) + 1, le32(s['address']), le32(s['command'])))
            else:
                t = [min(int(x), 65535) for x in s['data'].split()]
                off = len(raw_all)
                raw_all.extend(t)
                sl.append('{%d,0,%du,%du,%du,%d}' % (ni, int(s.get('frequency', 38000)), 0, off, len(t)))
        lines.append('static const char* const %s_names[] = {%s,nullptr};' % (nm, ','.join('"%s"' % esc(n) for n in names)))
        lines.append('static const IrSig %s_sigs[] = {\n%s};' % (nm, ',\n'.join(sl)))
        remotes.append('{"%s",%s,%s,"%s",%s_items,%d,%s_icons,%d,%s_names,%s_sigs,%d}' % (
            label.group(3), label.group(1), label.group(2), db, nm, len(it), nm, len(ics), nm, nm, len(sl)))
    lines.insert(4, 'extern const uint16_t IR_RAW[] PROGMEM = {%s};' % ','.join(map(str, raw_all)))
    lines.append('extern const UniRemote UNI_REMOTES[] = {\n' + ',\n'.join(remotes) + '};')
    lines.append('extern const int UNI_REMOTE_COUNT = %d;' % len(remotes))
    lines[0] = '#include "gen_ir.h"'
    open(os.path.join(OUT, 'gen_ir.cpp'), 'w').write('\n'.join(lines) + '\n')
    open(os.path.join(OUT, 'gen_ir.h'), 'w').write('#pragma once\n#include "gfx_types.h"\nextern const uint16_t IR_RAW[];\nextern const UniRemote UNI_REMOTES[];\nextern const int UNI_REMOTE_COUNT;\n')

# ---------------- BadUSB ----------------
def gen_badusb():
    base = os.path.join(SRC, 'applications', 'main', 'bad_usb', 'resources', 'badusb')
    lines = ['#pragma once', '#include <stdint.h>', '']
    kls = sorted(glob.glob(os.path.join(base, 'assets', 'layouts', '*.kl')))
    lr = []
    for i, p in enumerate(kls):
        d = open(p, 'rb').read()
        vals = [d[j] | (d[j + 1] << 8) for j in range(0, min(len(d), 256), 2)]
        vals += [0] * (128 - len(vals))
        lines.append('static const uint16_t KL_%d[128] = {%s};' % (i, ','.join('0x%04x' % v for v in vals)))
        lr.append('{"%s",KL_%d}' % (os.path.basename(p)[:-3], i))
    lines.append('struct KbLayout { const char* name; const uint16_t* map; };')
    lines.append('static const KbLayout KB_LAYOUTS[] = {%s};' % ','.join(lr))
    lines.append('static const int KB_LAYOUT_COUNT = %d;' % len(lr))
    dem = sorted(glob.glob(os.path.join(base, 'Demos', '*.txt')))
    dr = []
    for i, p in enumerate(dem):
        if 'qFlipper' in p: continue
        t = open(p, encoding='utf-8', errors='replace').read().replace('\r', '')
        lines.append('static const char DEMO_%d[] = R"MNTMDEMO(%s)MNTMDEMO";' % (i, t))
        dr.append('{"%s",DEMO_%d}' % (os.path.basename(p), i))
    lines.append('struct DemoScript { const char* name; const char* text; };')
    lines.append('static const DemoScript DEMO_SCRIPTS[] = {%s};' % ','.join(dr))
    lines.append('static const int DEMO_SCRIPT_COUNT = %d;' % len(dr))
    open(os.path.join(OUT, 'gen_badusb.h'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')

gen_icons(); gen_dolphin(); gen_fonts(); gen_ir(); gen_badusb()
print('ok')
