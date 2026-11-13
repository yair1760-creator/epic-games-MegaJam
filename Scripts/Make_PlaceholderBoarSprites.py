# Writes placeholder boar sprites (side + front) as RGBA PNGs with an ink outline, pure standard library.
# Replace them with the real boar art later: same names, or point the sprite component at new textures.
import struct, zlib, math, os

OUT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Placeholders"
os.makedirs(OUT, exist_ok=True)

BROWN = (122, 78, 46, 255)
DARK = (70, 44, 26, 255)
PINK = (214, 140, 140, 255)
TUSK = (240, 232, 210, 255)
INK = (12, 10, 8, 255)
CLEAR = (0, 0, 0, 0)


def write_png(path, w, h, px):
    raw = b"".join(b"\x00" + bytes(c for p in px[y * w:(y + 1) * w] for c in p) for y in range(h))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def render(w, h, shapes, ink=4):
    """shapes: list of (inside(x,y)->bool, color), later shapes drawn on top. Ink outline around the union."""
    px = [CLEAR] * (w * h)
    solid = [False] * (w * h)
    for y in range(h):
        for x in range(w):
            for inside, col in shapes:
                if inside(x, y):
                    px[y * w + x] = col
                    solid[y * w + x] = True
    # outline: clear pixels within `ink` of a solid pixel become ink; also ink between different colours
    out = list(px)
    for y in range(h):
        for x in range(w):
            i = y * w + x
            if not solid[i]:
                near = False
                for dy in range(-ink, ink + 1):
                    for dx in range(-ink, ink + 1):
                        if dx * dx + dy * dy > ink * ink:
                            continue
                        xx, yy = x + dx, y + dy
                        if 0 <= xx < w and 0 <= yy < h and solid[yy * w + xx]:
                            near = True
                            break
                    if near:
                        break
                if near:
                    out[i] = INK
            else:
                for dx, dy in ((1, 0), (0, 1)):
                    xx, yy = x + dx, y + dy
                    if xx < w and yy < h and solid[yy * w + xx] and px[yy * w + xx] != px[i] and px[i] != INK and px[yy * w + xx] != INK:
                        out[i] = INK
    return out


def ellipse(cx, cy, rx, ry):
    return lambda x, y: ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0


def rect(x0, y0, x1, y1):
    return lambda x, y: x0 <= x <= x1 and y0 <= y <= y1


# ---------------- side view, facing right (the component flips it for left), feet at the bottom edge
W, H = 384, 256
side = [
    (rect(110, 170, 132, 244), DARK), (rect(150, 175, 172, 244), DARK),      # back legs
    (rect(240, 175, 262, 244), DARK), (rect(272, 170, 294, 244), DARK),      # front legs
    (ellipse(190, 135, 140, 72), BROWN),                                    # body
    (ellipse(120, 92, 70, 22), DARK),                                        # bristly back
    (ellipse(300, 130, 56, 52), BROWN),                                     # head
    (lambda x, y: 262 <= x <= 300 and 60 <= y <= 95 and (x - 262) * 0.9 >= (95 - y) * 0.6, DARK),  # ear
    (ellipse(352, 146, 22, 20), PINK),                                      # snout
    (lambda x, y: 318 <= x <= 346 and 164 <= y <= 176 and y - 164 <= (346 - x) * 0.5, TUSK),    # tusk
    (ellipse(318, 118, 6, 6), INK),                                         # eye
    (lambda x, y: 40 <= x <= 56 and 110 <= y <= 150 and abs((x - 48) - 6 * math.sin((y - 110) / 6.0)) < 4, DARK),  # tail
]
write_png(os.path.join(OUT, "T_DC_Boar_Side_Placeholder.png"), W, H, render(W, H, side))

# ---------------- front view (coming at you)
W2, H2 = 256, 256
front = [
    (rect(70, 190, 96, 250), DARK), (rect(160, 190, 186, 250), DARK),        # legs
    (ellipse(128, 160, 96, 70), BROWN),                                     # body behind
    (ellipse(128, 118, 74, 66), BROWN),                                     # head
    (lambda x, y: 50 <= x <= 84 and 40 <= y <= 80 and (x - 50) >= (80 - y) * 0.3, DARK),     # left ear
    (lambda x, y: 172 <= x <= 206 and 40 <= y <= 80 and (206 - x) >= (80 - y) * 0.3, DARK),  # right ear
    (ellipse(128, 150, 34, 26), PINK),                                      # snout
    (ellipse(116, 150, 6, 9), DARK), (ellipse(140, 150, 6, 9), DARK),       # nostrils
    (lambda x, y: 80 <= x <= 96 and 150 <= y <= 186 and abs(x - 88) <= (186 - y) * 0.25, TUSK),
    (lambda x, y: 160 <= x <= 176 and 150 <= y <= 186 and abs(x - 168) <= (186 - y) * 0.25, TUSK),
    (ellipse(98, 106, 8, 8), INK), (ellipse(158, 106, 8, 8), INK),          # eyes
]
write_png(os.path.join(OUT, "T_DC_Boar_Front_Placeholder.png"), W2, H2, render(W2, H2, front))
print("ok")
