#!/usr/bin/env python3
"""Mirror the kit UI into TouchGFX Designer.

Input is the simulator export (PSU_UI_TOUR + PSU_LAYOUT_EXPORT, first theme)
and the matching page screenshots. For every page a container named "Layout"
is (re)written as the top-most item of the Designer screen: a background box,
boxes for cards and gauges, text areas for labels and values, and small
containers for buttons (box, icon image, label). The Designer canvas then
shows the real page and it can be edited there.

Also written:
  assets/texts/texts.xml        text group "Layout" (one text per label)
  assets/images/kiticon_*.png   button icons cropped from the screenshots
  gui/src/common/KitLayoutTable.cpp   what was exported, so the firmware can
                                      tell which items were edited in Designer

Usage: designer_layout.py <export.jsonl> <screenshots dir> <Appli/TouchGFX>
"""
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from PIL import Image, ImageFont

KINDS = ['Label', 'Panel', 'Fill', 'Bar', 'Level', 'Segments', 'Dots', 'Value', 'Rows',
         'Button', 'Slider', 'Swatch', 'Chip', 'Plot']
K_LABEL, K_PANEL, K_FILL, K_BAR, K_LEVEL, K_SEGMENTS, K_DOTS, K_VALUE, K_ROWS, K_BTN, K_SLIDER, K_SWATCH, K_CHIP, K_PLOT = range(14)
FONTS = ['KCap', 'KSub', 'KBody', 'KBodyM', 'KLabel', 'KTitle', 'KHead', 'KM12', 'KM14', 'KM16', 'KM20', 'KM26', 'KM34', 'KM52', 'KM60']
FONT_H = [15, 16, 19, 19, 21, 24, 29, 16, 19, 21, 27, 35, 45, 69, 79]
M14 = 8
SCREENS = {1: 'ScreenSplash', 2: 'Screen1', 3: 'ScreenSettings', 4: 'ScreenBattery', 5: 'ScreenBms', 6: 'ScreenUsbPd',
           7: 'ScreenMeasurements', 8: 'ScreenPowerPath', 9: 'ScreenProtection', 10: 'ScreenPresets',
           11: 'ScreenSequencer', 12: 'ScreenExtCharger', 13: 'ScreenDiagnostics', 14: 'ScreenNetwork',
           15: 'ScreenAppearance', 16: 'ScreenService'}
ALIGN = ['Left', 'Center', 'Right']


def color(c):
    return {"Red": int(c[0]), "Green": int(c[1]), "Blue": int(c[2])}


def fnv(text):
    # The text converter stores whitespace-only texts as empty strings.
    if text is not None and not text.strip():
        text = ''
    h = 2166136261
    for unit in text.encode('utf-16-le').decode('utf-16-le'):
        code = ord(unit)
        if code > 0xFFFF:
            continue
        h ^= code
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def slug(text, limit=24):
    s = re.sub(r'[^A-Za-z0-9]+', '_', text or '').strip('_')
    return s[:limit]


def clean(text):
    # "<>" marks a wildcard in TouchGFX texts; never emit it in mirrored text.
    t = (text or '').replace('<', '(').replace('>', ')')
    return t if t.strip() else ' '


def read_pages(path):
    raw = open(path, encoding='utf-8').read()
    pages = []
    for chunk in re.split(r'\n(?=\{"page")', raw.strip()):
        if chunk.strip():
            pages.append(json.loads(chunk))
    return pages


def ppm(path):
    return Image.open(path).convert('RGB')


class Generator:
    def __init__(self, app_dir, shots):
        self.app = app_dir
        self.shots = shots
        self.texts = []          # (id, typography, alignment, text)
        self.icons = {}          # pixel bytes -> file name
        self.tables = {}         # page -> entries
        self.fontcache = {}
        root = ET.parse(os.path.join(app_dir, 'assets', 'texts', 'texts.xml')).getroot()
        self.typos = {t.get('Id'): (t.get('Font'), int(t.get('Size'))) for t in root.iter('Typography')}

    def text(self, screen, index, suffix, typography, align, value):
        tid = 'D_%s_%03d%s' % (screen, index, suffix)
        self.texts.append((tid, typography, ALIGN[align], clean(value)))
        return tid, clean(value)

    def icon(self, shot, x, y, size):
        if shot is None or size <= 0:
            return None
        crop = shot.crop((x, y, x + size, y + size))
        key = crop.tobytes()
        if key not in self.icons:
            name = 'kiticon_%02d_%dpx.png' % (len(self.icons), size)
            crop.save(os.path.join(self.app, 'assets', 'images', name))
            self.icons[key] = name
        return self.icons[key]

    # ------------------------------------------------------------ helpers
    def metrics(self, font):
        """(ascent, height) of a kit typography, from the same TTF files."""
        if font not in self.fontcache:
            el = self.typos[FONTS[font]]
            f = ImageFont.truetype(os.path.join(self.app, 'assets', 'fonts', el[0]), el[1])
            a, d = f.getmetrics()
            self.fontcache[font] = (f, a, a + d)
        return self.fontcache[font]

    def width(self, font, text):
        return int(round(self.metrics(font)[0].getlength(text)))

    def box(self, name, x, y, w, h, c, visible=True):
        b = {"Type": "Box", "Name": name, "X": x, "Y": y, "Width": max(1, w), "Height": max(1, h), "Color": color(c)}
        if not visible:
            b["Visible"] = False
        return b

    def framed(self, name, w, h, fill, border):
        """Box with a 1 px outline, as two boxes (the outline is the kit border)."""
        if border is None or list(border) == list(fill) or w < 3 or h < 3:
            return [self.box(name + "_bg", 0, 0, w, h, fill)]
        return [self.box(name + "_border", 0, 0, w, h, border), self.box(name + "_bg", 1, 1, w - 2, h - 2, fill)]

    def textarea(self, name, tid, x, y, w, h, ink):
        return {"Type": "TextArea", "Name": name, "X": x, "Y": y, "Width": max(1, w), "Height": max(1, h),
                "TextId": tid, "TextRotation": "0", "Color": color(ink)}

    # ------------------------------------------------------------ page
    def page(self, p, shot):
        screen = SCREENS[p['page']]
        comps = [
            {"Type": "Box", "Name": "LayoutMarker", "Width": 1, "Height": 1, "Visible": False,
             "Color": {"Red": 40, "Green": 80, "Blue": 120}},
            {"Type": "Box", "Name": "LayoutBackground", "Width": 800, "Height": 480, "Color": color(p['background'])},
        ]
        table = []
        for i, w in enumerate(p['widgets']):
            k = w['kind']
            x, y, wd, ht = w['x'], w['y'], max(1, w['w']), max(1, w['h'])
            label = w.get('text', '')
            name = 'k%03d_%s%s' % (i, KINDS[k], ('_' + slug(label)) if slug(label) and k in (K_LABEL, K_BTN, K_CHIP) else '')
            node = {"Name": name, "X": x, "Y": y, "Width": wd, "Height": ht}
            if not w['visible']:
                node["Visible"] = False
            font_field, hash_value = 0, 0
            kids = None
            if k == K_LABEL:
                f = w.get('font', M14)
                tid, shown = self.text(screen, i, '', FONTS[f], w.get('align', 0), label)
                node.update({"Type": "TextArea", "TextId": tid, "TextRotation": "0", "Color": color(w['ink'])})
                font_field, hash_value = f, fnv(shown)
            elif k == K_VALUE:
                kids = self.value(screen, i, name, w, wd, ht)
            elif k == K_ROWS:
                kids = self.rows(screen, i, name, w, wd, ht)
            elif k == K_BTN:
                kids, font_field, hash_value = self.button(screen, i, name, w, wd, ht, shot)
            elif k == K_CHIP:
                kids = self.chip(screen, i, name, w, wd, ht)
            elif k == K_PANEL:
                kids = self.framed(name, wd, ht, w['fill'], w.get('border'))
            elif k == K_BAR:
                kids = [self.box(name + "_track", 0, 0, wd, ht, w['track'])]
                v = max(-1000, min(1000, w.get('value', 0)))
                if w.get('centered'):
                    mid, ln = wd // 2, (wd // 2) * abs(v) // 1000
                    if ln:
                        kids.append(self.box(name + "_fill", mid - ln if v < 0 else mid, 0, ln, ht, w['role']))
                    kids.append(self.box(name + "_zero", mid - 1, 0, 2, ht, w['muted']))
                elif v > 0:
                    kids.append(self.box(name + "_fill", 0, 0, wd * v // 1000, ht, w['role']))
            elif k == K_LEVEL:
                kids = self.framed(name, wd, ht, w['track'], w['outline'])
                h = (ht - 6) * max(0, min(1000, w.get('value', 0))) // 1000
                if h > 0:
                    kids.append(self.box(name + "_fill", 3, ht - 3 - h, wd - 6, h, w['role']))
            elif k == K_SEGMENTS:
                n = max(1, w.get('count', 10)); gap = 3; sw = (wd - gap * (n - 1)) // n
                lit = w.get('value', 0) * n
                kids = [self.box('%s_s%d' % (name, s), s * (sw + gap), 0, sw, ht, w['role'] if lit >= (s + 1) * 1000 else w['track'])
                        for s in range(n)]
            elif k == K_DOTS:
                kids, dx = [], 0
                for d in range(w.get('count', 0)):
                    dw = 16 if d == w.get('index') else 6
                    kids.append(self.box('%s_d%d' % (name, d), dx, 1, dw, 6, w['role'] if d == w.get('index') else w['track']))
                    dx += dw + 4
                if not kids:
                    kids = [self.box(name + "_empty", 0, 0, 1, 1, p['background'], visible=False)]
            elif k == K_SLIDER:
                knob = ht // 2; span = wd - 2 * knob; kx = knob + span * w.get('value', 0) // 1000; cy = ht // 2
                kids = [self.box(name + "_track", knob, cy - 3, span, 6, w['track']),
                        self.box(name + "_fill", knob, cy - 3, kx - knob, 6, w['role']),
                        self.box(name + "_knob", kx - 12, cy - 12, 24, 24, w['knob'])]
            elif k == K_SWATCH and shot is not None:
                img = self.crop(shot, x, y, wd, ht, 'kitswatch')
                node.update({"Type": "Image", "RelativeFilename": img})
            else:
                node.update({"Type": "Box", "Color": color(w['fill'])})
            if kids is not None:
                node.update({"Type": "Container", "Components": kids})
            comps.append(node)
            table.append((k, font_field, 1 if w['visible'] else 0, x, y, wd, ht, hash_value))
        self.tables[p['page']] = table
        return {"Type": "Container", "Name": "Layout", "Width": 800, "Height": 480, "Components": comps}

    def legacy(self, screen, items):
        """Mirror runtime position/visibility of the original Designer items."""
        compatible = {'TextArea': ('TextArea',), 'Image': ('Image',), 'Box': ('Box', 'BoxWithBorder'),
                      'Button': ('Button', 'ButtonWithLabel', 'ToggleButton', 'FlexButton', 'ButtonWithIcon'),
                      'Container': ('Container', 'ScrollableContainer', 'SwipeContainer'), 'Other': ()}
        nodes = []
        for it in items:
            path = [int(n) for n in it['path'].split('.')]
            if path[0] == 0:
                continue  # generated screen background
            path[0] -= 1
            comps, node = screen['Components'], None
            for n in path:
                if n >= len(comps):
                    node = None
                    break
                node = comps[n]
                comps = node.get('Components', [])
            if node is None:
                continue  # added by the page code at runtime, not a Designer item
            if node['Type'] not in compatible[it['type']]:
                print('  %s: Designer items do not match the firmware order (%s), left unchanged' % (screen['Name'], it['path']))
                return
            nodes.append((node, it))
        for node, it in nodes:
            if it['visible'] or it.get('themed'):
                node.pop('Visible', None)
                if node['Type'] != 'Container' or it['w'] > 0:
                    node['X'], node['Y'], node['Width'], node['Height'] = it['x'], it['y'], it['w'], it['h']
                    if not node['X']:
                        node.pop('X')
                    if not node['Y']:
                        node.pop('Y')
            else:
                node['Visible'] = False

    def crop(self, shot, x, y, w, h, prefix):
        part = shot.crop((x, y, x + w, y + h))
        key = part.tobytes()
        if key not in self.icons:
            count = sum(1 for n in self.icons.values() if n.startswith(prefix))
            name = ('%s_%02d_%dpx.png' % (prefix, count, w)) if w == h else ('%s_%02d_%dx%d.png' % (prefix, count, w, h))
            part.save(os.path.join(self.app, 'assets', 'images', name))
            self.icons[key] = name
        return self.icons[key]

    def value(self, screen, i, name, w, wd, ht):
        text = w.get('text', '')
        num, unit = text, ''
        if ' ' in text:
            head, tail = text.rsplit(' ', 1)
            if tail and len(tail) <= 4 and not tail[0].isdigit() and tail[0] != '-':
                num, unit = head, tail
        big, small = w.get('font', 12), w.get('small', 9)
        _, ab, hb = self.metrics(big)
        _, asm, hs = self.metrics(small)
        wa = self.width(big, num); wb = self.width(small, unit) if unit else 0; gap = 4 if unit else 0
        align = w.get('align', 0)
        x0 = 0 if align == 0 else (wd - wa - gap - wb) // 2 if align == 1 else wd - wa - gap - wb
        top = (ht - hb) // 2
        tid, _ = self.text(screen, i, '', FONTS[big], 0, num)
        kids = [self.textarea(name + "_number", tid, x0, top, wa + 4, hb, w['ink'])]
        if unit:
            uid, _ = self.text(screen, i, 'u', FONTS[small], 0, unit)
            kids.append(self.textarea(name + "_unit", uid, x0 + wa + gap, top + ab - asm, wb + 4, hs, w['muted']))
        return kids

    def rows(self, screen, i, name, w, wd, ht):
        kids, row = [], w.get('row', 22)
        lines = [l for l in w.get('text', '').split('\n')]
        while lines and not lines[-1].strip():
            lines.pop()
        for n, line in enumerate(lines):
            y = n * row
            if y + row > ht + row // 2:
                break
            label, value = (line[:19].rstrip(), line[19:].strip()) if len(line) > 19 else (line.rstrip(), '')
            lid, _ = self.text(screen, i, 'r%dl' % n, 'KBody', 0, label)
            kids.append(self.textarea('%s_r%d_label' % (name, n), lid, 0, y, wd // 2 + 40, row, w['ink'] if value else w['muted']))
            if value:
                vid, _ = self.text(screen, i, 'r%dv' % n, 'KM14', 2, value)
                kids.append(self.textarea('%s_r%d_value' % (name, n), vid, wd // 2, y, wd - wd // 2, row, w['ink']))
        if not kids:
            kids = [self.box(name + "_empty", 0, 0, 1, 1, [0, 0, 0], visible=False)]
        return kids

    def chip(self, screen, i, name, w, wd, ht):
        kids = [self.box(name + "_bg", 0, 0, wd, ht, w['fill'])]
        tx = 9
        if 'led' in w:
            kids.append(self.box(name + "_led", 9, 8, 8, 8, w['led']))
            tx += 13
        if w.get('text'):
            tid, _ = self.text(screen, i, '', 'KCap', 0, w['text'])
            _, a, h = self.metrics(0)
            kids.append(self.textarea(name + "_label", tid, tx, (ht - h) // 2, wd - tx - 6, h, w['ink']))
        return kids

    def button(self, screen, i, name, w, wd, ht, shot):
        label = w.get('text', '')
        kids = self.framed(name, wd, ht, w['fill'], w.get('border'))
        icon = w.get('icon', 0)
        has_detail = bool(w.get('detail'))
        f = w.get('font', 3)
        icon_file, size, ix, iy = None, 0, 0, 0
        if icon:
            if not label:
                size = min(24, w.get('iconSize', 20)); ix, iy = (wd - size) // 2, (ht - size) // 2
            elif has_detail:
                size = 22; ix, iy = 13, (ht - 22) // 2
            elif w.get('iconLeft'):
                tw = self.width(f, label); size = 16; ix, iy = (wd - tw - 26) // 2 + 1, (ht - 16) // 2
            else:
                size = 18 if w.get('align', 1) == 1 else 22; ix, iy = 10 + (24 - size) // 2, (ht - size) // 2
            if shot is not None:
                icon_file = self.crop(shot, w['x'] + ix, w['y'] + iy, size, size, 'kiticon')
                kids.append({"Type": "Image", "Name": name + "_icon", "X": ix, "Y": iy, "Width": size,
                             "Height": size, "RelativeFilename": icon_file})
        shown = None
        if has_detail:
            tx = 48 if icon else 12
            _, _, th = self.metrics(f); _, _, dh = self.metrics(w.get('detailFont', M14))
            y0 = (ht - th - dh) // 2
            tid, shown = self.text(screen, i, '', FONTS[f], 0, label)
            kids.append(self.textarea(name + "_label", tid, tx, y0, wd - tx - 12, th, w['title']))
            did, _ = self.text(screen, i, 'd', FONTS[w.get('detailFont', M14)], 0, w['detail'])
            kids.append(self.textarea(name + "_detail", did, tx, y0 + th, wd - tx - 12, dh, w['ink']))
        elif label:
            align = w.get('align', 1)
            _, _, th = self.metrics(f)
            if icon and w.get('iconLeft'):
                tw = self.width(f, label); lx = (wd - tw - 26) // 2 + 26; lw = tw + 4; al = 0
            elif icon:
                lx = 26 if align == 1 else 46; lw = wd - lx - 4; al = align
            else:
                lx = 0 if align == 1 else 10; lw = wd - 2 * lx; al = align
            tid, shown = self.text(screen, i, '', FONTS[f], al, label)
            kids.append(self.textarea(name + "_label", tid, lx, (ht - th) // 2, lw, th, w['ink']))
        boxes = 2 if kids[0]['Name'].endswith('_border') else 1
        font_field = (0x80 if icon_file else 0) | (0x40 if shown is not None else 0) | boxes
        return kids, font_field, (fnv(shown) if shown is not None else 0)

    def run(self, export):
        project_path = os.path.join(self.app, 'GUI_Lab_PD_PSU.touchgfx')
        project = json.load(open(project_path, encoding='utf-8-sig'))
        screens = {s['Name']: s for s in project['Application']['Screens']}
        for name in os.listdir(os.path.join(self.app, 'assets', 'images')):
            if name.startswith('kiticon_') or name.startswith('kitswatch_'):
                os.remove(os.path.join(self.app, 'assets', 'images', name))
        for p in read_pages(export):
            shot_path = os.path.join(self.shots, 't0-%s.ppm' % p['name'])
            shot = ppm(shot_path) if os.path.exists(shot_path) else None
            screen = screens[SCREENS[p['page']]]
            screen['Components'] = [c for c in screen['Components'] if c['Name'] != 'Layout']
            layout = self.page(p, shot)
            self.legacy(screen, p.get('legacy', []))
            # The Layout sits right above the page background; Designer items the
            # firmware still shows are drawn above it, the rest are hidden.
            screen['Components'].insert(1, layout)
        with open(project_path, 'w', encoding='utf-8', newline='\r\n') as f:
            json.dump(project, f, indent=2, ensure_ascii=False)
        self.write_texts()
        self.write_table()
        print('pages %d, texts %d, icons %d' % (len(self.tables), len(self.texts), len(self.icons)))

    def write_texts(self):
        path = os.path.join(self.app, 'assets', 'texts', 'texts.xml')
        raw = open(path, encoding='utf-8', newline='').read()
        nl = '\r\n' if '\r\n' in raw else '\n'
        raw = re.sub(r'\s*<TextGroup Id="Layout">.*?</TextGroup>', '', raw, flags=re.S)
        def esc(t):
            return t.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('\n', '&#10;')
        group = ['    <TextGroup Id="Layout">']
        for tid, typo, align, value in self.texts:
            group.append('      <Text Id="%s" Alignment="%s" TypographyId="%s">' % (tid, align, typo))
            group.append('        <Translation Language="GB">%s</Translation>' % esc(value))
            group.append('      </Text>')
        group.append('    </TextGroup>')
        raw = raw.replace('  </Texts>', nl.join(group) + nl + '  </Texts>', 1)
        open(path, 'w', encoding='utf-8', newline='').write(raw)

    def write_table(self):
        path = os.path.join(self.app, 'gui', 'src', 'common', 'KitLayoutTable.cpp')
        out = ['// Generated by tools/designer_layout.py from the simulator export - do not edit.',
               '// Exported state of every page layout; Designer values that differ are edits.',
               '#include <gui/common/KitLayout.hpp>', 'namespace kit {']
        top = max(self.tables) + 1
        for page in sorted(self.tables):
            out.append('static const LayoutEntry page%d[] = {' % page)
            for k, f, v, x, y, w, h, hv in self.tables[page]:
                out.append('    { %d, %d, %d, %d, %d, %d, %d, %uu },' % (k, f, v, x, y, w, h, hv))
            out.append('};')
        out.append('const LayoutScreen layoutScreens[] = {')
        for page in range(top):
            if page in self.tables:
                out.append('    { page%d, %d },' % (page, len(self.tables[page])))
            else:
                out.append('    { 0, 0 },')
        out.append('};')
        out.append('const unsigned layoutScreenCount = %d;' % top)
        out.append('}')
        open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')


if __name__ == '__main__':
    Generator(sys.argv[3], sys.argv[2]).run(sys.argv[1])
