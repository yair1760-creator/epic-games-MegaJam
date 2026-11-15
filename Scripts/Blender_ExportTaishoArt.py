"""Exports Taisho's Nov 15 art to PNGs the engine can import, straight from his Photoshop files (no change to the art).
Each named layer is placed on the full canvas at its own position, so every frame of an animation lines up.
- bacon_bar_01.psd   : layers 90, 80, ... 0 = the King's health bar at that percent; the flattened picture = full
- puff_of_smoke_01.psd: layers 01..11 = the smoke puff animation
- stars_puff_01.psd  : layers "Frame 1".."Frame 4" = the stars puff animation
- the two crown PSDs : the flattened picture
Output: Art/Meshy/<Name>/Frames/*.png      Run: blender -b --python Scripts/Blender_ExportTaishoArt.py"""
import os
import numpy as np
import OpenImageIO as oiio

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy"
JOBS = [
    ("BaconBar", "bacon_bar_01.psd", "T_DC_BaconBar_", None),
    ("PuffOfSmoke", "puff_of_smoke_01.psd", "T_FX_SmokePuff_", None),
    ("StarsPuff", "stars_puff_01.psd", "T_FX_StarsPuff_", "Frame "),
    ("CrownKingBaconJr", "Crwon King Becon Jr.psd", "Crown_King_Bacon_Jr", "flat"),
    ("JustTheCrown", "Just The Crwon King Becon Jr.psd", "Just_The_Crown", "flat"),
]


def save(px, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    h, w = px.shape[:2]
    out = oiio.ImageOutput.create(path)
    out.open(path, oiio.ImageSpec(w, h, 4, oiio.UINT8))
    out.write_image((np.clip(px, 0, 1) * 255 + 0.5).astype(np.uint8))
    out.close()


for folder, name, prefix, mode in JOBS:
    src = os.path.join(ROOT, folder, name)
    outdir = os.path.join(ROOT, folder, "Frames")
    inp = oiio.ImageInput.open(src)
    layers = []
    n = 0
    while inp.seek_subimage(n, 0):
        s = inp.spec()
        layers.append((n, s.getattribute("oiio:subimagename"), s.width, s.height, s.x, s.y))
        n += 1
    inp.close()
    full = layers[0]
    W, H = full[2], full[3]

    def layer_px(i):
        buf = oiio.ImageBuf(src, i, 0)
        a = buf.get_pixels(oiio.FLOAT)
        if a.shape[2] == 3:
            a = np.dstack([a, np.ones(a.shape[:2], np.float32)])
        return a

    if mode == "flat":
        px = layer_px(0)
        ys, xs = np.where(px[..., 3] > 0.02)                     # trimmed to the drawing, 8 px margin
        m = 8
        px = px[max(0, ys.min() - m):ys.max() + m + 1, max(0, xs.min() - m):xs.max() + m + 1]
        save(px, os.path.join(outdir, prefix + ".png"))
        print("EXPORT", folder, "flat", px.shape[1], px.shape[0])
        continue

    names = []
    frames = []
    if folder == "BaconBar":
        save(layer_px(0), os.path.join(outdir, prefix + "100.png"))           # the flattened picture: full bar
        names.append("100")
    for idx, lname, w, h, x, y in layers[1:]:
        if not lname or w == 0 or h == 0:
            continue
        if mode and not str(lname).startswith(mode):
            continue
        tag = str(lname).replace(mode or "", "").strip()
        tag = "%02d" % int(float(tag)) if folder != "BaconBar" else "%d" % int(float(tag))
        canvas = np.zeros((H, W, 4), np.float32)
        canvas[y:y + h, x:x + w] = layer_px(idx)
        frames.append((tag, canvas))
        names.append(tag)
    # effects: every frame trimmed to the same box (the union of all frames), so the animation keeps its alignment
    if folder != "BaconBar":
        alpha = np.max(np.stack([c[..., 3] for _, c in frames]), axis=0)
        ys, xs = np.where(alpha > 0.02)
        m = 8
        y0, y1, x0, x1 = max(0, ys.min() - m), min(H, ys.max() + m + 1), max(0, xs.min() - m), min(W, xs.max() + m + 1)
        frames = [(t, c[y0:y1, x0:x1]) for t, c in frames]
    for tag, c in frames:
        save(c, os.path.join(outdir, prefix + tag + ".png"))
    print("EXPORT", folder, frames[0][1].shape[1], frames[0][1].shape[0], names)
