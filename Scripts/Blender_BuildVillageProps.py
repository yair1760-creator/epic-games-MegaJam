# Director's Cut - village props in the Forgive Me Father comic style, built in Blender:
#   the WELL (stone ring, water, two posts, windlass with crank, rope and bucket, little plank roof)
#   the SWORD RACK (plank base and back board with a rail; the swords themselves are the game's SM_DC_Sword, placed in the level)
#   blender --background --python Scripts/Blender_BuildVillageProps.py [-- preview]
# In : Art/Meshy/Village/Source/StoneBricks_meshy_image.png   Meshy text-to-image: inked stone bricks
#      Art/Meshy/SetFloor/Export/T_DC_SetStage_Planks.png     inked planks (reused for all wood)
# Out: Art/Meshy/Village/Export/
#      SM_DC_Well.glb        materials M_Well_Stone, M_Well_Wood, M_Well_Water   (greybox: cylinder r 1.1 m, 1 m tall)
#      SM_DC_SwordRack.glb   material  M_Rack_Wood                               (greybox: box 1 x 4 x 1.2 m)
#      T_DC_StoneBricks.png  the picture's inner bricks, mirrored so it wraps left-right
#      T_DC_WellWater.png    drawn here: dark water with ink ripples
#      VillageProps.blend
# Axes: Blender (x, y) = Unreal (100 x, -100 y). Origins = on the ground at each prop's centre.
# The rack's open side (where the swords stand) is Unreal +X = Blender +X, towards the sword pickup point.
import bpy, bmesh, math, os, sys
import numpy as np
from mathutils import Vector

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "Village", "Source")
OUT = os.path.join(ROOT, "Art", "Meshy", "Village", "Export")
WOOD_TEX = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export", "T_DC_SetStage_Planks.png")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
WOOD_M = 2.4
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)


def load_px(path):
    img = bpy.data.images.load(path)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()
    bpy.data.images.remove(img)
    px[..., 3] = 1.0
    return px


def save_px(px, path):
    H, W = px.shape[:2]
    img = bpy.data.images.new(os.path.basename(path), W, H, alpha=False)
    img.pixels[:] = px[::-1].ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def resize(px, w, h):
    H, W = px.shape[:2]
    ys = (np.arange(h) + 0.5) * H / h - 0.5
    xs = (np.arange(w) + 0.5) * W / w - 0.5
    y0 = np.clip(np.floor(ys).astype(int), 0, H - 1); y1 = np.clip(y0 + 1, 0, H - 1); fy = np.clip(ys - y0, 0, 1)[:, None, None]
    x0 = np.clip(np.floor(xs).astype(int), 0, W - 1); x1 = np.clip(x0 + 1, 0, W - 1); fx = np.clip(xs - x0, 0, 1)[None, :, None]
    a = px[y0][:, x0] * (1 - fx) + px[y0][:, x1] * fx
    b = px[y1][:, x0] * (1 - fx) + px[y1][:, x1] * fx
    return a * (1 - fy) + b * fy


# ---------------------------------------------------------------- textures
# stone: the Meshy picture has a white margin, so keep its 8 inner rows of bricks and mirror them left-right
# -> one texture = 8 rows tall, wraps around a ring with no seam
st = load_px(os.path.join(SRC, "StoneBricks_meshy_image.png"))
crop = st[108:906, 32:990]
save_px(resize(np.concatenate([crop, crop[:, ::-1]], axis=1), 2048, 1024), os.path.join(OUT, "T_DC_StoneBricks.png"))
ROWS = 8                               # rows of bricks in the texture

# water: dark teal with a few inked ripple rings
N = 256
yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
r = np.hypot(xx - N / 2, yy - N / 2)
wt = np.zeros((N, N, 4), dtype=np.float32); wt[..., 3] = 1.0
wt[..., :3] = (0.07, 0.16, 0.18)
for rr, wdt in ((38, 3.0), (70, 2.5), (98, 2.0)):
    ang = np.arctan2(yy - N / 2, xx - N / 2)
    ring = (np.abs(r - rr) < wdt) & (np.sin(ang * 3 + rr) > -0.35)       # broken rings
    wt[ring, :3] = (0.02, 0.03, 0.03)
hl = (np.abs(r - 54) < 2.0) & (np.sin(np.arctan2(yy - N / 2, xx - N / 2) * 2 + 1.0) > 0.6)
wt[hl, :3] = (0.45, 0.62, 0.62)
save_px(wt, os.path.join(OUT, "T_DC_WellWater.png"))


def material(name, tex_path):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    tex = m.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(tex_path)
    m.node_tree.links.new(tex.outputs["Color"], m.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
    m.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
    m.use_backface_culling = False
    return m


MAT = {"M_Well_Stone": material("M_Well_Stone", os.path.join(OUT, "T_DC_StoneBricks.png")),
       "M_Well_Wood": material("M_Well_Wood", WOOD_TEX),
       "M_Well_Water": material("M_Well_Water", os.path.join(OUT, "T_DC_WellWater.png")),
       "M_Rack_Wood": material("M_Rack_Wood", WOOD_TEX)}


class Mesh:
    def __init__(self, name, mats):
        self.name, self.mats = name, mats
        self.bm = bmesh.new()
        self.uvl = self.bm.loops.layers.uv.new("UVMap")

    def face(self, pts, uvs, mat, out):
        n = (Vector(pts[1]) - Vector(pts[0])).cross(Vector(pts[2]) - Vector(pts[0]))
        if n.dot(Vector(out)) < 0:
            pts, uvs = pts[::-1], uvs[::-1]
        f = self.bm.faces.new([self.bm.verts.new(p) for p in pts])
        f.material_index = self.mats.index(mat)
        for loop, uv in zip(f.loops, uvs):
            loop[self.uvl].uv = uv

    def box(self, lo, hi, mat, uv):
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        for a, b, c, d, out, ax in (((x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1), (-1, 0, 0), 0),
                                    ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1), (1, 0, 0), 0),
                                    ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, -1, 0), 1),
                                    ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1), (0, 1, 0), 1),
                                    ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (0, 0, -1), 2),
                                    ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1), 2)):
            self.face([a, b, c, d], [uv(p, ax) for p in (a, b, c, d)], mat, out)

    def finish(self):
        bmesh.ops.triangulate(self.bm, faces=self.bm.faces)
        me = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(me)
        self.bm.free()
        ob = bpy.data.objects.new(self.name, me)
        bpy.context.collection.objects.link(ob)
        for m in self.mats:
            me.materials.append(MAT[m])
        co = np.array([v.co[:] for v in me.vertices])
        print("MESH", self.name, "size m", (co.max(0) - co.min(0)).round(2).tolist(), "tris", len(me.polygons))
        return ob


def wood_h(axis_long):
    """planks lying flat, running along x (0) or y (1)"""
    def uv(p, ax):
        if ax == 2: return (p[axis_long] / WOOD_M, p[1 - axis_long] / WOOD_M)
        return (p[axis_long] / WOOD_M if ax != axis_long else p[1 - axis_long] / WOOD_M, p[2] / WOOD_M)
    return uv


def wood_v(p, ax):                     # posts: the grain runs up
    if ax == 0: return (p[2] / WOOD_M, p[1] / WOOD_M + 0.03)
    if ax == 1: return (p[2] / WOOD_M, p[0] / WOOD_M + 0.03)
    return (p[0] / WOOD_M, p[1] / WOOD_M)


# ---------------------------------------------------------------- 1: the well
w = Mesh("SM_DC_Well", ["M_Well_Stone", "M_Well_Wood", "M_Well_Water"])
SEG, RO, RI, WH = 28, 1.10, 0.80, 1.00
REPS = 2.0                             # the stone texture goes round the ring twice
V_ROW = 1.0 / ROWS


def ring_pt(rad, k, z):
    a = 2 * math.pi * k / SEG
    return (rad * math.cos(a), rad * math.sin(a), z)


for k in range(SEG):
    u0, u1 = REPS * k / SEG, REPS * (k + 1) / SEG
    a_mid = 2 * math.pi * (k + 0.5) / SEG
    outn = (math.cos(a_mid), math.sin(a_mid), 0)
    inn = (-outn[0], -outn[1], 0)
    # outside wall: 5 rows of bricks
    w.face([ring_pt(RO, k, 0), ring_pt(RO, k + 1, 0), ring_pt(RO, k + 1, WH), ring_pt(RO, k, WH)],
           [(u0, 0), (u1, 0), (u1, 5 * V_ROW), (u0, 5 * V_ROW)], "M_Well_Stone", outn)
    # inside wall, down to the water
    w.face([ring_pt(RI, k, 0.45), ring_pt(RI, k + 1, 0.45), ring_pt(RI, k + 1, WH), ring_pt(RI, k, WH)],
           [(u0 + 0.13, 5 * V_ROW), (u1 + 0.13, 5 * V_ROW), (u1 + 0.13, 8 * V_ROW), (u0 + 0.13, 8 * V_ROW)], "M_Well_Stone", inn)
    # cap stones: one row of bricks laid flat on top, a little wider than the wall
    co, ci, cz = RO + 0.07, RI - 0.05, WH + 0.09
    w.face([ring_pt(ci, k, cz), ring_pt(ci, k + 1, cz), ring_pt(co, k + 1, cz), ring_pt(co, k, cz)],
           [(u0 + 0.31, 6 * V_ROW), (u1 + 0.31, 6 * V_ROW), (u1 + 0.31, 7 * V_ROW), (u0 + 0.31, 7 * V_ROW)], "M_Well_Stone", (0, 0, 1))
    w.face([ring_pt(co, k, WH), ring_pt(co, k + 1, WH), ring_pt(co, k + 1, cz), ring_pt(co, k, cz)],
           [(u0 + 0.31, 7 * V_ROW), (u1 + 0.31, 7 * V_ROW), (u1 + 0.31, 7.45 * V_ROW), (u0 + 0.31, 7.45 * V_ROW)], "M_Well_Stone", outn)
    w.face([ring_pt(ci, k, WH), ring_pt(ci, k + 1, WH), ring_pt(ci, k + 1, cz), ring_pt(ci, k, cz)],
           [(u0 + 0.31, 7 * V_ROW), (u1 + 0.31, 7 * V_ROW), (u1 + 0.31, 7.45 * V_ROW), (u0 + 0.31, 7.45 * V_ROW)], "M_Well_Stone", inn)
    w.face([ring_pt(RO, k, WH), ring_pt(RO, k + 1, WH), ring_pt(co, k + 1, WH), ring_pt(co, k, WH)],
           [(u0, 0), (u1, 0), (u1, 0.02), (u0, 0.02)], "M_Well_Stone", (0, 0, -1))
    # water
    p0, p1 = ring_pt(RI, k, 0.45), ring_pt(RI, k + 1, 0.45)
    w.face([(0, 0, 0.45), p0, p1], [(0.5, 0.5), (0.5 + p0[0] / (2 * RI), 0.5 + p0[1] / (2 * RI)), (0.5 + p1[0] / (2 * RI), 0.5 + p1[1] / (2 * RI))],
           "M_Well_Water", (0, 0, 1))
CAP = WH + 0.09
# posts on the ring (left and right along y), windlass between them, gabled plank roof
for py in (-0.95, 0.95):
    w.box((-0.09, py - 0.09, CAP), (0.09, py + 0.09, 2.62), "M_Well_Wood", wood_v)
w.box((-0.07, -1.05, 1.86), (0.07, 1.05, 2.00), "M_Well_Wood", wood_h(1))                    # windlass beam
w.box((-0.03, 1.05, 1.90), (0.03, 1.30, 1.96), "M_Well_Wood", wood_h(1))                     # crank arm out
w.box((-0.03, 1.24, 1.66), (0.03, 1.30, 1.96), "M_Well_Wood", wood_v)                        # crank arm down
w.box((-0.03, 1.24, 1.66), (0.03, 1.46, 1.72), "M_Well_Wood", wood_h(1))                     # crank handle
w.box((-0.012, -0.012, 1.42), (0.012, 0.012, 1.86), "M_Well_Wood", lambda p, ax: (0.02, 0.02))   # rope (dark ink line of the planks)
BS, BR0, BR1, BZ0, BZ1 = 10, 0.15, 0.20, 1.14, 1.42                                          # bucket: tapered, open top
for k in range(BS):
    a0, a1 = 2 * math.pi * k / BS, 2 * math.pi * (k + 1) / BS
    am = (a0 + a1) / 2
    q = [(BR0 * math.cos(a0), BR0 * math.sin(a0), BZ0), (BR0 * math.cos(a1), BR0 * math.sin(a1), BZ0),
         (BR1 * math.cos(a1), BR1 * math.sin(a1), BZ1), (BR1 * math.cos(a0), BR1 * math.sin(a0), BZ1)]
    uu0, uu1 = k * 0.11, (k + 1) * 0.11
    w.face(q, [(0.02, uu0), (0.02, uu1), (0.14, uu1), (0.14, uu0)], "M_Well_Wood", (math.cos(am), math.sin(am), 0))
    w.face([(0, 0, BZ0), q[0], q[1]], [(0.05, 0.05), (0.07, 0.05), (0.07, 0.07)], "M_Well_Wood", (0, 0, -1))
RIDGE, EAVE, RX, RY = 3.12, 2.56, 1.02, 1.32
for sx in (-1, 1):
    pts = [(0, -RY, RIDGE), (0, RY, RIDGE), (sx * RX, RY, EAVE), (sx * RX, -RY, EAVE)]
    sl = math.hypot(RX, RIDGE - EAVE)
    w.face(pts, [(-RY / WOOD_M, 0), (RY / WOOD_M, 0), (RY / WOOD_M, sl / WOOD_M), (-RY / WOOD_M, sl / WOOD_M)], "M_Well_Wood", (sx, 0, 1))
for sy in (-1, 1):                                                                           # gable ends
    yv = sy * 0.99
    w.face([(-0.86, yv, 2.60), (0.86, yv, 2.60), (0, yv, RIDGE - 0.05)],
           [(-0.86 / WOOD_M, 2.60 / WOOD_M), (0.86 / WOOD_M, 2.60 / WOOD_M), (0, (RIDGE - 0.05) / WOOD_M)], "M_Well_Wood", (0, sy, 0))
w.box((-0.05, -RY - 0.03, RIDGE - 0.03), (0.05, RY + 0.03, RIDGE + 0.05), "M_Well_Wood", wood_h(1))   # ridge board
well = w.finish()

# ---------------------------------------------------------------- 2: the sword rack (swords stand on the +X side, leaning on the rail)
rk = Mesh("SM_DC_SwordRack", ["M_Rack_Wood"])
rk.box((-0.50, -2.00, 0.00), (0.50, 2.00, 0.40), "M_Rack_Wood", wood_h(1))                   # base block
rk.box((-0.54, -2.04, 0.36), (0.54, 2.04, 0.42), "M_Rack_Wood", wood_h(1))                   # base top board
rk.box((-0.50, -2.00, 0.42), (-0.40, 2.00, 1.78), "M_Rack_Wood", wood_h(1))                  # back board
rk.box((-0.56, -2.06, 1.78), (-0.30, 2.06, 1.86), "M_Rack_Wood", wood_h(1))                  # cap
for py in (-1.93, 1.93):
    rk.box((-0.40, py - 0.07, 0.42), (-0.10, py + 0.07, 1.30), "M_Rack_Wood", wood_v)        # side cheeks
rk.box((-0.40, -2.00, 1.14), (-0.22, 2.00, 1.24), "M_Rack_Wood", wood_h(1))                  # rail the swords lean on
rack = rk.finish()

# ---------------------------------------------------------------- export
for ob in (well, rack):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, ob.name + ".glb"), export_format="GLB", use_selection=True,
                              export_apply=True, export_image_format="NONE")
rack.location = (0, 5.0, 0)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "VillageProps.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng; break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 900, 760
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.7, 0.8, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(120)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("Well_Angle", (5.2, -4.2, 3.4), (0, 0, 1.5))
    shoot("Well_Down", (2.6, 0.8, 4.2), (0, 0, 0.8))
    shoot("Rack_Angle", (5.5, 2.0, 2.6), (0, 5.0, 0.9))
print("DONE")
