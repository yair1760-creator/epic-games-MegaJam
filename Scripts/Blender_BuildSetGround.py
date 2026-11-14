# Director's Cut - the whole playing platform's floor, built in Blender from Meshy output in the Forgive Me Father
# comic style: cobblestone from a Meshy 3D tile (text-to-3D meshy-t2.5 + texture), grass and dirt from Meshy
# text-to-image ground pictures (the 3D grass tile failed - loose tufts only - and the 3D dirt tile came out grey).
#   blender --background --python Scripts/Blender_BuildSetGround.py [-- preview]
# In : Art/Meshy/Ground/Source/Cobble_meshy.glb, Grass_meshy_image.png, Dirt_meshy_image.png   (untouched Meshy output;
#      the unused Grass_meshy.glb / Dirt_meshy.glb 3D attempts are kept there too)
# Out: Art/Meshy/Ground/Export/T_DC_Ground_{Grass,Dirt,Cobble}.png         (seamless, repeatable)
#      Art/Meshy/Ground/Export/SM_DC_SetGround.glb + .blend                (ONE floor mesh for the platform, 3 materials)
# Steps:
#   1. the cobblestone tile is photographed square-on, shadeless, over its inner square (no slab edges)
#   2. every picture is made to repeat with no seams: used as it is when its edges already match, otherwise mirrored
#   3. the floor is built to the level's own layout (LAYOUT below = the greybox pieces it covers, Unreal cm):
#      meadow rectangle (grass), six path strips (dirt), village square (cobblestone); UVs repeat every TILE_M metres
# The floor is visual only (no collision): in the level it lies a few mm over the greybox ground, which keeps
# doing collision and navigation.
import bpy, bmesh, math, os, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "Ground", "Source")
OUT = os.path.join(ROOT, "Art", "Meshy", "Ground", "Export")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
os.makedirs(OUT, exist_ok=True)
RES = 1024                      # top-down photo size; the mirrored texture is 2 x RES
INNER = 0.80                    # fraction of the tile photographed (skips the slab's edges)
TILE_M = {"Grass": 8.0, "Dirt": 3.5, "Cobble": 6.0}     # metres one texture covers (dirt = the path's width)

# Unreal layout (cm): name, centre x, y, half length, half width, yaw, top z
MEADOW = (0.0, 0.0, 6000.0, 5000.0, 0.0, 0.0)
SQUARE = (-4700.0, 0.0, 1300.0, 2600.0, 0.0, 3.0)
PATHS = [(-2700.0, 0.0, 2650.0, 175.0, 0.0, 5.0), (3900.0, 0.0, 1850.0, 175.0, 0.0, 5.0),
         (-2250.0, 1100.0, 1278.05, 175.0, 102.804, 5.0), (2250.0, 1650.0, 1001.47, 175.0, 49.764, 5.0),
         (2100.0, -1850.0, 1089.41, 175.0, -64.799, 5.0), (-1000.0, -2200.0, 2350.0, 175.0, -90.0, 5.0)]
LIFT = 0.6                      # cm above the greybox surface it covers
PATH_STEP = 0.5                 # cm: every path strip sits this much above the one before it. The strips overlap at the
                                # junctions; at the same height the two overlapping faces flicker (z-fighting) as the camera moves


def set_engine(scn):
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng
            return
        except Exception:
            pass


def photograph(name):
    """import one Meshy tile, render it shadeless from straight above, return pixels (row 0 = top)"""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, name + "_meshy.glb"))
    obs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    co = np.array([o.matrix_world @ v.co for o in obs for v in o.data.vertices])
    mn, mx = co.min(0), co.max(0)
    size = mx - mn
    thin = int(np.argmin(size))                      # the slab's thickness axis (Meshy does not always lay it flat)
    wide = [i for i in range(3) if i != thin]
    side = min(size[wide[0]], size[wide[1]]) * INNER
    c = (mn + mx) / 2
    # shadeless: the texture goes straight to emission
    for o in obs:
        for m in o.data.materials:
            nt = m.node_tree
            tex = next((n for n in nt.nodes if n.type == "TEX_IMAGE" and n.image), None)
            out = next(n for n in nt.nodes if n.type == "OUTPUT_MATERIAL")
            em = nt.nodes.new("ShaderNodeEmission")
            if tex:
                nt.links.new(tex.outputs["Color"], em.inputs["Color"])
            nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    scn = bpy.context.scene
    set_engine(scn)
    scn.render.resolution_x = scn.render.resolution_y = RES
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W")
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = side
    if thin == 2:                                     # lying flat: look straight down
        cam.location = (c[0], c[1], mx[2] + 2.0)
    elif thin == 1:                                   # standing up facing -Y: look at it from the front
        cam.location = (c[0], mn[1] - 2.0, c[2])
        cam.rotation_euler = (math.radians(90), 0, 0)
    else:
        cam.location = (mn[0] - 2.0, c[1], c[2])
        cam.rotation_euler = (math.radians(90), 0, math.radians(-90))
    scn.collection.objects.link(cam)
    scn.camera = cam
    tmp = os.path.join(OUT, "_top_%s.png" % name)
    scn.render.filepath = tmp
    bpy.ops.render.render(write_still=True)
    img = bpy.data.images.load(tmp)
    px = np.array(img.pixels[:], dtype=np.float32).reshape(RES, RES, 4)[::-1].copy()
    bpy.data.images.remove(img)
    os.remove(tmp)
    print("PHOTO", name, "tile size", (mx - mn).round(3).tolist(), "inner", round(float(side), 3))
    return px


def save_px(px, path):
    H, W = px.shape[:2]
    img = bpy.data.images.new(os.path.basename(path), W, H, alpha=True)
    img.pixels[:] = px[::-1].ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def load_px(path):
    img = bpy.data.images.load(path)
    W, H = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)[::-1].copy()
    bpy.data.images.remove(img)
    return px


def wraps(px, axis):
    """do the two opposite edges continue each other about as well as neighbouring rows inside the picture do?"""
    a = np.moveaxis(px[..., :3], axis, 0)
    edge = float(np.abs(a[0] - a[-1]).mean())
    inner = float(np.abs(a[1:] - a[:-1]).mean())
    return edge < inner * 1.6, round(edge, 4), round(inner, 4)


def seamless(px, name):
    px[..., 3] = 1.0
    okx, ex, ix = wraps(px, 1)
    oky, ey, iy = wraps(px, 0)
    print("WRAP", name, "left-right", okx, ex, ix, "top-bottom", oky, ey, iy)
    if not okx:
        px = np.concatenate([px, px[:, ::-1]], axis=1)
    if not oky:
        px = np.concatenate([px, px[::-1]], axis=0)
    return px, (1 if okx else 2), (1 if oky else 2)


def sparse_grass():
    """The meadow picture. The first one (Grass_meshy_image.png, tufts edge to edge) read as crowded in play, so the
    meadow now uses a sparse Meshy picture (Grass_sparse_meshy_image.png: a few tufts on open ground), adjusted here:
      - three shifted copies are laid over each other -> about 3x the tufts (the raw picture is too empty at 8 m)
      - ink lines thickened to match the heavy outlines of the other surfaces
      - ground colour set to the old picture's ground colour, so the meadow keeps its tone next to dirt and cobble
    Same size and repeat (every 8 m) as the old texture, so the floor mesh and its UVs do not change."""
    px = load_px(os.path.join(SRC, "Grass_sparse_meshy_image.png"))
    old = load_px(os.path.join(SRC, "Grass_meshy_image.png"))
    H, W = px.shape[:2]
    bg = np.median(px[..., :3].reshape(-1, 3), axis=0)
    out = px.copy()
    for layer, (dy, dx) in ((px[:, ::-1], (int(H * 0.37), int(W * 0.21))), (px, (int(H * 0.71), int(W * 0.58)))):
        b = np.roll(layer, (dy, dx), axis=(0, 1))
        drawn = np.abs(b[..., :3] - bg).max(axis=2) > 0.10
        for _ in range(2):                                     # take a small border with the drawing
            drawn = drawn | np.roll(drawn, 1, 0) | np.roll(drawn, -1, 0) | np.roll(drawn, 1, 1) | np.roll(drawn, -1, 1)
        out[drawn] = b[drawn]
    ink = out[..., :3].max(axis=2) < 0.35                      # thicken the black lines by 1 px each way
    thick = ink.copy()
    for ax in (0, 1):
        for sh in (1, -1):
            thick |= np.roll(ink, sh, ax)
    out[thick, :3] = np.minimum(out[thick, :3], 0.04)
    target = np.percentile(old[..., :3].reshape(-1, 3), 80, axis=0)      # the old picture's open-ground colour
    white = out[..., :3].min(axis=2) > 0.75                    # flowers stay white
    rgb = out[..., :3] * (target / bg)
    out[..., :3] = np.where(white[..., None], out[..., :3], np.clip(rgb, 0, 1))
    print("GRASS bg", bg.round(3).tolist(), "-> ground", target.round(3).tolist())
    return out


# ---------------------------------------------------------------- 1 + 2: textures
tex_paths, REPEAT = {}, {}
sources = {"Grass": sparse_grass(), "Dirt": load_px(os.path.join(SRC, "Dirt_meshy_image.png")),
           "Cobble": photograph("Cobble")}
for name, px in sources.items():
    t, rx, ry = seamless(px, name)
    REPEAT[name] = (rx, ry)                           # how many source pictures one texture holds each way
    tex_paths[name] = os.path.join(OUT, "T_DC_Ground_%s.png" % name)
    save_px(t, tex_paths[name])

# ---------------------------------------------------------------- 3: the floor
bpy.ops.wm.read_factory_settings(use_empty=True)
bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")
MATS = ["Grass", "Dirt", "Cobble"]


def rect(cx, cy, hl, hw, yaw, top, mat, cells, extra=0.0):
    """rotated rectangle in Unreal cm -> Blender metres (x, -y); UVs in world metres / tile size so every piece lines up"""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    tile = TILE_M[mat]
    nx, ny = cells
    for i in range(nx):
        for j in range(ny):
            pts, uvs = [], []
            for (a, b) in ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)):
                lx, ly = -hl + 2 * hl * a / nx, -hw + 2 * hw * b / ny
                ux, uy = cx + lx * c - ly * s, cy + lx * s + ly * c            # Unreal cm
                pts.append((ux / 100.0, -uy / 100.0, (top + LIFT + extra) / 100.0))
                if mat == "Dirt":                                             # ruts run along the path, one picture across its width
                    uvs.append((lx / 100.0 / tile / REPEAT[mat][0], (ly + hw) / (2 * hw) / REPEAT[mat][1]))
                else:
                    uvs.append((ux / 100.0 / tile / REPEAT[mat][0], -uy / 100.0 / tile / REPEAT[mat][1]))
            f = bm.faces.new([bm.verts.new(p) for p in pts][::-1])            # y flip reverses winding: keep faces up
            f.material_index = MATS.index(mat)
            for loop, uv in zip(f.loops, uvs[::-1]):
                loop[uvl].uv = uv


rect(*MEADOW, "Grass", (12, 10))
rect(*SQUARE, "Cobble", (3, 5))
for k, p in enumerate(PATHS):
    rect(*p, "Dirt", (max(1, int(p[2] / 500)), 1), extra=k * PATH_STEP)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
up = sum(1 for f in bm.faces if f.normal.z > 0.5)
if up < len(bm.faces) / 2:
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
bmesh.ops.triangulate(bm, faces=bm.faces)
me = bpy.data.meshes.new("SM_DC_SetGround")
bm.to_mesh(me)
bm.free()
ob = bpy.data.objects.new("SM_DC_SetGround", me)
bpy.context.collection.objects.link(ob)
for name in MATS:
    m = bpy.data.materials.new("M_Ground_" + name)
    m.use_nodes = True
    nt = m.node_tree
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(tex_paths[name])
    nt.links.new(tex.outputs["Color"], nt.nodes["Principled BSDF"].inputs["Base Color"])
    nt.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
    me.materials.append(m)
co = np.array([v.co[:] for v in me.vertices])
print("FLOOR size m", (co.max(0) - co.min(0)).round(2).tolist(), "verts", len(me.vertices), "tris", len(me.polygons),
      "up-facing", sum(1 for p in me.polygons if p.normal.z > 0.5))
bpy.ops.object.select_all(action="DESELECT")
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_SetGround.glb"), export_format="GLB", use_selection=True,
                          export_apply=True, export_image_format="NONE")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_DC_SetGround.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    set_engine(scn)
    scn.render.resolution_x, scn.render.resolution_y = 1000, 640
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.7, 0.8, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(45), 0, math.radians(30)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); cam.data.clip_end = 1000
    scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("SetGround_Overview", (-95, 0.01, 75), (0, 0, 0))            # same angle as the level overview: from the village side
    shoot("SetGround_Eye_Village", (-52, 0, 1.7), (-30, 0, 0.5))       # player at the spawn, looking down the main path
    shoot("SetGround_Eye_Meadow", (-10, 12, 1.7), (10, 25, 0.3))      # in the meadow, away from paths
print("DONE")
