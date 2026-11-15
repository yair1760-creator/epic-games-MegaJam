"""OPTIONS button and "not enough budget" card (Meshy nano-banana-2, styled on the team's START button and PAUSED panel).
Meshy's background removal also removed the cream inside the outlines (and the pig's pink face), so:
  - pixels connected to the picture's border through transparent area = outside, stay transparent;
  - every other transparent or half-transparent pixel is inside an outline: laid over cream (#F0D3AC), or over pink
    when it is inside the pig face's box (card only);
  - faint dark ink (the half-removed "FOR THE OPTIONS" letters) is made solid again;
  - the result is trimmed to the drawing with a small margin.
Run: blender -b --python Scripts/Blender_OptionsMenuArt.py"""
import bpy, os
import numpy as np
from collections import deque

SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\RatedPG\Source"
OUT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\RatedPG"
CREAM = np.array([0.941, 0.827, 0.675])      # #F0D3AC, the START bar / PAUSED panel colour (sRGB)
PINK = np.array([0.965, 0.690, 0.745])
JOBS = [
    ("Button_Options_meshy.png", "Button_Options.png", None),
    # pig face box in the source picture, from the top-left: x0, y0, x1, y1
    ("Options_NoBudget_meshy.png", "Options_NoBudget_Panel.png", (480, 520, 720, 735)),
]


def load(path):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"           # keep the file's sRGB numbers as they are
    w, h = im.size
    a = np.empty(w * h * 4, dtype=np.float32); im.pixels.foreach_get(a)
    return a.reshape(h, w, 4)[::-1].copy()             # row 0 = top


def save(px, path):
    h, w = px.shape[:2]
    im = bpy.data.images.new("out", w, h, alpha=True)
    im.colorspace_settings.name = "Non-Color"
    im.pixels.foreach_set(np.ascontiguousarray(px[::-1]).ravel())
    im.filepath_raw = path; im.file_format = "PNG"; im.save()


def outside_mask(alpha, thr=0.5):
    h, w = alpha.shape
    clear = alpha < thr
    out = np.zeros((h, w), bool)
    q = deque()
    for x in range(w):
        for y in (0, h - 1):
            if clear[y, x] and not out[y, x]: out[y, x] = True; q.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if clear[y, x] and not out[y, x]: out[y, x] = True; q.append((y, x))
    while q:
        y, x = q.popleft()
        for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if 0 <= ny < h and 0 <= nx < w and clear[ny, nx] and not out[ny, nx]:
                out[ny, nx] = True; q.append((ny, nx))
    return out


for src, dst, pig in JOBS:
    px = load(os.path.join(SRC, src))
    h, w = px.shape[:2]
    rgb, a = px[..., :3], px[..., 3]
    out = outside_mask(a)
    inside = ~out
    fill = np.broadcast_to(CREAM, rgb.shape).copy()
    if pig:
        x0, y0, x1, y1 = pig
        fill[y0:y1, x0:x1] = PINK
        # outside the pig's own outline the box is still card: only pixels the pig ring encloses get pink
        sub = outside_mask(a[y0:y1, x0:x1], 0.5)
        box = fill[y0:y1, x0:x1]; box[sub] = CREAM
    luma = rgb @ np.array([0.299, 0.587, 0.114])
    ink = inside & (luma < 0.35) & (a > 0.08)
    if pig:
        ink[pig[1]:pig[3], pig[0]:pig[2]] = False    # inside the pig, faint dark is leftover smudge, not ink
    a_eff = np.where(ink, np.minimum(1.0, a * 3.0), a)
    new_rgb = np.where(inside[..., None], fill * (1.0 - a_eff[..., None]) + rgb * a_eff[..., None], rgb)
    new_a = np.where(inside, 1.0, a)
    res = np.dstack([new_rgb, new_a]).astype(np.float32)
    ys, xs = np.where(new_a > 0.05)
    m = 6
    y0, y1 = max(0, ys.min() - m), min(h, ys.max() + m + 1)
    x0, x1 = max(0, xs.min() - m), min(w, xs.max() + m + 1)
    res = res[y0:y1, x0:x1]
    save(res, os.path.join(OUT, dst))
    print("OPTIONS_ART", dst, res.shape[1], res.shape[0])


# ---- the crying boar (Meshy image-to-image from the designer's T_Boar_idle_01) replaces the round pig on the card
BOAR_SRC = os.path.join(SRC, "Boar_Crying_meshy.png")
CARD = os.path.join(OUT, "Options_NoBudget_Panel.png")
PIG_CENTRE, PIG_RADIUS = (551, 607), 112          # the round pig on the trimmed card, covered with card cream
BOAR_CENTRE, BOAR_HEIGHT = (551, 588), 340        # where the boar goes (between the text and the card's bottom edge)

boar = load(BOAR_SRC)
bh, bw = boar.shape[:2]
lum = boar[..., :3] @ np.array([0.299, 0.587, 0.114])
bg = outside_mask(np.where(lum > 0.9, 0.0, 1.0))          # near-white connected to the border = background
alpha = np.where(bg, 0.0, 1.0).astype(np.float32)
ys, xs = np.where(alpha > 0)
boar = np.dstack([boar[..., :3], alpha])[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
bh, bw = boar.shape[:2]
scale = BOAR_HEIGHT / bh
nw, nh = int(round(bw * scale)), int(round(bh * scale))
im = bpy.data.images.new("boar", bw, bh, alpha=True)
im.colorspace_settings.name = "Non-Color"
im.pixels.foreach_set(np.ascontiguousarray(boar[::-1]).ravel())
im.scale(nw, nh)
small = np.empty(nw * nh * 4, dtype=np.float32); im.pixels.foreach_get(small)
small = small.reshape(nh, nw, 4)[::-1]

card = load(CARD)
ch, cw = card.shape[:2]
yy, xx = np.mgrid[0:ch, 0:cw]
pig = (xx - PIG_CENTRE[0]) ** 2 + (yy - PIG_CENTRE[1]) ** 2 <= PIG_RADIUS ** 2
card[pig, :3] = CREAM
cx, cy = BOAR_CENTRE
x0, y0 = cx - nw // 2, cy - nh // 2
region = card[y0:y0 + nh, x0:x0 + nw]
a = small[..., 3:4]
region[..., :3] = small[..., :3] * a + region[..., :3] * (1.0 - a)
region[..., 3:4] = np.maximum(region[..., 3:4], a)
save(card, CARD)
print("OPTIONS_ART boar pasted", nw, nh)
