# Director's Cut - hand-drawn-style flipbook effects (ink outline, flat colors), pure standard library.
#   py Scripts/Make_PlaceholderFX.py -> Art/FX/T_FX_HitSplat_0..3.png, T_FX_Poof_0..4.png (256 x 256, RGBA)
# Used by ADCSpriteFX (camera-facing card that plays the frames once). Swap for the artist's frames, same names.
import math, os, random, struct, zlib

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Art", "FX")
os.makedirs(OUT, exist_ok=True)
W = H = 256
INK = (12, 10, 8, 255)
CLEAR = (0, 0, 0, 0)
random.seed(11)


def write_png(path, w, h, px):
    raw = b"".join(b"\x00" + bytes(c for p in px[y * w:(y + 1) * w] for c in p) for y in range(h))
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def render(shapes, ink=4):
    """shapes: list of (inside(x, y) -> bool, color); later shapes on top; ink outline around the union and between colors"""
    px = [CLEAR] * (W * H)
    solid = [False] * (W * H)
    for y in range(H):
        for x in range(W):
            for inside, col in shapes:
                if inside(x, y):
                    px[y * W + x] = col
                    solid[y * W + x] = True
    out = list(px)
    offs = [(dx, dy) for dy in range(-ink, ink + 1) for dx in range(-ink, ink + 1) if dx * dx + dy * dy <= ink * ink]
    for y in range(H):
        for x in range(W):
            i = y * W + x
            if not solid[i]:
                for dx, dy in offs:
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < W and 0 <= yy < H and solid[yy * W + xx]:
                        out[i] = INK
                        break
            else:
                for dx, dy in ((1, 0), (0, 1)):
                    xx, yy = x + dx, y + dy
                    if xx < W and yy < H and solid[yy * W + xx] and px[yy * W + xx] != px[i]:
                        out[i] = INK
    return out


def circle(cx, cy, r):
    return lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def star(cx, cy, r_out, r_in, points=8, rot=0.0):
    """spiky comic burst: radius alternates between r_out (tips) and r_in (valleys)"""
    seg = 2 * math.pi / (points * 2)
    def inside(x, y):
        dx, dy = x - cx, y - cy
        d = math.hypot(dx, dy)
        if d > r_out:
            return False
        a = (math.atan2(dy, dx) - rot) % (2 * math.pi)
        k = a / seg
        i = int(k)
        f = k - i
        r0, r1 = (r_out, r_in) if i % 2 == 0 else (r_in, r_out)
        return d <= r0 + (r1 - r0) * f
    return inside


def shard(cx, cy, ang, dist, length, width):
    """a little triangle flying outward"""
    bx, by = cx + math.cos(ang) * dist, cy + math.sin(ang) * dist
    tx, ty = cx + math.cos(ang) * (dist + length), cy + math.sin(ang) * (dist + length)
    nx, ny = -math.sin(ang) * width, math.cos(ang) * width
    A, B, C = (bx + nx, by + ny), (bx - nx, by - ny), (tx, ty)
    def inside(x, y):
        def s(p, q, r):
            return (p[0] - r[0]) * (q[1] - r[1]) - (q[0] - r[0]) * (p[1] - r[1])
        d1, d2, d3 = s((x, y), A, B), s((x, y), B, C), s((x, y), C, A)
        return not ((d1 < 0 or d2 < 0 or d3 < 0) and (d1 > 0 or d2 > 0 or d3 > 0))
    return inside


# ---------------- sword hit: comic impact burst (yellow star, white core) -> shards
YEL = (255, 214, 64, 255)
ORG = (246, 140, 40, 255)
WHT = (255, 250, 236, 255)
C = 128
hit = [
    [(star(C, C, 50, 16, 8, 0.0), YEL), (circle(C, C, 12), WHT)],
    [(star(C, C, 118, 34, 8, 0.0), ORG), (star(C, C, 82, 30, 8, 0.0), YEL), (circle(C, C, 22), WHT)],
    [(shard(C, C, a, 58, 52, 13), YEL) for a in [i * math.pi / 4 + 0.15 for i in range(8)]] + [(circle(C, C, 16), WHT)],
    [(shard(C, C, a, 92, 26, 7), YEL) for a in [i * math.pi / 4 + 0.15 for i in range(8)]],
]
for i, shapes in enumerate(hit):
    write_png(os.path.join(OUT, "T_FX_HitSplat_%d.png" % i), W, H, render(shapes, ink=4))

# ---------------- death poof: cartoon dust cloud that bursts and breaks into puffs
DUST = (238, 228, 204, 255)
DUSK = (206, 192, 160, 255)
def cloud(blobs, shade=False):   # flat: shading lines read as faces at this size
    shapes = [(circle(x, y, r), DUST) for x, y, r in blobs]
    if shade:   # a darker lower half on each puff
        shapes += [((lambda x, y, bx=bx, by=by, r=r: (x - bx) ** 2 + (y - by) ** 2 <= (r * 0.72) ** 2 and y > by + r * 0.25), DUSK) for bx, by, r in blobs]
    return shapes
poof = [
    cloud([(128, 200, 26), (104, 208, 18), (152, 206, 20)]),
    cloud([(128, 172, 44), (88, 196, 32), (168, 194, 34), (110, 140, 28), (150, 138, 26)]),
    cloud([(128, 150, 54), (76, 178, 38), (180, 176, 40), (100, 102, 36), (158, 98, 38), (128, 200, 36), (60, 132, 26), (198, 130, 26)]),
    cloud([(70, 160, 32), (186, 156, 34), (96, 86, 30), (164, 80, 30), (128, 128, 30), (128, 206, 26)]),
    cloud([(52, 150, 16), (204, 142, 18), (84, 60, 15), (176, 54, 16), (128, 104, 12)], shade=False),
]
for i, shapes in enumerate(poof):
    write_png(os.path.join(OUT, "T_FX_Poof_%d.png" % i), W, H, render(shapes, ink=5))

print("wrote", sorted(f for f in os.listdir(OUT) if f.endswith(".png")))
