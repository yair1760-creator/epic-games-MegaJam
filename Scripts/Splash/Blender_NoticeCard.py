"""Content notice card (Meshy nano-banana-2, styled on the "not enough budget" card and the PAUSED panel; words only).
Cuts the picture out of its white background (white connected to the border = outside) and trims it.
Writes Art/UI/RatedPG/ContentNotice_Card.png.   Run: blender -b --python Scripts/Splash/Blender_QuitBar.py"""
import bpy
import numpy as np
from collections import deque

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\RatedPG"
im = bpy.data.images.load(ROOT + r"\Source\ContentNotice_Card_meshy.png")
im.colorspace_settings.name = "Non-Color"
w, h = im.size
px = np.empty(w * h * 4, dtype=np.float32); im.pixels.foreach_get(px)
px = px.reshape(h, w, 4)[::-1].copy()
white = px[..., :3].min(axis=2) > 0.9
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
px[..., 3] = np.where(out, 0.0, 1.0)
ys, xs = np.where(~out)
m = 6
px = px[max(0, ys.min() - m):ys.max() + m + 1, max(0, xs.min() - m):xs.max() + m + 1]
res = bpy.data.images.new("out", px.shape[1], px.shape[0], alpha=True)
res.colorspace_settings.name = "Non-Color"
res.pixels.foreach_set(np.ascontiguousarray(px[::-1]).ravel())
res.filepath_raw = ROOT + r"\ContentNotice_Card.png"; res.file_format = "PNG"; res.save()
print("NOTICE_CARD", px.shape[1], px.shape[0])

# preview: the card 1150 px wide, centred on a black 1920 x 1080 screen
ch, cw = px.shape[:2]
nw = 1150; nh = int(round(ch * nw / cw))
res.scale(nw, nh)
small = np.empty(nw * nh * 4, dtype=np.float32); res.pixels.foreach_get(small)
small = small.reshape(nh, nw, 4)[::-1]
bg = np.zeros((1080, 1920, 4), dtype=np.float32); bg[..., 3] = 1.0
y0, x0 = (1080 - nh) // 2, (1920 - nw) // 2
a = small[..., 3:4]
bg[y0:y0 + nh, x0:x0 + nw, :3] = small[..., :3] * a
prev = bpy.data.images.new("prev", 1920, 1080, alpha=True)
prev.colorspace_settings.name = "Non-Color"
prev.pixels.foreach_set(np.ascontiguousarray(bg[::-1]).ravel())
prev.filepath_raw = ROOT + r"\..\MainMenu\Preview\Preview_ContentNotice_Card.png"; prev.file_format = "PNG"; prev.save()
