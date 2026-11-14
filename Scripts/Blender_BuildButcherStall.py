# Director's Cut - the butcher stall and its two joke signs in the Forgive Me Father comic style, built in Blender and
# covered with Meshy text-to-image pictures (same method as the stage and the platform floor).
#   blender --background --python Scripts/Blender_BuildButcherStall.py [-- preview]
# In : Art/Meshy/ButcherStall/Source/Awning_meshy_image.png   striped market cloth (roof)
#      Art/Meshy/ButcherStall/Source/SoldOut_meshy_image.png  only its white letters are used (the board came out as a photo)
#      Art/Meshy/ButcherStall/Source/HaHaHa_meshy_image.png   dark board with red dripping letters
#      Art/Meshy/SetFloor/Source/Planks_meshy_image.png       inked planks (the stage picture, reused for all wood)
# Out: Art/Meshy/ButcherStall/Export/
#      SM_DC_ButcherStall.glb   counter + 4 posts + 2 beams + sloped striped awning with a scalloped front edge
#                               (materials M_Stall_Wood, M_Stall_Awning). Same footprint as the greybox stall.
#      SM_DC_SoldOutSign.glb    hanging plank board, white inked "SOLD OUT" (beat 2)        + T_DC_SoldOutSign.png
#      SM_DC_LaughSign.glb      tall two-pole billboard, red "HA HA HA" (beat 2)             + T_DC_LaughSign.png
#      SM_DC_ButcherSign.glb    the stall's name board "BUTCHER" above the back of the awning + T_DC_ButcherSign.png
#                               (Source/Butcher_meshy_image.png; same origin as the stall, always visible)
#      T_DC_Stall_Awning.png, ButcherStall.blend (all four, for hand edits)
# Axes: Blender (x, y) = Unreal (100 x, -100 y). The stall's customer side is Unreal -Y = Blender +Y.
import bpy, bmesh, math, os, shutil, sys
import numpy as np
from mathutils import Vector

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "ButcherStall", "Source")
OUT = os.path.join(ROOT, "Art", "Meshy", "ButcherStall", "Export")
PLANKS = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Source", "Planks_meshy_image.png")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
WOOD_M = 2.4            # metres one plank picture covers (8 planks -> 30 cm planks, like the stage)
CLOTH_M = 3.0           # metres one awning picture covers
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)


# ---------------------------------------------------------------- pictures (row 0 = top)
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
    """bilinear"""
    H, W = px.shape[:2]
    ys = (np.arange(h) + 0.5) * H / h - 0.5
    xs = (np.arange(w) + 0.5) * W / w - 0.5
    y0 = np.clip(np.floor(ys).astype(int), 0, H - 1); y1 = np.clip(y0 + 1, 0, H - 1); fy = np.clip(ys - y0, 0, 1)[:, None, None]
    x0 = np.clip(np.floor(xs).astype(int), 0, W - 1); x1 = np.clip(x0 + 1, 0, W - 1); fx = np.clip(xs - x0, 0, 1)[None, :, None]
    a = px[y0][:, x0] * (1 - fx) + px[y0][:, x1] * fx
    b = px[y1][:, x0] * (1 - fx) + px[y1][:, x1] * fx
    return a * (1 - fy) + b * fy


def grow(mask, n):
    for _ in range(n):
        mask = mask | np.roll(mask, 1, 0) | np.roll(mask, -1, 0) | np.roll(mask, 1, 1) | np.roll(mask, -1, 1)
    return mask


planks = load_px(PLANKS)
shutil.copyfile(os.path.join(SRC, "Awning_meshy_image.png"), os.path.join(OUT, "T_DC_Stall_Awning.png"))

# SOLD OUT texture, 1024 x 1024: top half = the sign's front (inked planks + the Meshy letters, outlined in ink),
# bottom half = plain planks for the back and edges; a dark patch bottom-right for the ropes
so = load_px(os.path.join(SRC, "SoldOut_meshy_image.png"))
front = resize(planks[128:512, 0:768], 1024, 512)
back = resize(planks[512:896, 256:1024], 1024, 512)
crop = so[235:525, 95:1305]                                   # the letters
LW = 880; LH = int(LW * crop.shape[0] / crop.shape[1])
let = resize(crop, LW, LH)
white = (let[..., :3].min(axis=2) > 0.70)
white = grow(white, 1) & (let[..., :3].min(axis=2) > 0.55)    # pull in the soft brush edges
y0, x0 = (512 - LH) // 2, (1024 - LW) // 2
full = np.zeros((512, 1024), dtype=bool); full[y0:y0 + LH, x0:x0 + LW] = white
shade = np.ones((512, 1024), dtype=np.float32); shade[y0:y0 + LH, x0:x0 + LW] = np.clip(let[..., :3].mean(axis=2) * 1.08, 0.78, 1.0)
ink = grow(full, 5) & ~full
front[ink, :3] = 0.03
front[full, :3] = (np.array([0.96, 0.93, 0.84], dtype=np.float32)[None, :] * shade[full][:, None])
soldout = np.concatenate([front, back], axis=0)
soldout[1000:1024, 1000:1024, :3] = (0.16, 0.10, 0.05)
save_px(soldout, os.path.join(OUT, "T_DC_SoldOutSign.png"))

# HA HA HA texture: the Meshy picture as it is, resampled to a power of two
ha = load_px(os.path.join(SRC, "HaHaHa_meshy_image.png"))
HA_W, HA_H = ha.shape[1], ha.shape[0]
save_px(resize(ha, 2048, 1024), os.path.join(OUT, "T_DC_LaughSign.png"))

# BUTCHER texture: the Meshy picture as it is, resampled to a power of two
bu = load_px(os.path.join(SRC, "Butcher_meshy_image.png"))
BU_W, BU_H = bu.shape[1], bu.shape[0]
save_px(resize(bu, 2048, 1024), os.path.join(OUT, "T_DC_ButcherSign.png"))


# ---------------------------------------------------------------- mesh helpers
class Mesh:
    def __init__(self, name, mats):
        self.name, self.mats = name, mats
        self.bm = bmesh.new()
        self.uvl = self.bm.loops.layers.uv.new("UVMap")

    def face(self, pts, uvs, mat, out):
        """pts in order around the face; flipped if needed so the normal points along `out`"""
        n = (Vector(pts[1]) - Vector(pts[0])).cross(Vector(pts[2]) - Vector(pts[0]))
        if n.dot(Vector(out)) < 0:
            pts, uvs = pts[::-1], uvs[::-1]
        f = self.bm.faces.new([self.bm.verts.new(p) for p in pts])
        f.material_index = self.mats.index(mat)
        for loop, uv in zip(f.loops, uvs):
            loop[self.uvl].uv = uv

    def box(self, lo, hi, mat, uv):
        """uv(point, axis) -> (u, v); axis = the face's normal axis (0 x, 1 y, 2 z)"""
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        faces = [((x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1), (-1, 0, 0), 0),
                 ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1), (1, 0, 0), 0),
                 ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, -1, 0), 1),
                 ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1), (0, 1, 0), 1),
                 ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (0, 0, -1), 2),
                 ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1), 2)]
        for a, b, c, d, out, ax in faces:
            self.face([a, b, c, d], [uv(p, ax) for p in (a, b, c, d)], mat, out)

    def finish(self):
        bmesh.ops.triangulate(self.bm, faces=self.bm.faces)
        me = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(me)
        self.bm.free()
        ob = bpy.data.objects.new(self.name, me)
        bpy.context.collection.objects.link(ob)
        for m in self.mats:
            me.materials.append(MATS[m])
        co = np.array([v.co[:] for v in me.vertices])
        print("MESH", self.name, "size m", (co.max(0) - co.min(0)).round(2).tolist(), "tris", len(me.polygons))
        return ob


def material(name, tex_path):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    tex = m.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(tex_path)
    m.node_tree.links.new(tex.outputs["Color"], m.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
    m.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
    m.use_backface_culling = False
    return m


WOOD_TEX = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export", "T_DC_SetStage_Planks.png")
MATS = {"M_Stall_Wood": material("M_Stall_Wood", WOOD_TEX),
        "M_Stall_Awning": material("M_Stall_Awning", os.path.join(OUT, "T_DC_Stall_Awning.png")),
        "M_SoldOutSign": material("M_SoldOutSign", os.path.join(OUT, "T_DC_SoldOutSign.png")),
        "M_LaughSign": material("M_LaughSign", os.path.join(OUT, "T_DC_LaughSign.png")),
        "M_ButcherSign": material("M_ButcherSign", os.path.join(OUT, "T_DC_ButcherSign.png"))}


def wood_h(p, ax):
    """planks lying flat: they run along the long horizontal side"""
    if ax == 0: return (p[1] / WOOD_M, p[2] / WOOD_M)
    if ax == 1: return (p[0] / WOOD_M, p[2] / WOOD_M)
    return (p[0] / WOOD_M, p[1] / WOOD_M)


def wood_v(p, ax):
    """posts: the grain runs up the post"""
    if ax == 0: return (p[2] / WOOD_M, p[1] / WOOD_M + 0.03)
    if ax == 1: return (p[2] / WOOD_M, p[0] / WOOD_M + 0.03)
    return (p[0] / WOOD_M, p[1] / WOOD_M)


# ---------------------------------------------------------------- 1: the stall (greybox: counter 8 x 4 x 1.1 m, posts at +-3.8, +-1.8)
st = Mesh("SM_DC_ButcherStall", ["M_Stall_Wood", "M_Stall_Awning"])
st.box((-4.0, -2.0, 0.0), (4.0, 2.0, 1.04), "M_Stall_Wood", wood_h)                      # counter body
st.box((-4.12, -2.12, 1.04), (4.12, 2.12, 1.10), "M_Stall_Wood", wood_h)                 # counter top, small overhang
st.box((-4.03, -2.03, 0.0), (4.03, 2.03, 0.14), "M_Stall_Wood", wood_h)                  # kick board
BACK_Y, FRONT_Y, BACK_Z, FRONT_Z = -2.7, 3.1, 3.40, 2.78                                 # the awning's slope


def roof_z(y):
    return BACK_Z + (y - BACK_Y) / (FRONT_Y - BACK_Y) * (FRONT_Z - BACK_Z)


for px_, py_ in ((-3.8, -1.8), (3.8, -1.8), (-3.8, 1.8), (3.8, 1.8)):
    st.box((px_ - 0.14, py_ - 0.14, 0.0), (px_ + 0.14, py_ + 0.14, roof_z(py_) - 0.03), "M_Stall_Wood", wood_v)
for py_ in (-1.8, 1.8):                                                                   # beams under the awning
    z = roof_z(py_)
    st.box((-4.25, py_ - 0.09, z - 0.26), (4.25, py_ + 0.09, z - 0.04), "M_Stall_Wood", wood_h)
for px_ in (-3.8, 3.8):                                                                   # side rafters, sloped
    a, b = (px_ - 0.07, -2.3), (px_ + 0.07, 2.6)
    pts_top = [(a[0], a[1], roof_z(a[1]) - 0.03), (b[0], a[1], roof_z(a[1]) - 0.03), (b[0], b[1], roof_z(b[1]) - 0.03), (a[0], b[1], roof_z(b[1]) - 0.03)]
    for dz, out in ((0.0, (0, 0, 1)), (-0.16, (0, 0, -1))):
        pts = [(p[0], p[1], p[2] + dz) for p in pts_top]
        st.face(pts, [(p[1] / WOOD_M, p[0] / WOOD_M) for p in pts], "M_Stall_Wood", out)
    for x, out in ((a[0], (-1, 0, 0)), (b[0], (1, 0, 0))):
        pts = [(x, a[1], roof_z(a[1]) - 0.19), (x, b[1], roof_z(b[1]) - 0.19), (x, b[1], roof_z(b[1]) - 0.03), (x, a[1], roof_z(a[1]) - 0.03)]
        st.face(pts, [(p[1] / WOOD_M, p[2] / WOOD_M) for p in pts], "M_Stall_Wood", out)
# awning: stripes run front to back; scalloped edge hanging at the front
RX = 4.75
slope = math.hypot(FRONT_Y - BACK_Y, FRONT_Z - BACK_Z)
st.face([(-RX, BACK_Y, BACK_Z), (RX, BACK_Y, BACK_Z), (RX, FRONT_Y, FRONT_Z), (-RX, FRONT_Y, FRONT_Z)],
        [(-RX / CLOTH_M, 0), (RX / CLOTH_M, 0), (RX / CLOTH_M, slope / CLOTH_M), (-RX / CLOTH_M, slope / CLOTH_M)], "M_Stall_Awning", (0, 0, 1))
NS = 19
v0 = slope / CLOTH_M
for i in range(NS):
    xa, xb = -RX + 2 * RX * i / NS, -RX + 2 * RX * (i + 1) / NS
    xm = (xa + xb) / 2
    st.face([(xa, FRONT_Y, FRONT_Z), (xb, FRONT_Y, FRONT_Z), (xb, FRONT_Y + 0.02, FRONT_Z - 0.22), (xm, FRONT_Y + 0.03, FRONT_Z - 0.42), (xa, FRONT_Y + 0.02, FRONT_Z - 0.22)],
            [(xa / CLOTH_M, v0), (xb / CLOTH_M, v0), (xb / CLOTH_M, v0 + 0.22 / CLOTH_M), (xm / CLOTH_M, v0 + 0.42 / CLOTH_M), (xa / CLOTH_M, v0 + 0.22 / CLOTH_M)],
            "M_Stall_Awning", (0, 1, 0))
stall = st.finish()


# ---------------------------------------------------------------- 2: SOLD OUT (hangs over the counter front; reads from +X to -X when seen from +Y)
def so_uv(u, v):                      # u, v in pixels of the 1024 x 1024 texture, v down
    return (u / 1024.0, 1.0 - v / 1024.0)


sg = Mesh("SM_DC_SoldOutSign", ["M_SoldOutSign"])
SW, SH, ST = 2.4, 1.2, 0.06           # origin = the board's centre
x0, x1, z0, z1 = -SW / 2, SW / 2, -SH / 2, SH / 2
sg.face([(x1, ST / 2, z1), (x0, ST / 2, z1), (x0, ST / 2, z0), (x1, ST / 2, z0)],
        [so_uv(0, 0), so_uv(1024, 0), so_uv(1024, 512), so_uv(0, 512)], "M_SoldOutSign", (0, 1, 0))
sg.face([(x0, -ST / 2, z1), (x1, -ST / 2, z1), (x1, -ST / 2, z0), (x0, -ST / 2, z0)],
        [so_uv(0, 512), so_uv(1024, 512), so_uv(1024, 1000), so_uv(0, 1000)], "M_SoldOutSign", (0, -1, 0))
edge = [so_uv(8, 600), so_uv(1016, 600), so_uv(1016, 625), so_uv(8, 625)]
sg.face([(x0, -ST / 2, z1), (x1, -ST / 2, z1), (x1, ST / 2, z1), (x0, ST / 2, z1)], edge, "M_SoldOutSign", (0, 0, 1))
sg.face([(x0, -ST / 2, z0), (x1, -ST / 2, z0), (x1, ST / 2, z0), (x0, ST / 2, z0)], edge, "M_SoldOutSign", (0, 0, -1))
sg.face([(x0, -ST / 2, z0), (x0, -ST / 2, z1), (x0, ST / 2, z1), (x0, ST / 2, z0)], edge, "M_SoldOutSign", (-1, 0, 0))
sg.face([(x1, -ST / 2, z0), (x1, -ST / 2, z1), (x1, ST / 2, z1), (x1, ST / 2, z0)], edge, "M_SoldOutSign", (1, 0, 0))
rope = lambda p, ax: so_uv(1012, 1012)
for rx in (-0.95, 0.95):              # two ropes up to the awning
    sg.box((rx - 0.02, -0.02, z1), (rx + 0.02, 0.02, z1 + 0.55), "M_SoldOutSign", rope)
soldout_ob = sg.finish()


# ---------------------------------------------------------------- 3: HA HA HA (two poles, board in front of them; origin on the ground between the poles)
def ha_uv(u, v):                      # pixels of the original Meshy picture, v down
    return (u / HA_W, 1.0 - v / HA_H)


lg = Mesh("SM_DC_LaughSign", ["M_LaughSign"])
BW = 12.6
BH = BW * (665 - 100) / (1364 - 12)   # the cropped picture's shape -> about 5.3 m
BZ0 = 11.2 - BH                       # top edge at 11.2 m
bx0, bx1, by0, by1 = -BW / 2, BW / 2, 0.0, 0.25
lg.face([(bx1, by1, BZ0 + BH), (bx0, by1, BZ0 + BH), (bx0, by1, BZ0), (bx1, by1, BZ0)],
        [ha_uv(12, 100), ha_uv(1364, 100), ha_uv(1364, 665), ha_uv(12, 665)], "M_LaughSign", (0, 1, 0))
NB = 6                                # back: plain plank strips
for i in range(NB):
    za, zb = BZ0 + BH * i / NB, BZ0 + BH * (i + 1) / NB
    lg.face([(bx0, by0, zb), (bx1, by0, zb), (bx1, by0, za), (bx0, by0, za)],
            [ha_uv(20, 672), ha_uv(1356, 672), ha_uv(1356, 752), ha_uv(20, 752)], "M_LaughSign", (0, -1, 0))
strip = [ha_uv(40, 22), ha_uv(1340, 22), ha_uv(1340, 92), ha_uv(40, 92)]
lg.face([(bx0, by0, BZ0 + BH), (bx1, by0, BZ0 + BH), (bx1, by1, BZ0 + BH), (bx0, by1, BZ0 + BH)], strip, "M_LaughSign", (0, 0, 1))
lg.face([(bx0, by0, BZ0), (bx1, by0, BZ0), (bx1, by1, BZ0), (bx0, by1, BZ0)], strip, "M_LaughSign", (0, 0, -1))
lg.face([(bx0, by0, BZ0), (bx0, by0, BZ0 + BH), (bx0, by1, BZ0 + BH), (bx0, by1, BZ0)], strip, "M_LaughSign", (-1, 0, 0))
lg.face([(bx1, by0, BZ0), (bx1, by0, BZ0 + BH), (bx1, by1, BZ0 + BH), (bx1, by1, BZ0)], strip, "M_LaughSign", (1, 0, 0))
PT = BZ0 + BH - 0.4                   # pole top, hidden behind the board
for pxx in (-5.0, 5.0):
    a, b = (pxx - 0.25, -0.5), (pxx + 0.25, 0.0)
    for (p, q, out) in (((a[0], a[1]), (b[0], a[1]), (0, -1, 0)), ((a[0], b[1]), (b[0], b[1]), (0, 1, 0)),
                        ((a[0], a[1]), (a[0], b[1]), (-1, 0, 0)), ((b[0], a[1]), (b[0], b[1]), (1, 0, 0))):
        lg.face([(p[0], p[1], 0), (p[0], p[1], PT), (q[0], q[1], PT), (q[0], q[1], 0)], strip, "M_LaughSign", out)
    lg.face([(a[0], a[1], PT), (b[0], a[1], PT), (b[0], b[1], PT), (a[0], b[1], PT)], strip, "M_LaughSign", (0, 0, 1))
laugh_ob = lg.finish()

# ---------------------------------------------------------------- 4: BUTCHER (the stall's name board, on two stubs above the back of the awning;
#                                                                     same origin as the stall, so it is placed at the stall's spot)
def bu_uv(u, v):                      # pixels of the original Meshy picture, v down
    return (u / BU_W, 1.0 - v / BU_H)


bs = Mesh("SM_DC_ButcherSign", ["M_ButcherSign"])
NW = 5.4
NH = NW * (592 - 172) / (1352 - 24)   # the middle three planks of the picture -> about 1.7 m
NZ0, NY1, NT = 3.55, -1.75, 0.10      # bottom edge, front face, thickness
nx0, nx1, ny0 = -NW / 2, NW / 2, NY1 - NT
bs.face([(nx1, NY1, NZ0 + NH), (nx0, NY1, NZ0 + NH), (nx0, NY1, NZ0), (nx1, NY1, NZ0)],
        [bu_uv(24, 172), bu_uv(1352, 172), bu_uv(1352, 592), bu_uv(24, 592)], "M_ButcherSign", (0, 1, 0))
for i in range(3):                    # back: plain plank strips
    za, zb = NZ0 + NH * i / 3, NZ0 + NH * (i + 1) / 3
    bs.face([(nx0, ny0, zb), (nx1, ny0, zb), (nx1, ny0, za), (nx0, ny0, za)],
            [bu_uv(330, 600), bu_uv(1340, 600), bu_uv(1340, 718), bu_uv(330, 718)], "M_ButcherSign", (0, -1, 0))
nstrip = [bu_uv(340, 60), bu_uv(1040, 60), bu_uv(1040, 150), bu_uv(340, 150)]
bs.face([(nx0, ny0, NZ0 + NH), (nx1, ny0, NZ0 + NH), (nx1, NY1, NZ0 + NH), (nx0, NY1, NZ0 + NH)], nstrip, "M_ButcherSign", (0, 0, 1))
bs.face([(nx0, ny0, NZ0), (nx1, ny0, NZ0), (nx1, NY1, NZ0), (nx0, NY1, NZ0)], nstrip, "M_ButcherSign", (0, 0, -1))
bs.face([(nx0, ny0, NZ0), (nx0, ny0, NZ0 + NH), (nx0, NY1, NZ0 + NH), (nx0, NY1, NZ0)], nstrip, "M_ButcherSign", (-1, 0, 0))
bs.face([(nx1, ny0, NZ0), (nx1, ny0, NZ0 + NH), (nx1, NY1, NZ0 + NH), (nx1, NY1, NZ0)], nstrip, "M_ButcherSign", (1, 0, 0))
for sx_ in (-2.1, 2.1):               # stubs from the back beam up through the cloth
    a, b = (sx_ - 0.09, ny0 - 0.16), (sx_ + 0.09, ny0)
    for (p, q, out) in (((a[0], a[1]), (b[0], a[1]), (0, -1, 0)), ((a[0], b[1]), (b[0], b[1]), (0, 1, 0)),
                        ((a[0], a[1]), (a[0], b[1]), (-1, 0, 0)), ((b[0], a[1]), (b[0], b[1]), (1, 0, 0))):
        bs.face([(p[0], p[1], 3.05), (p[0], p[1], NZ0 + 0.9), (q[0], q[1], NZ0 + 0.9), (q[0], q[1], 3.05)], nstrip, "M_ButcherSign", out)
butcher_ob = bs.finish()

# ---------------------------------------------------------------- export
for ob in (stall, soldout_ob, laugh_ob, butcher_ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, ob.name + ".glb"), export_format="GLB", use_selection=True,
                              export_apply=True, export_image_format="NONE")
# lay them out like in the level for the .blend and the previews
soldout_ob.location = (0, 2.25, 1.80)
laugh_ob.location = (0, -3.0, 0)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "ButcherStall.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng; break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 1000, 640
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.7, 0.8, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(55), 0, math.radians(200)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("Stall_Front", (2.5, 12.0, 2.2), (0, 0, 1.8))
    shoot("Stall_Angle", (9.0, 9.0, 4.5), (0, 0, 1.6))
    shoot("Stall_All", (3.0, 26.0, 3.0), (0, 0, 5.5))
    shoot("Stall_Back", (-7.0, -14.0, 5.0), (0, -2, 4.0))
print("DONE")
