# Director's Cut - the village notice board with the Boar King wanted poster, in the Forgive Me Father comic style.
# Built in Blender; the poster is a Meshy text-to-image picture, the wood is the stage's inked plank picture.
#   blender --background --python Scripts/Blender_BuildNoticeBoard.py [-- preview]
# In : Art/Meshy/NoticeBoard/Source/Wanted_meshy_image.png   "WANTED / THE BOAR KING / REWARD: 10 XP" on torn parchment
#      Art/Meshy/SetFloor/Export/T_DC_SetStage_Planks.png    inked planks (reused)
# Out: Art/Meshy/NoticeBoard/Export/
#      SM_DC_NoticeBoard.glb    plank board on two posts with a small cap, the poster nailed to the front
#                               materials: M_Notice_Wood, M_Notice_Poster
#      T_DC_WantedPoster.png    the poster with the white around its torn edge cut away (alpha)
#      T_DC_QuestMark.png       the quest-giver "!" drawn here: flat yellow, thick ink outline (alpha). Shown in the
#                               level as ONE camera-facing sprite over the board (a nod to classic kill-ten-boars quests)
#      NoticeBoard.blend
# Axes: Blender (x, y) = Unreal (100 x, -100 y). Origin = on the ground under the board's centre.
# The board faces Unreal -X (towards the player spawn), so the poster is on the Blender -X side.
import bpy, bmesh, math, os, sys
import numpy as np
from mathutils import Vector

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "NoticeBoard", "Source")
OUT = os.path.join(ROOT, "Art", "Meshy", "NoticeBoard", "Export")
WOOD_TEX = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export", "T_DC_SetStage_Planks.png")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
WOOD_M = 2.4                       # metres one plank picture covers (30 cm planks, like the stage and the stall)
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)


def load_px(path):
    img = bpy.data.images.load(path)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()
    bpy.data.images.remove(img)
    return px


def save_px(px, path):
    H, W = px.shape[:2]
    img = bpy.data.images.new(os.path.basename(path), W, H, alpha=True)
    img.alpha_mode = "STRAIGHT"
    img.pixels[:] = px[::-1].ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def grow(mask, n):
    for _ in range(n):
        mask = mask | np.roll(mask, 1, 0) | np.roll(mask, -1, 0) | np.roll(mask, 1, 1) | np.roll(mask, -1, 1)
    return mask


# ---------------------------------------------------------------- poster: cut away the white outside the torn edge
po = load_px(os.path.join(SRC, "Wanted_meshy_image.png"))
PH, PW = po.shape[:2]
white = po[..., :3].min(axis=2) > 0.90
outside = np.zeros_like(white)
outside[0, :] = white[0, :]; outside[-1, :] = white[-1, :]; outside[:, 0] = white[:, 0]; outside[:, -1] = white[:, -1]
while True:                                        # flood in from the picture's border, through white only
    nxt = outside.copy()
    nxt[1:, :] |= outside[:-1, :]; nxt[:-1, :] |= outside[1:, :]; nxt[:, 1:] |= outside[:, :-1]; nxt[:, :-1] |= outside[:, 1:]
    nxt &= white
    if nxt.sum() == outside.sum():
        break
    outside = nxt
outside = grow(outside, 1)                         # take the light fringe with it
po[..., 3] = np.where(outside, 0.0, 1.0)
po[outside, :3] = (0.55, 0.45, 0.30)               # paper colour under the cut, so no white halo when filtered
print("POSTER", PW, PH, "cut away", round(float(outside.mean()) * 100, 1), "%")
save_px(po, os.path.join(OUT, "T_DC_WantedPoster.png"))

# ---------------------------------------------------------------- quest mark: a drawn "!" (256 x 512)
QW, QH = 256, 512
yy, xx = np.mgrid[0:QH, 0:QW].astype(np.float32)
t = (yy - 40) / 290.0                              # 0 at the top of the bar, 1 at its bottom
half = 58 - 34 * np.clip(t, 0, 1)                  # tapered bar: wide at the top
bar = (yy >= 40) & (yy <= 330) & (np.abs(xx - QW / 2) <= half)
dot = (xx - QW / 2) ** 2 + (yy - 420) ** 2 <= 46 ** 2
fill = bar | dot
ink = grow(fill, 12) & ~fill
qm = np.zeros((QH, QW, 4), dtype=np.float32)
qm[..., :3] = (1.0, 0.80, 0.08)
shade = (xx - QW / 2 > 14) & fill                  # flat comic shading on the right side
qm[shade, :3] = (0.88, 0.60, 0.05)
qm[ink, :3] = 0.03
qm[..., 3] = np.where(fill | ink, 1.0, 0.0)
save_px(qm, os.path.join(OUT, "T_DC_QuestMark.png"))


# ---------------------------------------------------------------- mesh helpers
def material(name, tex_path, alpha=False):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    tex = m.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(tex_path)
    bsdf = m.node_tree.nodes["Principled BSDF"]
    m.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    if alpha:
        m.node_tree.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
    bsdf.inputs["Roughness"].default_value = 1.0
    m.use_backface_culling = False
    return m


MATS = ["M_Notice_Wood", "M_Notice_Poster"]
MAT = {"M_Notice_Wood": material("M_Notice_Wood", WOOD_TEX),
       "M_Notice_Poster": material("M_Notice_Poster", os.path.join(OUT, "T_DC_WantedPoster.png"), True)}
bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")


def face(pts, uvs, mat, out):
    n = (Vector(pts[1]) - Vector(pts[0])).cross(Vector(pts[2]) - Vector(pts[0]))
    if n.dot(Vector(out)) < 0:
        pts, uvs = pts[::-1], uvs[::-1]
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    f.material_index = MATS.index(mat)
    for loop, uv in zip(f.loops, uvs):
        loop[uvl].uv = uv


def box(lo, hi, uv):
    (x0, y0, z0), (x1, y1, z1) = lo, hi
    for a, b, c, d, out, ax in (((x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1), (-1, 0, 0), 0),
                                ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1), (1, 0, 0), 0),
                                ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, -1, 0), 1),
                                ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1), (0, 1, 0), 1),
                                ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (0, 0, -1), 2),
                                ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1), 2)):
        face([a, b, c, d], [uv(p, ax) for p in (a, b, c, d)], "M_Notice_Wood", out)


def wood_h(p, ax):                 # planks lying flat, running along the board
    if ax == 0: return (p[1] / WOOD_M, p[2] / WOOD_M)
    if ax == 1: return (p[0] / WOOD_M, p[2] / WOOD_M)
    return (p[1] / WOOD_M, p[0] / WOOD_M)


def wood_v(p, ax):                 # posts: the grain runs up the post
    if ax == 0: return (p[2] / WOOD_M, p[1] / WOOD_M + 0.03)
    if ax == 1: return (p[2] / WOOD_M, p[0] / WOOD_M + 0.03)
    return (p[0] / WOOD_M, p[1] / WOOD_M)


# ---------------------------------------------------------------- the board (greybox: 3.6 m wide, 0.8 - 2.6 m high, posts 3 m apart)
box((-0.06, -1.8, 0.80), (0.06, 1.8, 2.60), wood_h)                       # plank board
box((-0.09, -1.86, 0.74), (0.09, 1.86, 0.82), wood_h)                     # bottom rail
box((-0.20, -1.98, 2.60), (0.20, 1.98, 2.68), wood_h)                     # cap, overhanging
for py in (-1.5, 1.5):
    box((0.06, py - 0.11, 0.0), (0.28, py + 0.11, 2.60), wood_v)          # posts behind the board

# the poster, nailed on a little crooked
POSTER_H = 1.62
POSTER_W = POSTER_H * PW / PH
cz, tilt = 1.71, math.radians(-3.0)
corners = []
for (dy, dz, u, v) in ((POSTER_W / 2, POSTER_H / 2, 0, 1), (-POSTER_W / 2, POSTER_H / 2, 1, 1),
                       (-POSTER_W / 2, -POSTER_H / 2, 1, 0), (POSTER_W / 2, -POSTER_H / 2, 0, 0)):
    ry = dy * math.cos(tilt) - dz * math.sin(tilt)
    rz = dy * math.sin(tilt) + dz * math.cos(tilt)
    corners.append(((-0.075, ry, cz + rz), (u, v)))
face([c[0] for c in corners], [c[1] for c in corners], "M_Notice_Poster", (-1, 0, 0))

# the quest-giver mark is NOT part of this mesh: two crossed cards looked like two marks from above and from
# the side. It is one camera-facing sprite in the level instead (LD_DressMainMap.dress_notice, T_DC_QuestMark).

bmesh.ops.triangulate(bm, faces=bm.faces)
me = bpy.data.meshes.new("SM_DC_NoticeBoard")
bm.to_mesh(me)
bm.free()
ob = bpy.data.objects.new("SM_DC_NoticeBoard", me)
bpy.context.collection.objects.link(ob)
for n in MATS:
    me.materials.append(MAT[n])
co = np.array([v.co[:] for v in me.vertices])
print("MESH size m", (co.max(0) - co.min(0)).round(2).tolist(), "tris", len(me.polygons))
bpy.ops.object.select_all(action="DESELECT")
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_NoticeBoard.glb"), export_format="GLB", use_selection=True,
                          export_apply=True, export_image_format="NONE")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "NoticeBoard.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng; break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 900, 900
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.7, 0.8, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(55), 0, math.radians(-110)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("Notice_Front", (-7.5, 0.6, 2.0), (0, 0, 2.2))
    shoot("Notice_Angle", (-5.0, -5.0, 2.6), (0, 0, 2.0))
print("DONE")
