# Rated PG for Pig - splits the 4-flower sheet (Art/Meshy/Placeholders/Export/T_DC_Flowers.png, 1376 x 768) into four
# single-flower pictures for camera-facing cards: Art/Meshy/Placeholders/Sized/T_DC_Flower_0..3.png
#   blender --background --python Scripts/Blender_SplitFlowers.py
import bpy, os
import numpy as np
ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\Placeholders"
OUT = os.path.join(ROOT, "Sized"); os.makedirs(OUT, exist_ok=True)
img = bpy.data.images.load(os.path.join(ROOT, "Export", "T_DC_Flowers.png")); W, H = img.size
px = np.array(img.pixels[:], np.float32).reshape(H, W, 4)          # row 0 = bottom (Blender order)
for k in range(4):
    x0, x1 = round(k * W / 4), round((k + 1) * W / 4)
    part = px[:, x0:x1]
    cols = np.where(part[..., 3].max(0) > 0.05)[0]                  # trim empty columns, keep 6 px margin
    a, b = max(0, cols.min() - 6), min(part.shape[1], cols.max() + 7)
    part = part[:, a:b].copy()
    out = bpy.data.images.new("f%d" % k, part.shape[1], part.shape[0], alpha=True)
    out.alpha_mode = "STRAIGHT"
    out.pixels[:] = part.ravel()
    out.filepath_raw = os.path.join(OUT, "T_DC_Flower_%d.png" % k); out.file_format = "PNG"; out.save()
    print("FLOWER", k, part.shape[1], part.shape[0])
