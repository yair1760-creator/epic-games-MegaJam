# Director's Cut - generates inked comic UI art assets into Art/UI/ (pure Python stdlib, 2x supersampled).
#   python Scripts/Make_UIArt.py
import math
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Art", "UI")
os.makedirs(OUT, exist_ok=True)

INK = (12, 10, 8, 255)
CREAM = (246, 238, 220, 255)
GOLD = (232, 178, 48, 255)
GOLD_DARK = (176, 122, 22, 255)
GOLD_BRIGHT = (255, 224, 118, 255)
RED = (212, 36, 32, 255)
RED_DARK = (146, 18, 18, 255)
RED_BRIGHT = (248, 96, 84, 255)
CHARCOAL = (28, 25, 26, 255)
COAT = (38, 34, 42, 255)
COAT_SHADE = (22, 19, 25, 255)
SKIN = (234, 192, 162, 255)
SKIN_SHADE = (196, 148, 118, 255)
SHADES = (18, 18, 22, 255)
WHITE = (255, 252, 245, 255)
CLEAR = (0, 0, 0, 0)


def write_png(path, w, h, px):
    raw = b"".join(b"\x00" + bytes(c for p in px[y * w:(y + 1) * w] for c in p) for y in range(h))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def downsample_2x(w2, h2, px):
    w = w2 // 2
    h = h2 // 2
    out = [CLEAR] * (w * h)
    for y in range(h):
        y0 = y * 2
        row0 = y0 * w2
        row1 = (y0 + 1) * w2
        for x in range(w):
            x0 = x * 2
            c00 = px[row0 + x0]
            c10 = px[row0 + x0 + 1]
            c01 = px[row1 + x0]
            c11 = px[row1 + x0 + 1]
            a = (c00[3] + c10[3] + c01[3] + c11[3]) // 4
            if a == 0:
                continue
            r = (c00[0] * c00[3] + c10[0] * c10[3] + c01[0] * c01[3] + c11[0] * c11[3]) // max(1, c00[3] + c10[3] + c01[3] + c11[3])
            g = (c00[1] * c00[3] + c10[1] * c10[3] + c01[1] * c01[3] + c11[1] * c11[3]) // max(1, c00[3] + c10[3] + c01[3] + c11[3])
            b = (c00[2] * c00[3] + c10[2] * c10[3] + c01[2] * c01[3] + c11[2] * c11[3]) // max(1, c00[3] + c10[3] + c01[3] + c11[3])
            out[y * w + x] = (r, g, b, a)
    return out


def render_inked(w, h, layers, bg_fn=None, ink_r=4):
    """layers: list of (fn(x, y) -> bool, color, outline_group_id_or_None).
    Outline is drawn around solid shapes and between different group ids."""
    px = [CLEAR] * (w * h)
    grp = [-1] * (w * h)
    for y in range(h):
        for x in range(w):
            i = y * w + x
            for fn, col, gid in layers:
                if fn(x, y):
                    px[i] = col
                    grp[i] = gid

    offs = [(dx, dy) for dy in range(-ink_r, ink_r + 1) for dx in range(-ink_r, ink_r + 1) if dx * dx + dy * dy <= ink_r * ink_r]
    inner_offs = [(dx, dy) for dy in range(-max(1, ink_r - 1), max(1, ink_r - 1) + 1)
                  for dx in range(-max(1, ink_r - 1), max(1, ink_r - 1) + 1)
                  if dx * dx + dy * dy <= max(1, ink_r - 1) ** 2]

    out = list(px)
    for y in range(h):
        for x in range(w):
            i = y * w + x
            g = grp[i]
            if g < 0:
                for dx, dy in offs:
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < w and 0 <= yy < h and grp[yy * w + xx] >= 0:
                        out[i] = INK
                        break
            else:
                for dx, dy in inner_offs:
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < w and 0 <= yy < h:
                        gg = grp[yy * w + xx]
                        if gg >= 0 and gg != g and g < gg:
                            out[i] = INK
                            break

    if bg_fn is not None:
        for y in range(h):
            for x in range(w):
                i = y * w + x
                if out[i][3] == 0:
                    out[i] = bg_fn(x, y)
    return out


def ellipse(cx, cy, rx, ry, rot_deg=0.0):
    c = math.cos(math.radians(rot_deg))
    s = math.sin(math.radians(rot_deg))
    def fn(x, y):
        dx = x - cx
        dy = y - cy
        lx = dx * c + dy * s
        ly = -dx * s + dy * c
        return (lx / rx) ** 2 + (ly / ry) ** 2 <= 1.0
    return fn


def rect(x0, y0, x1, y1, rot_deg=0.0, cx=None, cy=None):
    if rot_deg == 0.0:
        return lambda x, y: x0 <= x <= x1 and y0 <= y <= y1
    rcx = (x0 + x1) * 0.5 if cx is None else cx
    rcy = (y0 + y1) * 0.5 if cy is None else cy
    c = math.cos(math.radians(rot_deg))
    s = math.sin(math.radians(rot_deg))
    hx = (x1 - x0) * 0.5
    hy = (y1 - y0) * 0.5
    def fn(x, y):
        dx = x - rcx
        dy = y - rcy
        lx = dx * c + dy * s
        ly = -dx * s + dy * c
        return abs(lx) <= hx and abs(ly) <= hy
    return fn


def poly(pts):
    n = len(pts)
    def fn(x, y):
        inside = False
        j = n - 1
        for i in range(n):
            xi, yi = pts[i]
            xj, yj = pts[j]
            if ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / ((yj - yi) if yj != yi else 1e-9) + xi):
                inside = not inside
            j = i
        return inside
    return fn


def make_director_portrait():
    S = 512  # 2x -> 256x256
    # Background: warm dark cinema spotlight radial gradient + comic halftone feel
    def bg(x, y):
        dx = (x - S * 0.48) / (S * 0.55)
        dy = (y - S * 0.42) / (S * 0.55)
        d = math.sqrt(dx * dx + dy * dy)
        t = max(0.0, min(1.0, 1.0 - d))
        # subtle warm amber spotlight against dark theater backdrop
        r = int(26 + 95 * (t ** 1.6))
        g = int(20 + 62 * (t ** 1.7))
        b = int(24 + 26 * (t ** 1.8))
        return (r, g, b, 255)

    layers = [
        # Coat & shoulders
        (ellipse(226, 495, 195, 165), COAT, 10),
        (ellipse(295, 510, 125, 130), COAT_SHADE, 10),
        # White shirt V + red tie/cravat
        (poly([(180, 355), (272, 355), (228, 485)]), CREAM, 12),
        (poly([(214, 372), (242, 372), (235, 455), (221, 455)]), RED, 14),
        # Coat lapels
        (poly([(140, 365), (188, 355), (218, 495), (150, 495)]), COAT_SHADE, 15),
        (poly([(315, 365), (266, 355), (236, 495), (305, 495)]), COAT_SHADE, 16),

        # Neck & Head
        (rect(192, 300, 260, 378), SKIN_SHADE, 20),
        (ellipse(226, 232, 92, 108), SKIN, 22),
        # Cheekbone shade
        (poly([(250, 220), (318, 210), (310, 305), (242, 336), (262, 265)]), SKIN_SHADE, 22),
        # Left ear
        (ellipse(134, 240, 18, 28), SKIN_SHADE, 21),

        # Deadpan stern mouth + goatee
        (poly([(205, 314), (255, 316), (244, 342), (216, 342)]), CHARCOAL, 25),
        (poly([(196, 294), (258, 292), (258, 300), (196, 302)]), INK, 26),

        # Sunglasses (iconic dark auteur shades + angled white sheen)
        (rect(144, 198, 220, 246, rot_deg=-3), SHADES, 30),
        (rect(230, 196, 308, 244, rot_deg=-3), SHADES, 31),
        (rect(214, 210, 238, 222, rot_deg=-3), SHADES, 32),
        # Specular shine on glasses
        (poly([(158, 204), (178, 204), (162, 240), (148, 240)]), WHITE, 30),
        (poly([(244, 202), (264, 202), (248, 238), (234, 238)]), WHITE, 31),
        # Heavy expressive eyebrow above shades
        (poly([(148, 184), (222, 174), (222, 188), (148, 196)]), INK, 33),
        (poly([(232, 174), (304, 182), (304, 194), (232, 188)]), INK, 34),

        # Beret (signature bright crimson beret + stem on top)
        (ellipse(224, 134, 128, 56, rot_deg=-10), RED, 40),
        (ellipse(250, 114, 108, 42, rot_deg=-8), RED_BRIGHT, 40),
        (ellipse(196, 152, 102, 26, rot_deg=-10), RED_DARK, 40),
        (rect(220, 62, 234, 86, rot_deg=-8), RED_DARK, 41),

        # Golden Megaphone held on the right side aimed left/forward
        # Handle
        (rect(344, 360, 376, 448, rot_deg=-18), CHARCOAL, 50),
        # Cone body
        (poly([(284, 316), (446, 250), (470, 408), (294, 366)]), GOLD, 52),
        (poly([(288, 342), (458, 330), (470, 408), (294, 366)]), GOLD_DARK, 52),
        (poly([(286, 320), (446, 254), (450, 286), (290, 334)]), GOLD_BRIGHT, 52),
        # Mouthpiece cap on left
        (ellipse(286, 342, 16, 28, rot_deg=-14), CHARCOAL, 53),
        # Rim opening on right
        (ellipse(456, 330, 22, 82, rot_deg=-10), GOLD_BRIGHT, 54),
        (ellipse(456, 330, 13, 66, rot_deg=-10), CHARCOAL, 55),
        # Hand gripping handle
        (ellipse(358, 396, 32, 26, rot_deg=-12), SKIN, 56),
    ]

    px2 = render_inked(S, S, layers, bg_fn=bg, ink_r=6)
    # Add crisp outer dark comic frame border
    b = 10
    for y in range(S):
        for x in range(S):
            if x < b or x >= S - b or y < b or y >= S - b:
                px2[y * S + x] = INK
            elif x < b + 6 or x >= S - b - 6 or y < b + 6 or y >= S - b - 6:
                px2[y * S + x] = GOLD

    px = downsample_2x(S, S, px2)
    write_png(os.path.join(OUT, "T_DC_Director_Portrait.png"), 256, 256, px)


def make_clapper_bar():
    W2, H2 = 1024, 64  # -> 512x32
    px2 = [CHARCOAL] * (W2 * H2)
    stripe_w = 56
    for y in range(H2):
        for x in range(W2):
            if y < 6 or y >= H2 - 6 or x < 6 or x >= W2 - 6:
                px2[y * W2 + x] = INK
            else:
                v = (x + y) % (stripe_w * 2)
                if v < stripe_w:
                    px2[y * W2 + x] = CREAM
                elif v < stripe_w + 5 or v >= stripe_w * 2 - 5:
                    px2[y * W2 + x] = INK
                else:
                    px2[y * W2 + x] = (32, 28, 28, 255)
    px = downsample_2x(W2, H2, px2)
    write_png(os.path.join(OUT, "T_DC_UI_ClapperBar.png"), 512, 32, px)


def make_heart_icon():
    S = 256  # -> 128x128
    def heart_inside(x, y):
        nx = (x - 128.0) / 76.0
        ny = -(y - 118.0) / 76.0
        # Classic heart implicit curve: (x^2 + y^2 - 1)^3 - x^2 * y^3 <= 0
        v = (nx * nx + ny * ny - 1.0)
        return (v * v * v - (nx * nx) * (ny * ny * ny)) <= 0.0

    def heart_shade(x, y):
        return heart_inside(x, y) and (x - y * 0.35 > 105)

    layers = [
        (heart_inside, RED, 1),
        (heart_shade, RED_DARK, 1),
        (ellipse(92, 92, 18, 12, rot_deg=-35), WHITE, 1),
    ]
    px2 = render_inked(S, S, layers, ink_r=8)
    px = downsample_2x(S, S, px2)
    write_png(os.path.join(OUT, "T_DC_UI_HeartIcon.png"), 128, 128, px)


def make_slate_icon():
    S = 256  # -> 128x128
    layers = [
        # Slate body
        (rect(46, 94, 230, 218), CHARCOAL, 1),
        (rect(58, 128, 218, 206), (48, 44, 46, 255), 1),
        # Gold hinge + horizontal slate rules
        (rect(46, 94, 230, 118), GOLD, 2),
        (rect(68, 146, 208, 156), CREAM, 1),
        (rect(68, 174, 174, 184), GOLD, 1),
        # Angled clapper arm on top
        (rect(46, 48, 232, 82, rot_deg=-10, cx=56, cy=88), CREAM, 3),
        (rect(80, 48, 120, 82, rot_deg=-10, cx=56, cy=88), CHARCOAL, 3),
        (rect(158, 48, 198, 82, rot_deg=-10, cx=56, cy=88), CHARCOAL, 3),
    ]
    px2 = render_inked(S, S, layers, ink_r=7)
    px = downsample_2x(S, S, px2)
    write_png(os.path.join(OUT, "T_DC_UI_SlateIcon.png"), 128, 128, px)


def make_crosshair():
    S = 128  # -> 64x64
    layers = [
        # Center dot
        (ellipse(64, 64, 5, 5), CREAM, 1),
        # 4 camera viewfinder corner brackets
        (rect(22, 22, 42, 28), CREAM, 2),
        (rect(22, 22, 28, 42), CREAM, 2),
        (rect(86, 22, 106, 28), CREAM, 3),
        (rect(100, 22, 106, 42), CREAM, 3),
        (rect(22, 100, 42, 106), CREAM, 4),
        (rect(22, 86, 28, 106), CREAM, 4),
        (rect(86, 100, 106, 106), CREAM, 5),
        (rect(100, 86, 106, 106), CREAM, 5),
    ]
    px2 = render_inked(S, S, layers, ink_r=4)
    px = downsample_2x(S, S, px2)
    write_png(os.path.join(OUT, "T_DC_UI_Crosshair.png"), 64, 64, px)


if __name__ == "__main__":
    make_director_portrait()
    make_clapper_bar()
    make_heart_icon()
    make_slate_icon()
    make_crosshair()
    print("Generated UI art in", OUT)
