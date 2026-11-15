"""Victory card "CUT! THAT'S A WRAP!" (Meshy nano-banana-2, styled on the content notice card; title drawn in).
Cuts the picture out of its white background (white connected to the border = outside) and trims it.
Writes Art/UI/RatedPG/Victory_Card.png.   Run: blender -b --python Scripts/Splash/Blender_VictoryCard.py"""
import bpy
import numpy as np
from collections import deque

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\RatedPG"
im = bpy.data.images.load(ROOT + r"\Source\Victory_Card_Title_meshy.png")
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
res.filepath_raw = ROOT + r"\Victory_Card.png"; res.file_format = "PNG"; res.save()
print("VICTORY_CARD", px.shape[1], px.shape[0])
