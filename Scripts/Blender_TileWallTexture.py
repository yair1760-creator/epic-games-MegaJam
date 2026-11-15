# Director's Cut - makes Taisho's brick wall picture repeat without a visible cut.
#   blender --background --python Scripts/Blender_TileWallTexture.py [-- preview]
# In : Art/Meshy/WallTexture/Wall_Texture.jpg (1024 x 1024, not made to tile)
# Out: Art/Meshy/WallTexture/Export/T_Taisho_Wall_Tiled.png (1024 x 1024), and with "preview" a 2 x 2 repeat to check by eye
# Why the walls showed a cut: the picture starts and ends in the middle of a brick row, and its bottom is painted darker
# than its top. Repeated up a wall, half bricks met a brightness step. Fix:
#   1. crop from the middle of the first mortar line to the middle of the last one, so each repeat joins inside mortar;
#   2. take out the slow light/dark drift (rows and columns) so the two edges meet at the same brightness;
#   3. blend the left and right edges over a narrow band so the side-to-side repeat has no step either.
import bpy, os, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "WallTexture", "Wall_Texture.jpg")
OUT = os.path.join(ROOT, "Art", "Meshy", "WallTexture", "Export")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
os.makedirs(OUT, exist_ok=True)


def load(path):
    img = bpy.data.images.load(path)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()   # row 0 = top
    bpy.data.images.remove(img)
    return px


def save(px, path):
    H, W = px.shape[:2]
    if px.shape[2] == 3:                                   # Blender images are always RGBA
        px = np.concatenate([px, np.ones((H, W, 1), dtype=np.float32)], 2)
    img = bpy.data.images.new("out", W, H, alpha=False)
    img.pixels[:] = px[::-1].ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def resize(px, w, h):
    """bilinear"""
    H, W = px.shape[:2]
    ys = (np.arange(h) + 0.5) * H / h - 0.5
    xs = (np.arange(w) + 0.5) * W / w - 0.5
    y0 = np.clip(np.floor(ys).astype(int), 0, H - 1); y1 = np.clip(y0 + 1, 0, H - 1); fy = np.clip(ys - y0, 0, 1)[:, None, None]
    x0 = np.clip(np.floor(xs).astype(int), 0, W - 1); x1 = np.clip(x0 + 1, 0, W - 1); fx = np.clip(xs - x0, 0, 1)[None, :, None]
    a = px[y0][:, x0] * (1 - fx) + px[y0][:, x1] * fx
    b = px[y1][:, x0] * (1 - fx) + px[y1][:, x1] * fx
    return a * (1 - fy) + b * fy


def smooth(v, radius):
    k = np.exp(-0.5 * (np.arange(-3 * radius, 3 * radius + 1) / radius) ** 2); k /= k.sum()
    pad = np.pad(v, 3 * radius, mode="reflect")
    return np.convolve(pad, k, mode="same")[3 * radius:-3 * radius]


def mortar_centre(ink, lo, hi):
    """centre of the mortar band in rows lo..hi: midway between its two inked outlines (the two strongest ink peaks)"""
    seg = ink[lo:hi]
    order = np.argsort(seg)[::-1]
    a = order[0]
    b = next(i for i in order[1:] if abs(i - a) >= 5)
    return lo + int(round((a + b) / 2))


px = load(SRC)[..., :3]
H, W = px.shape[:2]
lum = px @ np.array([0.3, 0.59, 0.11], dtype=np.float32)
ink = (lum < 0.18).mean(1)
top = mortar_centre(ink, 10, 70)
bottom = mortar_centre(ink, H - 70, H - 5)
print("MORTAR top", top, "bottom", bottom, "height", bottom - top)
crop = px[top:bottom].copy()

# 2. flatten slow brightness drift: divide each row and column by its smoothed mean brightness
cl = crop @ np.array([0.3, 0.59, 0.11], dtype=np.float32)
row = smooth(cl.mean(1), 60); col = smooth(cl.mean(0), 60)
crop *= (row.mean() / row)[:, None, None]
crop *= (col.mean() / col)[None, :, None]
crop = np.clip(crop, 0.0, 1.0)

# 3. left/right: blend a 24 px band at each side with the other side's edge so the repeat meets without a step
B = 24
w = np.linspace(0.5, 0.0, B, dtype=np.float32)[None, :, None]           # 0.5 at the very edge -> 0 inside
left, right = crop[:, :B].copy(), crop[:, -B:].copy()
# each edge column is mixed with the column the same distance across the seam (the far edge, mirrored), so at the
# seam both sides hold the same average and the mix fades out over B columns
crop[:, :B] = left * (1 - w) + np.flip(right, 1) * w
crop[:, -B:] = right * (1 - w[:, ::-1]) + np.flip(left, 1) * w[:, ::-1]

out = resize(crop, 1024, 1024)
save(out, os.path.join(OUT, "T_Taisho_Wall_Tiled.png"))
print("SAVED", out.shape)
if PREVIEW:
    tile = np.concatenate([np.concatenate([out, out], 1)] * 2, 0)
    save(resize(tile, 1024, 1024), os.path.join(OUT, "Preview_2x2.png"))
    orig = resize(px, 1024, 1024)
    tile0 = np.concatenate([np.concatenate([orig, orig], 1)] * 2, 0)
    save(resize(tile0, 1024, 1024), os.path.join(OUT, "Preview_2x2_Before.png"))
