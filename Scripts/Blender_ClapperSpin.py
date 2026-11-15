"""The Director's thrown clapperboard (Meshy nano-banana-2, background removed): trims it to a square and writes 4 frames
turned 0/90/180/270 degrees, played as a flipbook so it tumbles in flight. Writes Art/Director/Clapper/Clapper_0N.png.
Run: blender -b --python Scripts/Blender_ClapperSpin.py"""
import os
import numpy as np
import OpenImageIO as oiio

SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director\Clapperboard_meshy.png"
DST = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director\Clapper"
os.makedirs(DST, exist_ok=True)
a = oiio.ImageBuf(SRC).get_pixels(oiio.FLOAT)
print("CHANNELS", a.shape, "corner_alpha", float(a[0, 0, 3]) if a.shape[2] == 4 else None)
if a.shape[2] == 3:
    a = np.dstack([a, (a.max(axis=2) > 0.06).astype(np.float32)])
ys, xs = np.where(a[..., 3] > 0.05)
c = a[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
s = max(c.shape[:2]) + 16
sq = np.zeros((s, s, 4), np.float32)
y0, x0 = (s - c.shape[0]) // 2, (s - c.shape[1]) // 2
sq[y0:y0 + c.shape[0], x0:x0 + c.shape[1]] = c
for i in range(4):
    f = np.rot90(sq, k=-i).copy()
    p = os.path.join(DST, "Clapper_%02d.png" % (i + 1))
    out = oiio.ImageOutput.create(p)
    out.open(p, oiio.ImageSpec(s, s, 4, oiio.UINT8))
    out.write_image((np.clip(f, 0, 1) * 255 + 0.5).astype(np.uint8))
    out.close()
print("CLAPPER", s)
