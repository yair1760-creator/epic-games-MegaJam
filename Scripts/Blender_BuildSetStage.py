# Director's Cut - the round film-set stage ("THE SET" disc in the middle of the platform) in the Forgive Me Father
# comic style: a simple round platform built in Blender and covered with a Meshy text-to-image plank picture
# (Art/Meshy/SetFloor/Source/Planks_meshy_image.png: inked floorboards, nails, scratches) at a real plank size.
# Replaces the first attempt (SM_DC_SetFloor, a Meshy 3D model stretched to 24 m: orange, blurry, 1-2 m planks).
#   blender --background --python Scripts/Blender_BuildSetStage.py [-- preview]
# Out: Art/Meshy/SetFloor/Export/SM_DC_SetStage.glb + .blend, T_DC_SetStage_Planks.png
#   - top: planks 30 cm wide running across the stage (the picture repeats every TILE_M metres)
#   - trim: a ring of boards around the top edge, running along the circle
#   - rim: one tall board all the way round the side
import bpy, bmesh, math, os, shutil, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Source", "Planks_meshy_image.png")
OUT = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
RADIUS, HEIGHT = 12.0, 0.40           # the disc POI_TheSet_Plinth: 24 m across, 40 cm tall
TRIM = 0.45                           # width of the board ring around the top edge
TILE_M = 2.4                          # the picture has 8 planks -> 30 cm planks
N = 96
os.makedirs(OUT, exist_ok=True)
TEX = os.path.join(OUT, "T_DC_SetStage_Planks.png")
shutil.copyfile(SRC, TEX)

bpy.ops.wm.read_factory_settings(use_empty=True)
img = bpy.data.images.load(TEX)
W, H = img.size
px = np.array(img.pixels[:], dtype=np.float32).reshape(H, W, 4)
for axis, nm in ((1, "left-right"), (0, "top-bottom")):
    a = np.moveaxis(px[..., :3], axis, 0)
    print("WRAP", nm, round(float(np.abs(a[0] - a[-1]).mean()), 4), "vs inside", round(float(np.abs(a[1:] - a[:-1]).mean()), 4))

bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")
PLANK_V = 1.0 / 8.0                   # one plank of the picture


def face(pts, uvs):
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    for loop, uv in zip(f.loops, uvs):
        loop[uvl].uv = uv


def ring(r, z, k):
    a = 2 * math.pi * k / N
    return (r * math.cos(a), r * math.sin(a), z)


ri = RADIUS - TRIM
circ = 2 * math.pi * RADIUS
reps = round(circ / TILE_M)           # whole number of pictures around the circle -> no seam
for k in range(N):
    k2 = k + 1
    # top: fan from the centre, planar UVs (planks run straight across)
    p0, p1 = ring(ri, HEIGHT, k), ring(ri, HEIGHT, k2)
    face([(0, 0, HEIGHT), p0, p1], [(0, 0), (p0[0] / TILE_M, p0[1] / TILE_M), (p1[0] / TILE_M, p1[1] / TILE_M)])
    u0, u1 = reps * k / N, reps * k2 / N
    # trim ring: boards follow the circle (1.5 planks wide), a hair above the top so its ink edge reads
    q0, q1 = ring(RADIUS, HEIGHT + 0.004, k), ring(RADIUS, HEIGHT + 0.004, k2)
    t0, t1 = ring(ri - 0.02, HEIGHT + 0.004, k), ring(ri - 0.02, HEIGHT + 0.004, k2)
    face([t0, q0, q1, t1], [(u0, 0), (u0, PLANK_V * 1.5), (u1, PLANK_V * 1.5), (u1, 0)])
    # rim: one board round the side
    b0, b1 = ring(RADIUS, 0, k), ring(RADIUS, 0, k2)
    face([b0, b1, q1, q0], [(u0, PLANK_V * 2), (u1, PLANK_V * 2), (u1, PLANK_V * 3), (u0, PLANK_V * 3)])
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
bmesh.ops.triangulate(bm, faces=bm.faces)
me = bpy.data.meshes.new("SM_DC_SetStage")
bm.to_mesh(me)
bm.free()
ob = bpy.data.objects.new("SM_DC_SetStage", me)
bpy.context.collection.objects.link(ob)
m = bpy.data.materials.new("M_SetStage")
m.use_nodes = True
tex = m.node_tree.nodes.new("ShaderNodeTexImage")
tex.image = img
m.node_tree.links.new(tex.outputs["Color"], m.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
m.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
me.materials.append(m)
co = np.array([v.co[:] for v in me.vertices])
print("STAGE size m", (co.max(0) - co.min(0)).round(3).tolist(), "verts", len(me.vertices), "tris", len(me.polygons),
      "top faces up", sum(1 for p in me.polygons if p.normal.z > 0.9), "reps around", reps)
bpy.ops.object.select_all(action="DESELECT")
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_SetStage.glb"), export_format="GLB", use_selection=True,
                          export_apply=True, export_image_format="NONE")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_DC_SetStage.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng; break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 1000, 600
    scn.view_settings.view_transform = "Standard"
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.62, 0.7, 0.8, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(45), 0, math.radians(30)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("SetStage_Angle", (-19, -12, 9), (0, 0, 0))
    shoot("SetStage_Rim", (-15.5, 0, 1.7), (-11.5, 0, 0.3))
print("DONE")
