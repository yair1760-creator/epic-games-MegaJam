"""Logo with a "DIRECTOR'S CUT" banner under it (PROPOSAL, not wired into the game).
The team's logo is kept pixel for pixel; the banner is one Meshy picture (nano-banana-2, styled on the logo's
cream banner), cut out of its white background and placed under the "RATED PG FOR" part.
Writes: Art/UI/RatedPG/Logo_Rated_PG_for_Pig_DirectorsCut.png (transparent)
        Art/UI/MainMenu/Preview/Preview_Logo_DirectorsCut.png (how it would look on the main menu)
Run: blender -b --python Scripts/Blender_LogoDirectorsCut.py"""
import bpy
import numpy as np
from collections import deque

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI"
BANNER_W = 700            # banner width on the 1600 px wide logo
BANNER_X, BANNER_Y = 30, 452   # its top-left on the logo canvas


def load(path):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    w, h = im.size
    a = np.empty(w * h * 4, dtype=np.float32); im.pixels.foreach_get(a)
    return a.reshape(h, w, 4)[::-1].copy()


def save(px, path):
    h, w = px.shape[:2]
    im = bpy.data.images.new("out", w, h, alpha=True)
    im.colorspace_settings.name = "Non-Color"
    im.pixels.foreach_set(np.ascontiguousarray(px[::-1]).astype(np.float32).ravel())
    im.filepath_raw = path; im.file_format = "PNG"; im.save()


def resize(px, nw, nh):
    h, w = px.shape[:2]
    im = bpy.data.images.new("tmp", w, h, alpha=True)
    im.colorspace_settings.name = "Non-Color"
    im.pixels.foreach_set(np.ascontiguousarray(px[::-1]).astype(np.float32).ravel())
    im.scale(nw, nh)
    a = np.empty(nw * nh * 4, dtype=np.float32); im.pixels.foreach_get(a)
    return a.reshape(nh, nw, 4)[::-1].copy()


def over(dst, src, x, y):
    h, w = src.shape[:2]
    reg = dst[y:y + h, x:x + w]
    a = src[..., 3:4]
    reg[..., :3] = src[..., :3] * a + reg[..., :3] * (1.0 - a)
    reg[..., 3:4] = a + reg[..., 3:4] * (1.0 - a)


# banner: white connected to the border = background
b = load(ROOT + r"\RatedPG\Source\Logo_DirectorsCut_Banner_meshy.png")
h, w = b.shape[:2]
white = (b[..., :3].min(axis=2) > 0.9)
out = np.zeros((h, w), bool); q = deque()
for x in range(w):
    for y in (0, h - 1):
        if white[y, x] and not out[y, x]: out[y, x] = True; q.append((y, x))
for y in range(h):
    for x in (0, w - 1):
        if white[y, x] and not out[y, x]: out[y, x] = True; q.append((y, x))
while q:
    y, x = q.popleft()
    for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
        if 0 <= ny < h and 0 <= nx < w and white[ny, nx] and not out[ny, nx]:
            out[ny, nx] = True; q.append((ny, nx))
b[..., 3] = np.where(out, 0.0, 1.0)
ys, xs = np.where(~out)
b = b[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
bh = int(round(b.shape[0] * BANNER_W / b.shape[1]))
b = resize(b, BANNER_W, bh)

logo = load(ROOT + r"\RatedPG\Logo_Rated_PG_for_Pig.png")
lh, lw = logo.shape[:2]
canvas = np.zeros((max(lh, BANNER_Y + bh + 10), lw, 4), dtype=np.float32)
over(canvas, b, BANNER_X, BANNER_Y)        # banner first: the logo stays on top where they meet
over(canvas, logo, 0, 0)
save(canvas, ROOT + r"\RatedPG\Logo_Rated_PG_for_Pig_DirectorsCut.png")

# preview on the menu background: logo 1026 px wide, centred 150 px above the middle (as in the game)
bg = load(ROOT + r"\MainMenu\MainMenu_BG_A_Framed.png")
bg[..., 3] = 1.0
H, W = bg.shape[:2]
k = W / 1920.0                               # the picture is smaller than the 1920 x 1080 the menu is laid out in
pw = int(round(1026 * k))
s = pw / lw
small = resize(canvas, pw, int(round(canvas.shape[0] * s)))
logo_h = int(round(lh * s))
over(bg, small, (W - pw) // 2, H // 2 - int(round(150 * k)) - logo_h // 2)
save(bg, ROOT + r"\MainMenu\Preview\Preview_Logo_DirectorsCut.png")
print("LOGO_DC_DONE", canvas.shape[1], canvas.shape[0], W, H)
