"""Trims the Director's boss frames (Art/Director/Frames, 1920 x 1080 each) to one shared box (the union of every frame's
drawing, so the animation keeps its alignment and his feet stay on the same line). Writes Art/Director/Trimmed/*.png.
Run: blender -b --python Scripts/Blender_TrimDirectorFrames.py"""
import os
import numpy as np
import OpenImageIO as oiio

SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director\Frames"
DST = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director\Trimmed"
os.makedirs(DST, exist_ok=True)
names = sorted(f for f in os.listdir(SRC) if f.lower().endswith(".png"))
imgs = {}
for n in names:
    a = oiio.ImageBuf(os.path.join(SRC, n)).get_pixels(oiio.FLOAT)
    if a.shape[2] == 3:
        a = np.dstack([a, np.ones(a.shape[:2], np.float32)])
    imgs[n] = a
alpha = np.max(np.stack([a[..., 3] for a in imgs.values()]), axis=0)
ys, xs = np.where(alpha > 0.02)
m = 6
H, W = alpha.shape
y0, y1, x0, x1 = max(0, ys.min() - m), min(H, ys.max() + m + 1), max(0, xs.min() - m), min(W, xs.max() + m + 1)
for n, a in imgs.items():
    c = a[y0:y1, x0:x1]
    out = oiio.ImageOutput.create(os.path.join(DST, n))
    out.open(os.path.join(DST, n), oiio.ImageSpec(c.shape[1], c.shape[0], 4, oiio.UINT8))
    out.write_image((np.clip(c, 0, 1) * 255 + 0.5).astype(np.uint8))
    out.close()
print("TRIM", x1 - x0, y1 - y0, "opaque_corner", float(alpha[0, 0]), names)
