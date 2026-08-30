#!/usr/bin/env python3
import argparse
import os
import time

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
FONTS = os.path.join(ROOT, "resources", "fonts-ttf")
OUT_DIR = os.path.join(ROOT, "src", "c", "generated")
REF_DIR = os.path.join(ROOT, ".design-ref")

SS = 1

CHARSET = [chr(c) for c in range(32, 127)]
LETTERS = [chr(c) for c in range(65, 91)] + [chr(c) for c in range(97, 123)] + [" "]
DIGITS_G = list("0123456789g ")
DIGITS_G_NEG = list("0123456789g- ")

DYNAMIC_ATLASES = [
    dict(name="brand32",   font="Reona.otf",             size=32),
    dict(name="brand_e32", font="BaiJamjuree-Light.ttf", size=32, chars="e"),
    dict(name="brand48",   font="Reona.otf",             size=48),
    dict(name="brand_e48", font="BaiJamjuree-Light.ttf", size=48, chars="e"),
    dict(name="brand40",   font="Reona.otf",             size=40),
    dict(name="brand_e40", font="BaiJamjuree-Light.ttf", size=40, chars="e"),
    dict(name="day24",     font="Lato-Black.ttf",        size=24,
         chars=LETTERS),
    dict(name="flavor14",  font="Lato-Regular.ttf",      size=14,
         chars=LETTERS),
    dict(name="flavor12",  font="Lato-Regular.ttf",      size=12,
         chars=LETTERS),
    dict(name="rg9",       font="RasterGothic18Cond.ttf", size=9,
         chars=LETTERS),
    dict(name="val20",     font="Lato-Black.ttf",        size=20,
         chars=DIGITS_G),
    dict(name="val36",     font="Lato-Black.ttf",        size=36,
         chars=DIGITS_G),
    dict(name="val12",     font="Lato-Black.ttf",        size=12,
         chars=DIGITS_G_NEG),
]

STATIC_LINES = [
    dict(name="label_protein", font="Lato-Regular.ttf", size=12, text="protein hours"),
    dict(name="label_minutes", font="Lato-Regular.ttf", size=12, text="ingredient minutes"),
    dict(name="footer_left",   font="Lato-Black.ttf",   size=12, text="\u00ba FL OZ, ("),
    dict(name="footer_right",  font="Lato-Black.ttf",   size=12, text=" mL)"),
]

def load_font(fname, px):
    return ImageFont.truetype(os.path.join(FONTS, fname), px * SS)

def rasterize(font, text):
    bbox = font.getbbox(text)
    if bbox is None:
        return None, 0, 0, 0, 0
    l, t, r, b = bbox
    w_ss, h_ss = r - l, b - t
    left, top = round(l / SS), round(t / SS)
    if w_ss <= 0 or h_ss <= 0:
        return None, 0, 0, left, top
    pad = 2 * SS
    img = Image.new("L", (w_ss + pad * 2, h_ss + pad * 2), 0)
    ImageDraw.Draw(img).text((pad - l, pad - t), text, font=font, fill=255)
    img = img.crop((pad, pad, pad + w_ss, pad + h_ss))
    w, h = max(round(w_ss / SS), 1), max(round(h_ss / SS), 1)
    if SS != 1:
        img = img.resize((w, h), Image.LANCZOS)
    return img.tobytes(), w, h, left, top

def quantize4(gray_bytes):
    return bytes(min(3, (v * 3 + 127) // 255) for v in gray_bytes)

def pack_levels(levels, w, h):
    stride = (w + 3) // 4
    out = bytearray(h * stride)
    for y in range(h):
        rowbase = y * stride
        for x in range(w):
            lvl = levels[y * w + x]
            if lvl:
                out[rowbase + (x >> 2)] |= lvl << (6 - 2 * (x & 3))
    return bytes(out), stride

class Glyph:
    __slots__ = ("data", "offset", "w", "h", "left", "top", "advance")

    def __init__(self):
        self.data = b""
        self.offset = 0
        self.w = self.h = self.left = self.top = self.advance = 0

def bake_atlas(spec):
    font = load_font(spec["font"], spec["size"])
    ascent = round(font.getmetrics()[0] / SS)
    chars = spec.get("chars", CHARSET)
    glyphs, blob = [], bytearray()
    for ch in chars:
        g = Glyph()
        g.advance = round(font.getlength(ch) / SS)
        raw, g.w, g.h, g.left, top_in_line = rasterize(font, ch)
        g.top = max(-128, min(127, top_in_line - ascent))
        if raw is not None:
            packed, _stride = pack_levels(quantize4(raw), g.w, g.h)
            g.data = packed
            g.offset = len(blob)
            blob.extend(packed)
        glyphs.append((ord(ch), g))
    return dict(blob=bytes(blob), glyphs=glyphs, ascent=ascent)

def bake_static_line(spec):
    font = load_font(spec["font"], spec["size"])
    raw, w, h, _, _top = rasterize(font, spec["text"])
    assert raw is not None, f"empty static line: {spec['name']}"
    packed, stride = pack_levels(quantize4(raw), w, h)
    return dict(blob=packed, w=w, h=h, stride=stride)

def c_blob(data):
    hexed = "".join(f"0x{b:02X}, " for b in data)
    lines = [hexed[i:i + 96].rstrip() for i in range(0, len(hexed), 96)]
    return "\n".join("    " + ln for ln in lines if ln)

def emit(out_c, out_h, atlases, statics):
    def cname(n):
        return f"g_atlas_{n}"

    h = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        "typedef struct __attribute__((packed)) {",
        "    uint16_t offset;",
        "    uint8_t w, h;",
        "    int8_t left;",
        "    int8_t top;",
        "    uint8_t advance;",
        "} Glyph;",
        "",
        "typedef struct {",
        "    const uint8_t *data;",
        "    const Glyph *glyphs;",
        "    const uint8_t *map;",
        "    uint8_t ascent;",
        "    uint8_t glyph_count;",
        "} Atlas;",
        "",
        "typedef struct {",
        "    const uint8_t *data;",
        "    uint8_t w, h;",
        "} StaticLine;",
        "",
    ]
    c = [
        '#include "glyphs.h"',
        "",
    ]

    for spec in DYNAMIC_ATLASES:
        n = spec["name"]
        d = atlases[n]
        c.append(f"static const uint8_t s_data_{n}[] = {{\n{c_blob(d['blob'])}\n}};")
        c.append(f"#define N_GLYPHS_{n} {len(d['glyphs'])}")
        c.append(f"static const Glyph s_glyphs_{n}[N_GLYPHS_{n}] = {{")
        for _, g in d["glyphs"]:
            c.append(f"    {{{g.offset},{g.w},{g.h},{g.left},{g.top},{g.advance}}},")
        c.append("};")
        cmap = [255] * 95
        for gi, (cp, _) in enumerate(d["glyphs"]):
            cmap[cp - 32] = gi
        c.append(f"static const uint8_t s_map_{n}[95] = {{"
                 + ", ".join(map(str, cmap)) + "};")
        c.append(f"const Atlas {cname(n)} = {{s_data_{n}, s_glyphs_{n}, "
                 f"s_map_{n}, {d['ascent']}, N_GLYPHS_{n}}};")
        c.append("")
        h.append(f"extern const Atlas {cname(n)};")
        h.append(f"#define GLYPH_COUNT_{n} {len(d['glyphs'])}")

    for spec in STATIC_LINES:
        n = spec["name"]
        d = statics[n]
        c.append(f"static const uint8_t s_line_{n}[] = {{\n{c_blob(d['blob'])}\n}};")
        c.append(f"const StaticLine g_line_{n} = {{s_line_{n}, {d['w']}, {d['h']}}};")
        c.append("")
        h.append(f"extern const StaticLine g_line_{n};")

    h += ["", "#ifdef __cplusplus", "}", "#endif", ""]
    with open(out_h, "w") as f:
        f.write("\n".join(h))
    with open(out_c, "w") as f:
        f.write("\n".join(c) + "\n")

SHADES = {0: 255, 1: 170, 2: 85, 3: 0}

def stride_for(w):
    return (w + 3) // 4

def glyph_of(atlas_d, ch):
    for cp, g in atlas_d["glyphs"]:
        if cp == ord(ch):
            return g
    return None

def blit(img, blob, g, x, y, color=0):
    px = img.load()
    stride = stride_for(g.w)
    for row in range(g.h):
        base = g.offset + row * stride
        gy = y + g.top + row
        if not 0 <= gy < img.height:
            continue
        for col in range(g.w):
            lvl = (blob[base + (col >> 2)] >> (6 - 2 * (col & 3))) & 3
            if lvl:
                ix = x + g.left + col
                if 0 <= ix < img.width:
                    px[ix, gy] = SHADES[lvl]

def string_width(atlas_d, text, special=None, special_atlas=None):
    total = 0
    for ch in text:
        a = special_atlas if (special and ch == special) else atlas_d
        g = glyph_of(a, ch)
        if g:
            total += g.advance
    return total

def draw_string(img, atlas_d, text, x, y, color=0, special=None,
                special_atlas=None):
    for ch in text:
        a = special_atlas if (special and ch == special) else atlas_d
        g = glyph_of(a, ch)
        if g:
            blit(img, a["blob"], g, x, y, color)
        x += g.advance if g else 0

def blit_static(img, ln, x, y, color=0):
    px = img.load()
    stride = (ln["w"] + 3) // 4
    for row in range(ln["h"]):
        base = row * stride
        gy = y + row
        if not 0 <= gy < img.height:
            continue
        for col in range(ln["w"]):
            lvl = (ln["blob"][base + (col >> 2)] >> (6 - 2 * (col & 3))) & 3
            if lvl:
                ix = x + col
                if 0 <= ix < img.width:
                    px[ix, gy] = SHADES[lvl]

def draw_box(img, x, y, w, h, stroke_w):
    px = img.load()
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            edge = (xx < x + stroke_w or xx >= x + w - stroke_w
                    or yy < y + stroke_w or yy >= y + h - stroke_w)
            px[xx, yy] = 0 if (stroke_w == 0 or edge) else 255

def ink_bounds(atlas_d, text, special=None, special_atlas=None):
    width, min_top, max_bot = 0, 127, -127
    for ch in text:
        a = special_atlas if (special and ch == special) else atlas_d
        g = glyph_of(a, ch)
        if not g or not g.w:
            continue
        if g.top < min_top:
            min_top = g.top
        if g.top + g.h > max_bot:
            max_bot = g.top + g.h
        width += g.advance
    if min_top > max_bot:
        min_top = 0
        max_bot = 0
    return width, min_top, (max_bot - min_top)

# this makes sure that the letters are the ones that need to be needed maybe i dont know im not a genius

def emit_previews(atlases, statics):
    t = time.localtime()
    days = ["Sundapple", "Mondate", "Tuesberry", "Wedmelon",
            "Thurspear", "Fricherry", "Satsuma"]

    for mode in ("normal", "bigtime"):
        img = Image.new("L", (200, 228), 255)

        bar = Image.new("L", (200, 16), 0)
        img.paste(bar, (0, 0))

        draw_string(img, atlases["day24"], days[t.tm_wday], 14,
                    13 + atlases["day24"]["ascent"])
        draw_string(img, atlases["flavor14"], "Thunderstorm Flavored", 14,
                    40 + atlases["flavor14"]["ascent"])

        brand = "brand48" if mode == "normal" else "brand40"
        spc = "brand_e48" if mode == "normal" else "brand_e40"
        bw, bh, sw = (176, 83, 4) if mode == "normal" else (176, 69, 3)
        bx, by = 12, 58

        draw_box(img, bx, by, bw, bh, sw)
        word = "Soylent"
        w_w, w_top, w_h = ink_bounds(atlases[brand], word, "e", atlases[spc])
        cx = bx + (bw - w_w) // 2
        baseline = by + (bh - w_h) // 2 - w_top
        draw_string(img, atlases[brand], word, cx, baseline, special="e",
                    special_atlas=atlases[spc])

        hour_s, min_s = "%dg" % t.tm_hour, "%02d" % t.tm_min
        val = "val20" if mode == "normal" else "val36"
        va = atlases[val]

        def center_baseline(s, box_x, box_y, box_w, box_h):
            iw, itop, ih = ink_bounds(va, s)
            return (box_x + (box_w - iw) // 2,
                    box_y + (box_h - ih) // 2 - itop)

        if mode == "normal":
            draw_box(img, 22, 151, 36, 27, 0)
            hx, hb = center_baseline(hour_s, 22, 151, 36, 27)
            draw_string(img, va, hour_s, hx, hb, color=1)
            mx, mb = center_baseline(min_s, 22, 180, 36, 27)
            draw_string(img, va, min_s, mx, mb)
            for name, lx, ly in (("label_protein", 64, 156),
                                 ("label_minutes", 64, 187)):
                blit_static(img, statics[name], lx, ly)
        else:
            draw_box(img, 16, 141, 80, 60, 0)
            hx, hb = center_baseline(hour_s, 16, 141, 80, 60)
            draw_string(img, va, hour_s, hx, hb, color=1)
            draw_box(img, 103, 141, 80, 60, 1)
            mx, mb = center_baseline(min_s, 103, 141, 80, 60)
            draw_string(img, va, min_s, mx, mb)

        fl, fr = statics["footer_left"], statics["footer_right"]
        t_s, b_s = "40", "86"
        v12 = atlases["val12"]
        gap_deg, gap_ml = 1, 2
        itw, _, _ = ink_bounds(v12, t_s)
        ibw, _, _ = ink_bounds(v12, b_s)
        total = itw + gap_deg + fl["w"] + ibw + gap_ml + fr["w"]
        fx = (200 - total) // 2
        deg_x = fx + itw + gap_deg
        batt_pen = deg_x + fl["w"]
        ml_x = batt_pen + ibw + gap_ml
        draw_string(img, v12, t_s, fx, 211 + v12["ascent"])
        blit_static(img, fl, deg_x, 211)
        draw_string(img, v12, b_s, batt_pen, 211 + v12["ascent"])
        blit_static(img, fr, ml_x, 211)
        img.save(os.path.join(REF_DIR, f"preview_{mode}.png"))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", action="store_true")
    args = ap.parse_args()
    os.makedirs(OUT_DIR, exist_ok=True)

    atlases = {s["name"]: bake_atlas(s) for s in DYNAMIC_ATLASES}
    statics = {s["name"]: bake_static_line(s) for s in STATIC_LINES}

    emit(os.path.join(OUT_DIR, "glyphs.c"), os.path.join(OUT_DIR, "glyphs.h"),
         atlases, statics)
    dyn = sum(len(a["blob"]) for a in atlases.values())
    stat = sum(len(s["blob"]) for s in statics.values())
    print(f"wrote src/generated/glyphs.c/.h  ({dyn} B dynamic atlases, "
          f"{stat} B static lines)")

    if args.preview:
        emit_previews(atlases, statics)
        print("previews -> .design-ref/preview_normal.png, preview_bigtime.png")

# nintendo 3ds. i remember

if __name__ == "__main__":
    main()
