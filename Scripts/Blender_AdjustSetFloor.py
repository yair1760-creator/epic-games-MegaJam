# Director's Cut - the film-set stage floor: a Meshy model (text-to-3D, meshy-t2.5 smart topology + texture) opened in
# Blender and adjusted for the game. Replaces the grey plinth "THE SET" in the middle of the playable level.
#   blender --background --python Scripts/Blender_AdjustSetFloor.py [-- preview]
# In : Art/Meshy/SetFloor/Source/SetFloor_meshy.glb (+ _base_color.png)   (straight from Meshy, untouched)
# Out: Art/Meshy/SetFloor/Export/SM_DC_SetFloor.glb, T_DC_SetFloor.png, SM_DC_SetFloor.blend
# Adjustments:
#   1. join everything, merge doubles, recalc normals (all of Meshy's separate plank / tape parts are kept)
#   2. centre on the origin, bottom at z = 0, make the footprint a true circle (equal X / Y size)
#   3. scale to the level: DIAMETER m across, HEIGHT m tall (same as the plinth it replaces, so boars can step on);
#      the strong Z squash flattens the walkable top (plank steps end up under 1 cm)
#   4. triangulate + export GLB without embedded images (Unreal gets the texture as its own PNG)
import bpy, bmesh, math, os, shutil, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Source", "SetFloor_meshy.glb")
SRC_TEX = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Source", "SetFloor_meshy_base_color.png")
OUT = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export")
PREVIEW = "preview" in (sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
DIAMETER, HEIGHT = 24.0, 0.20
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SRC)
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
print("IMPORTED", [(o.name, len(o.data.vertices), len(o.data.polygons)) for o in meshes])
bpy.ops.object.select_all(action="DESELECT")
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
ob = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
ob.name = ob.data.name = "SM_DC_SetFloor"

bm = bmesh.new()
bm.from_mesh(ob.data)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
# (Meshy's smart topology makes each plank / tape mark its own part: keep them all)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

# 2. centre, bottom at 0, true circle
co = np.array([v.co[:] for v in bm.verts])
mn, mx = co.min(0), co.max(0)
size = mx - mn
print("RAW SIZE", size.round(3).tolist())
cx, cy = (mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2
sx, sy, sz = DIAMETER / size[0], DIAMETER / size[1], HEIGHT / size[2]
# 3. no per-vertex flattening needed: the Z squash below (x1.8 vs x25 sideways) leaves plank steps under 1 cm
# 4. scale
for v in bm.verts:
    v.co.x = (v.co.x - cx) * sx
    v.co.y = (v.co.y - cy) * sy
    v.co.z = (v.co.z - mn[2]) * sz
bmesh.ops.triangulate(bm, faces=bm.faces)
bm.to_mesh(ob.data)
bm.free()
for p in ob.data.polygons:
    p.use_smooth = False                         # crisp planks
co = np.array([v.co[:] for v in ob.data.vertices])
print("FINAL SIZE", (co.max(0) - co.min(0)).round(3).tolist(), "verts", len(ob.data.vertices), "tris", len(ob.data.polygons),
      "materials", [m.name for m in ob.data.materials])

# texture: copy as its own PNG; one material named for Unreal
shutil.copyfile(SRC_TEX, os.path.join(OUT, "T_DC_SetFloor.png"))
for i, m in enumerate(ob.data.materials):
    m.name = "M_SetFloor" if i == 0 else "M_SetFloor_%d" % i

bpy.ops.object.select_all(action="DESELECT")
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_SetFloor.glb"), export_format="GLB", use_selection=True,
                          export_apply=True, export_image_format="NONE")
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_DC_SetFloor.blend"))

if PREVIEW:
    pv = os.path.join(ROOT, "Saved", "PropPreview")
    os.makedirs(pv, exist_ok=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng; break
        except Exception:
            pass
    scn.render.resolution_x, scn.render.resolution_y = 900, 560
    scn.world = bpy.data.worlds.new("W"); scn.world.use_nodes = True
    scn.world.node_tree.nodes["Background"].inputs[0].default_value = (0.6, 0.66, 0.72, 1)
    scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(35)); scn.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
    def shoot(name, loc, look):
        cam.location = loc
        d = [look[i] - loc[i] for i in range(3)]
        cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
        scn.render.filepath = os.path.join(pv, name + ".png")
        bpy.ops.render.render(write_still=True)
    shoot("SetFloor_Top", (0.01, -0.01, 44), (0, 0, 0))
    shoot("SetFloor_Angle", (-17, -17, 9), (0, 0, 0))
    shoot("SetFloor_Eye", (-15.5, 0, 1.7), (-8, 0, 0.1))        # a player standing next to the rim
print("DONE")
