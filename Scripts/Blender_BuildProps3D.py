# Director's Cut - Taisho's hay bale, grass and apple tree drawings turned into real 3D props (Forgive Me Father style:
# simple geometry, every surface painted with the hand-drawn picture, ink lines included).
#   blender --background --python Scripts/Blender_BuildProps3D.py [-- preview]
# Writes, per prop, Art/Meshy/<Prop>/Export/: SM_*.glb (geometry + UVs, no embedded images), the cleaned textures
# (sticker-white keyed out, wooden stand cropped off) and SM_*.blend. With "preview": renders each prop from 4 sides to
# Saved/PropPreview/<Prop>.png so every side can be checked.
#   - Hay: lying round bale (spiral end caps, rolled sides, rounded rims) + square bale + pitchfork leaning on it.
#   - Grass: a tuft of 5 crossed cut-out cards (the drawing's own blades, white keyed out) -> a full clump from any side.
#   - Apple tree: flared trunk + 5 branches (bark from the drawing), a lumpy canopy of leaf-painted blobs (leaves + apples
#     tile), the drawing's leafy outline as crossed cut-out cards for the silhouette, fallen apples and a grass skirt.
import bpy, bmesh, math, os, random, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
ART = os.path.join(ROOT, "Art", "Meshy")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
PREVIEW_DIR = os.path.join(ROOT, "Saved", "PropPreview")
rng = random.Random(1114)

# ---------------------------------------------------------------- textures
def load_px(path):
    img = bpy.data.images.load(path)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()   # row 0 = top
    bpy.data.images.remove(img)
    return px


def key_white(px, lum=0.86, sat=0.12):
    """The cut-outs sit on white sticker card: make near-white, unsaturated pixels transparent."""
    rgb = px[..., :3]
    mx, mn = rgb.max(-1), rgb.min(-1)
    white = (rgb.mean(-1) > lum) & ((mx - mn) < sat)
    px[..., 3] = np.where(white, 0.0, px[..., 3])
    return px


def save_px(px, path):
    H, W = px.shape[:2]
    img = bpy.data.images.new(os.path.basename(path), W, H, alpha=True)
    img.pixels[:] = px[::-1].ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)
    return W, H


# ---------------------------------------------------------------- mesh building
class Builder:
    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.uvl = self.bm.loops.layers.uv.new("UVMap")
        self.mats = []

    def mat_index(self, m):
        if m not in self.mats:
            self.mats.append(m)
        return self.mats.index(m)

    def face(self, pts, uvs, mat):
        vs = [self.bm.verts.new(p) for p in pts]
        f = self.bm.faces.new(vs)
        f.material_index = self.mat_index(mat)
        for loop, uv in zip(f.loops, uvs):
            loop[self.uvl].uv = uv
        return f

    def finish(self, materials):
        me = bpy.data.meshes.new(self.name)
        bmesh.ops.remove_doubles(self.bm, verts=self.bm.verts, dist=0.0001)
        self.bm.to_mesh(me)
        self.bm.free()
        for f in me.polygons:
            f.use_smooth = True
        ob = bpy.data.objects.new(self.name, me)
        bpy.context.collection.objects.link(ob)
        for m in self.mats:
            ob.data.materials.append(materials[m])
        return ob


def region_uv(img_wh, region):
    """pixel region (x0,y0,x1,y1, y down) -> function (u,v in 0..1, v up) -> texture uv"""
    W, H = img_wh
    x0, y0, x1, y1 = region
    return lambda u, v: ((x0 + (x1 - x0) * u) / W, 1.0 - (y1 - (y1 - y0) * v) / H)


def make_material(name, tex_path, cutout=True):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(tex_path)
    tex.interpolation = "Linear"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    if cutout:
        nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        for attr, val in (("surface_render_method", "DITHERED"), ("blend_method", "CLIP")):
            try:
                setattr(m, attr, val)
            except Exception:
                pass
    bsdf.inputs["Roughness"].default_value = 0.9
    m.use_backface_culling = False
    return m


def ring_pts(c, axis_u, axis_v, r, n, a0=0.0):
    return [tuple(c[i] + r * (math.cos(a0 + 2 * math.pi * k / n) * axis_u[i] + math.sin(a0 + 2 * math.pi * k / n) * axis_v[i]) for i in range(3)) for k in range(n)]


def tube(b, p0, p1, r0, r1, n, mat, uvf, repeats=1.0, cap=True):
    """tapered cylinder from p0 to p1; uvf(u,v) maps (around, along) in 0..1 onto a texture region"""
    d = [p1[i] - p0[i] for i in range(3)]
    L = math.sqrt(sum(x * x for x in d)) or 1.0
    w = [x / L for x in d]
    a = (0, 0, 1) if abs(w[2]) < 0.9 else (1, 0, 0)
    u_ = (w[1] * a[2] - w[2] * a[1], w[2] * a[0] - w[0] * a[2], w[0] * a[1] - w[1] * a[0])
    ul = math.sqrt(sum(x * x for x in u_)); u_ = [x / ul for x in u_]
    v_ = (w[1] * u_[2] - w[2] * u_[1], w[2] * u_[0] - w[0] * u_[2], w[0] * u_[1] - w[1] * u_[0])
    A, B = ring_pts(p0, u_, v_, r0, n), ring_pts(p1, u_, v_, r1, n)
    per = max(1, int(round(n / repeats)))
    for k in range(n):
        k2 = (k + 1) % n
        s0 = (k % per) / per
        s1 = s0 + 1.0 / per
        b.face([A[k], A[k2], B[k2], B[k]], [uvf(s0, 0), uvf(s1, 0), uvf(s1, 1), uvf(s0, 1)], mat)
    if cap:
        b.face(list(reversed(A)), [uvf(0.5, 0.05)] * n, mat)
        b.face(B, [uvf(0.5, 0.95)] * n, mat)


def card(b, center, width, height, yaw_deg, tilt_deg, mat, uvf, sink=0.0):
    """vertical cut-out card standing on the ground at center, rotated yaw, leaning by tilt around its own width axis"""
    yw, tl = math.radians(yaw_deg), math.radians(tilt_deg)
    ax = (math.cos(yw), math.sin(yw), 0.0)                    # width direction
    nx = (-math.sin(yw), math.cos(yw), 0.0)                   # card normal
    up = (nx[0] * math.sin(tl), nx[1] * math.sin(tl), math.cos(tl))
    cx, cy, cz = center
    cz -= sink
    def P(s, t):
        return (cx + ax[0] * s * width + up[0] * t * height, cy + ax[1] * s * width + up[1] * t * height, cz + up[2] * t * height)
    b.face([P(-0.5, 0), P(0.5, 0), P(0.5, 1), P(-0.5, 1)], [uvf(0, 0), uvf(1, 0), uvf(1, 1), uvf(0, 1)], mat)


def blob(b, center, radius, mat, tile_m, noise=0.1, subdiv=2):
    """lumpy icosphere; box-projected world-space UVs so a tiling leaf texture wraps every side evenly"""
    tmp = bmesh.new()
    bmesh.ops.create_icosphere(tmp, subdivisions=subdiv, radius=radius)
    for v in tmp.verts:
        n = v.co.normalized()
        k = 1.0 + noise * (math.sin(n.x * 5.1 + center[0]) * math.cos(n.y * 4.3 + center[1]) + 0.5 * math.sin(n.z * 7.7))
        v.co = v.co * k
        if v.co.z < -radius * 0.55:                          # flatter underside, like a real canopy
            v.co.z = -radius * 0.55 + (v.co.z + radius * 0.55) * 0.35
    for f in tmp.faces:
        n = f.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        pts, uvs = [], []
        for v in f.verts:
            w = (v.co.x + center[0], v.co.y + center[1], v.co.z + center[2])
            pts.append(w)
            c = [w[i] for i in range(3) if i != axis]
            uvs.append((c[0] / tile_m, c[1] / tile_m))
        b.face(pts, uvs, mat)
    tmp.free()


def export(ob, out_dir, name):
    os.makedirs(out_dir, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.gltf(filepath=os.path.join(out_dir, name + ".glb"), export_format="GLB", use_selection=True,
                              export_apply=True, export_image_format="NONE")


# ================================================================ HAY
def build_hay():
    src = os.path.join(ART, "HayBale", "hay_bale_01.png")
    out = os.path.join(ART, "HayBale", "Export")
    os.makedirs(out, exist_ok=True)
    px = key_white(load_px(src))
    tex = os.path.join(out, "T_DC_HayBale01_3D.png")
    wh = save_px(px, tex)
    mats = {"hay": make_material("M_HayBale01_3D", tex)}
    b = Builder("SM_DC_HayBale01_3D")
    SIDE = region_uv(wh, (765, 175, 1000, 555))        # rolled hay, no fork
    BALE_END = region_uv(wh, (842, 612, 985, 858))     # square bale end
    BALE_SIDE = region_uv(wh, (1002, 612, 1148, 858))  # square bale long side (left of the fork)
    HANDLE = region_uv(wh, (1062, 282, 1074, 302))     # fork handle (brown)
    TINE = region_uv(wh, (1176, 760, 1186, 790))       # fork ferrule / tines (dark rusty)
    cx, cy, rx, ry = 412.0, 485.0, 232.0, 288.0        # spiral end face ellipse in the drawing
    W, H = wh
    def cap_uv(a, f):
        return ((cx + rx * f * math.cos(a)) / W, 1.0 - (cy - ry * f * math.sin(a)) / H)

    # --- round bale lying along Y: radius 0.78, length 1.45, rounded rims, slightly squashed by its own weight
    R, L, N = 0.78, 1.45, 24
    ox = -0.35
    def P(a, r, y, squash=0.94):
        return (ox + r * math.cos(a), y, R * squash + r * math.sin(a) * squash)
    rings = [(-L / 2, R * 0.86), (-L / 2 + 0.06, R * 0.98), (-L / 2 + 0.16, R), (L / 2 - 0.16, R), (L / 2 - 0.06, R * 0.98), (L / 2, R * 0.86)]
    per = N // 4                                        # side texture repeats 4x around
    for i in range(len(rings) - 1):
        (y0, r0), (y1, r1) = rings[i], rings[i + 1]
        v0, v1 = (y0 + L / 2) / L, (y1 + L / 2) / L
        for k in range(N):
            a0, a1 = 2 * math.pi * k / N, 2 * math.pi * (k + 1) / N
            s0 = (k % per) / per; s1 = s0 + 1.0 / per
            b.face([P(a0, r0, y0), P(a1, r0, y0), P(a1, r1, y1), P(a0, r1, y1)],
                   [SIDE(v0, s0), SIDE(v0, s1), SIDE(v1, s1), SIDE(v1, s0)], "hay")
    # spiral end caps (slightly domed), the drawing's spiral painted on both
    for sgn, (yc, rc) in ((-1, rings[0]), (1, rings[-1])):
        ring2 = [P(2 * math.pi * k / N, rc * 0.55, yc + sgn * 0.03) for k in range(N)]
        ring1 = [P(2 * math.pi * k / N, rc, yc) for k in range(N)]
        ctr = P(0, 0, yc + sgn * 0.05)
        for k in range(N):
            k2 = (k + 1) % N
            a0, a1 = 2 * math.pi * k / N, 2 * math.pi * (k + 1) / N
            m0, m1 = (a0, a1) if sgn > 0 else (-a0, -a1)       # mirror so the spiral reads correctly on the far end
            q = [ring1[k], ring1[k2], ring2[k2], ring2[k]]
            u = [cap_uv(m0, 1), cap_uv(m1, 1), cap_uv(m1, 0.55), cap_uv(m0, 0.55)]
            if sgn < 0:
                q, u = q[::-1], u[::-1]
            b.face(q, u, "hay")
            t = [ring2[k], ring2[k2], ctr]
            tu = [cap_uv(m0, 0.55), cap_uv(m1, 0.55), ((cx - 18) / W, 1.0 - (cy + 12) / H)]
            if sgn < 0:
                t, tu = t[::-1], tu[::-1]
            b.face(t, tu, "hay")

    # --- square bale in front of the round one: 1.0 x 0.5 x 0.45 m, end faces + tiled long sides/top
    bx, by, bw, bd, bh = 0.55, -0.15, 1.0, 0.5, 0.45
    yaw = math.radians(-12)
    def Q(x, y, z):
        return (bx + x * math.cos(yaw) - y * math.sin(yaw), by + x * math.sin(yaw) + y * math.cos(yaw), z)
    X0, X1, Y0, Y1 = -bw / 2, bw / 2, -bd / 2, bd / 2
    for x in (X0, X1):     # ends
        pts = [Q(x, Y0, 0), Q(x, Y1, 0), Q(x, Y1, bh), Q(x, Y0, bh)]
        uvs = [BALE_END(0, 0), BALE_END(1, 0), BALE_END(1, 1), BALE_END(0, 1)]
        if x == X1:
            pts, uvs = [pts[1], pts[0], pts[3], pts[2]], [uvs[0], uvs[1], uvs[2], uvs[3]]
        else:
            pts = pts[::-1]; uvs = uvs[::-1]
        b.face(pts, uvs, "hay")
    for half in (0, 1):    # long sides and top, texture twice along the length
        xa, xb = X0 + half * bw / 2, X0 + (half + 1) * bw / 2
        b.face([Q(xa, Y0, 0), Q(xb, Y0, 0), Q(xb, Y0, bh), Q(xa, Y0, bh)], [BALE_SIDE(0, 0), BALE_SIDE(1, 0), BALE_SIDE(1, 1), BALE_SIDE(0, 1)], "hay")
        b.face([Q(xb, Y1, 0), Q(xa, Y1, 0), Q(xa, Y1, bh), Q(xb, Y1, bh)], [BALE_SIDE(0, 0), BALE_SIDE(1, 0), BALE_SIDE(1, 1), BALE_SIDE(0, 1)], "hay")
        b.face([Q(xa, Y0, bh), Q(xb, Y0, bh), Q(xb, Y1, bh), Q(xa, Y1, bh)], [BALE_SIDE(0, 0), BALE_SIDE(1, 0), BALE_SIDE(1, 1), BALE_SIDE(0, 1)], "hay")

    # --- pitchfork leaning on the round bale: handle + ferrule + crossbar + 4 tines
    foot = (0.95, -0.55, 0.0)
    top = (0.55, -0.10, 1.45)
    tube(b, (foot[0], foot[1], 0.30), top, 0.02, 0.018, 8, "hay", HANDLE)
    tube(b, (foot[0], foot[1], 0.24), (foot[0] - 0.012, foot[1] + 0.013, 0.31), 0.026, 0.022, 8, "hay", TINE)
    tube(b, (foot[0] - 0.13, foot[1] - 0.12, 0.24), (foot[0] + 0.13, foot[1] + 0.12, 0.24), 0.012, 0.012, 6, "hay", TINE)
    for k in range(4):
        f = -0.105 + 0.07 * k
        tx, ty = foot[0] + f * 0.73, foot[1] + f * 0.68
        tube(b, (tx, ty, 0.24), (tx + 0.01, ty - 0.01, 0.0), 0.009, 0.006, 6, "hay", TINE)
    ob = b.finish(mats)
    export(ob, out, "SM_DC_HayBale01_3D")
    return ob


# ================================================================ GRASS
def build_grass():
    src = os.path.join(ART, "Grass", "grass_01.png")
    out = os.path.join(ART, "Grass", "Export")
    os.makedirs(out, exist_ok=True)
    px = key_white(load_px(src))
    px = px[30:1138, 30:1590]                          # blades only: the wooden stand is cropped off
    tex = os.path.join(out, "T_DC_Grass01_3D.png")
    wh = save_px(px, tex)
    mats = {"grass": make_material("M_Grass01_3D", tex)}
    b = Builder("SM_DC_Grass01_3D")
    FULL = region_uv(wh, (0, 0, wh[0], wh[1]))
    w = 1.25
    h = w * wh[1] / wh[0]
    # 3 big crossed cards + 2 smaller ones, each leaning a little outward: a round clump from every side
    for yaw, scale, tilt, off in ((0, 1.0, 6, (0, 0)), (60, 0.95, -7, (0.04, 0.03)), (120, 0.92, 8, (-0.03, 0.04)),
                                  (30, 0.7, -10, (0.12, -0.08)), (150, 0.68, 9, (-0.1, -0.06))):
        card(b, (off[0], off[1], 0.0), w * scale, h * scale, yaw, tilt, "grass", FULL, sink=0.02)
    ob = b.finish(mats)
    export(ob, out, "SM_DC_Grass01_3D")
    return ob


# ================================================================ APPLE TREE
def build_tree():
    src = os.path.join(ART, "AppleTree", "apple_tree_02.png")
    out = os.path.join(ART, "AppleTree", "Export")
    os.makedirs(out, exist_ok=True)
    px = key_white(load_px(src))
    atlas = os.path.join(out, "T_DC_AppleTree02_3D.png")
    wh = save_px(px, atlas)
    # tiling leaf texture: the inside of the canopy (leaves + apples), mirrored-padded so it repeats softly
    leaf = load_px(src)[330:690, 400:1050].copy()      # inside the canopy only (no sticker-white gaps)
    leaf[..., 3] = 1.0
    leaves = os.path.join(out, "T_DC_AppleTree02_Leaves.png")
    save_px(leaf, leaves)
    mats = {"atlas": make_material("M_AppleTree02_3D", atlas), "leaves": make_material("M_AppleTree02_Leaves", leaves, cutout=False)}
    b = Builder("SM_DC_AppleTree02_3D")
    BARK = region_uv(wh, (684, 885, 788, 1150))
    CANOPY = region_uv(wh, (200, 120, 1240, 880))
    APPLE = region_uv(wh, (626, 348, 656, 382))
    SKIRT = region_uv(wh, (400, 1150, 1100, 1330))

    # trunk: flared base, slight lean, 12 sides
    tube(b, (0, 0, -0.05), (0, 0, 0.35), 0.42, 0.27, 12, "atlas", BARK, repeats=3, cap=False)
    tube(b, (0, 0, 0.35), (0.06, 0.03, 2.55), 0.27, 0.19, 12, "atlas", BARK, repeats=3, cap=False)
    # branches up into the canopy
    base = (0.06, 0.03, 2.4)
    for yawd, pitch, L in ((15, 42, 1.5), (95, 38, 1.4), (175, 45, 1.45), (255, 40, 1.35), (320, 60, 1.2)):
        y, p = math.radians(yawd), math.radians(pitch)
        end = (base[0] + L * math.cos(p) * math.cos(y), base[1] + L * math.cos(p) * math.sin(y), base[2] + L * math.sin(p))
        tube(b, base, end, 0.12, 0.05, 8, "atlas", BARK, repeats=2)
    # canopy: a lumpy core of leaf-painted blobs (kept inside the outline so the drawn leafy silhouette leads)
    cz = 3.85
    for (x, y, z, r) in ((0, 0, 0.2, 1.05), (0.8, 0.15, -0.05, 0.8), (-0.75, -0.1, 0.0, 0.82), (0.1, 0.75, -0.05, 0.78),
                         (-0.15, -0.8, 0.0, 0.78), (0.4, -0.4, 0.5, 0.7), (-0.4, 0.4, 0.45, 0.72)):
        blob(b, (x, y, cz + z), r, "leaves", tile_m=1.8, noise=0.08)
    # the drawing's leafy outline as 4 crossed cut-out cards: the canopy's hand-drawn silhouette from any side
    cw, ch = 4.3, 4.3 * (880 - 120) / (1240 - 200)
    for yawd in (0, 45, 90, 135):
        card(b, (0, 0, cz - ch * 0.6), cw, ch, yawd, 0, "atlas", CANOPY)
    # grass skirt + fallen apples at the foot
    for yawd in (20, 110):
        card(b, (0, 0, 0), 2.4, 2.4 * (1330 - 1150) / (1100 - 400), yawd, 0, "atlas", SKIRT, sink=0.03)
    for (x, y) in ((0.75, 0.45), (-0.6, 0.7), (0.35, -0.8), (-0.85, -0.35)):
        tmp = bmesh.new()
        bmesh.ops.create_uvsphere(tmp, u_segments=10, v_segments=6, radius=0.075)
        for f in tmp.faces:
            pts = [(v.co.x + x, v.co.y + y, v.co.z + 0.07) for v in f.verts]
            uvs = [APPLE(0.5 + 0.45 * v.co.x / 0.075, 0.5 + 0.45 * v.co.z / 0.075) for v in f.verts]
            b.face(pts, uvs, "atlas")
        tmp.free()
    ob = b.finish(mats)
    export(ob, out, "SM_DC_AppleTree02_3D")
    return ob


# ================================================================ preview renders
def preview(ob, name, dist, height, look_z):
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng
            break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 520, 520
    scn.render.film_transparent = False
    if not scn.world:
        scn.world = bpy.data.worlds.new("W")
    scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.55, 0.62, 0.7, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 0.9
    if "Sun" not in bpy.data.objects:
        sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
        sun.data.energy = 3.5
        sun.rotation_euler = (math.radians(50), 0, math.radians(35))
        scn.collection.objects.link(sun)
        g = bpy.data.objects.new("Ground", bpy.data.meshes.new("G"))
        gb = bmesh.new(); bmesh.ops.create_grid(gb, x_segments=1, y_segments=1, size=12); gb.to_mesh(g.data); gb.free()
        gm = bpy.data.materials.new("GM"); gm.use_nodes = True
        gm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.42, 0.5, 0.3, 1)
        g.data.materials.append(gm)
        scn.collection.objects.link(g)
    cam = bpy.data.objects.get("Cam") or bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    if cam.name not in scn.collection.objects:
        scn.collection.objects.link(cam)
    scn.camera = cam
    for o in bpy.data.objects:
        if o.type == "MESH" and o.name not in ("Ground",):
            o.hide_render = (o != ob)
    shots = []
    for k, yawd in enumerate((-90, 0, 90, 180)):
        y = math.radians(yawd)
        cam.location = (dist * math.cos(y), dist * math.sin(y), height)
        d = (-cam.location[0], -cam.location[1], look_z - height)
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        f = os.path.join(PREVIEW_DIR, "%s_%d.png" % (name, k))
        scn.render.filepath = f
        bpy.ops.render.render(write_still=True)
        shots.append(f)
    # side-by-side sheet
    tiles = [load_px(f) for f in shots]
    sheet = np.concatenate(tiles, axis=1)
    save_px(sheet, os.path.join(PREVIEW_DIR, name + ".png"))
    for f in shots:
        os.remove(f)


bpy.ops.wm.read_factory_settings(use_empty=True)
hay = build_hay()
grass = build_grass()
tree = build_tree()
for o, x in ((hay, -8), (grass, 0), (tree, 8)):
    o.location.x = 0
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ART, "Props3D_Taisho.blend"))
if PREVIEW:
    preview(hay, "HayBale01", 4.2, 1.6, 0.6)
    preview(grass, "Grass01", 2.6, 1.0, 0.35)
    preview(tree, "AppleTree02", 11.0, 3.2, 2.9)
for o in (hay, grass, tree):
    print("BUILT", o.name, len(o.data.vertices), "verts", len(o.data.polygons), "faces", [m.name for m in o.data.materials])
