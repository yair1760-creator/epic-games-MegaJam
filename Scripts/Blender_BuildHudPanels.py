# Director's Cut - HUD panel frames from four Meshy text-to-image pictures (nano-banana-2, transparent background).
#   blender --background --python Scripts/Blender_BuildHudPanels.py
# In : Art/UI/Panels/Source/Panel_{Health,Speech,Controls,Script}_meshy_image.png
# Out: Art/UI/Panels/Export/T_UI_Panel_{Health,Speech,Controls,Script}.png  + Panels.json (sizes and 9-slice margins)
# Each picture is cropped to its frame and turned into a 9-slice ("Box") texture: the four corner blocks keep their
# drawing, only the middle strips stretch. Where an edge has a feature in its middle that must not stretch (the iron
# bands of the health frame), the texture is re-composed from corner blocks + a clean middle strip.
# The texture is saved at 2x the size it is drawn at on a 1080p screen ("logical" size in Panels.json), so the frame
# band comes out about 12-16 px thick on screen and stays sharp.
import bpy, os, json
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "UI", "Panels", "Source")
OUT = os.path.join(ROOT, "Art", "UI", "Panels", "Export")
os.makedirs(OUT, exist_ok=True)

# cols / rows: (left block, middle strip, right block) and (top block, middle strip, bottom block) as pixel ranges in
# the source picture; None = "the picture's own edge" (taken from the alpha bounds). frame_px: thickness of the frame
# band in the source; screen_px: how thick it should be on screen.
PANELS = {
    "Health":   dict(cols=((None, 230), (520, 865), (1150, None)), rows=((None, 215), (310, 475), (560, None)), frame_px=80, screen_px=13),
    "Speech":   dict(cols=((None, 170), (170, 1210), (1210, None)), rows=((None, 195), (195, 575), (575, None)), frame_px=92, screen_px=14),
    "Controls": dict(cols=((None, 450), (450, 750), (750, None)), rows=((None, 340), (340, 550), (550, None)), frame_px=82, screen_px=15),
    "Script":   dict(cols=((None, 190), (190, 1185), (1185, None)), rows=((None, 190), (190, 580), (580, None)), frame_px=56, screen_px=11),
}


def load(path):
    img = bpy.data.images.load(path)
    img.alpha_mode = "STRAIGHT"
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()      # row 0 = top
    bpy.data.images.remove(img)
    return px


def save(px, path, w, h):
    H, W = px.shape[:2]
    img = bpy.data.images.new("out", W, H, alpha=True)
    img.alpha_mode = "STRAIGHT"
    img.pixels[:] = px[::-1].ravel()
    img.scale(w, h)
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


meta = {}
for name, cfg in PANELS.items():
    px = load(os.path.join(SRC, "Panel_%s_meshy_image.png" % name))
    ys, xs = np.where(px[..., 3] > 0.5)
    x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
    (cl, cm, cr), (rt, rm, rb) = cfg["cols"], cfg["rows"]
    colr = [(x0, cl[1]), cm, (cr[0], x1)]
    rowr = [(y0, rt[1]), rm, (rb[0], y1)]
    out = np.concatenate([np.concatenate([px[r0:r1, c0:c1] for c0, c1 in colr], axis=1) for r0, r1 in rowr], axis=0)
    # the outside of the frame is not perfectly cut by the generator: fully transparent pixels get the colour of
    # their nearest opaque neighbour's row so scaling does not pull in a white or black fringe
    a = out[..., 3:4]
    out[..., :3] = out[..., :3] * (a > 0.02) + 0.03 * (a <= 0.02)
    H, W = out.shape[:2]
    s = cfg["screen_px"] / cfg["frame_px"]                       # source px -> screen px
    lw, lh = max(1, round(W * s)), max(1, round(H * s))            # logical (on-screen) size
    save(out, os.path.join(OUT, "T_UI_Panel_%s.png" % name), lw * 2, lh * 2)
    L, R = colr[0][1] - colr[0][0], colr[2][1] - colr[2][0]
    T, B = rowr[0][1] - rowr[0][0], rowr[2][1] - rowr[2][0]
    meta[name] = dict(logical=[lw, lh], margin=[round(L / W, 4), round(T / H, 4), round(R / W, 4), round(B / H, 4)],
                      corner_px=[round(L * s), round(T * s), round(R * s), round(B * s)], frame_screen_px=cfg["screen_px"],
                      source_bounds=[x0, y0, x1, y1])
    print("PANEL", name, meta[name])
json.dump(meta, open(os.path.join(OUT, "Panels.json"), "w"), indent=1)
