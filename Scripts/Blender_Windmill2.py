"""New windmill art (Meshy, styled on the village house): copies the wall and roof pictures to Export, and fixes the
sail: the background removal also cut out the cream canvas inside the frame, so the canvas is painted back in
(inside the frame only) and the empty margins are trimmed so the sail fills its card.
Run: blender -b --python Scripts/Blender_Windmill2.py"""
import bpy, os

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\Placeholders"
RAW, OUT = os.path.join(ROOT, "Raw"), os.path.join(ROOT, "Export")
CANVAS = (0.93, 0.86, 0.72)
FRAME = (198, 52, 578, 1212)          # x0, y0, x1, y1 from the top-left: canvas area between the outer posts
CROP = (120, 30, 656, 1222)           # trimmed picture (hub peg cut off: each card runs through the hub)


def load(n):
    return bpy.data.images.load(os.path.join(RAW, n))


def save(w, h, px, name):
    im = bpy.data.images.new(name, w, h, alpha=True)
    im.pixels.foreach_set(px)
    im.filepath_raw = os.path.join(OUT, name + ".png"); im.file_format = "PNG"; im.save()


for src, dst in (("Windmill2_Wall.png", "T_DC_Windmill_Wall2"), ("Windmill2_Roof.png", "T_DC_Windmill_Roof")):
    im = load(src); w, h = im.size
    px = [0.0] * (w * h * 4); im.pixels.foreach_get(px)
    save(w, h, px, dst)

im = load("Windmill2_Sail.png"); w, h = im.size
px = [0.0] * (w * h * 4); im.pixels.foreach_get(px)
x0, y0, x1, y1 = FRAME
for y in range(y0, y1):
    row = h - 1 - y
    for x in range(x0, x1):
        i = (row * w + x) * 4
        a = px[i + 3]
        for c in range(3):
            px[i + c] = CANVAS[c] * (1.0 - a) + px[i + c] * a
        px[i + 3] = 1.0
cx0, cy0, cx1, cy1 = CROP
cw, ch = cx1 - cx0, cy1 - cy0
out = []
for r in range(ch):                       # output row 0 = bottom
    src_row = h - cy1 + r
    s = (src_row * w + cx0) * 4
    out.extend(px[s:s + cw * 4])
save(cw, ch, out, "T_DC_WindmillSail2")
print("WINDMILL2_DONE", cw, ch)
