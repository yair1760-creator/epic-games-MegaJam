# Director's Cut - Taisho's Meshy film-set models (film reel camera, boom mic, woodland slide) made game-ready in Blender.
# The Meshy FBX files are clean but far too dense (the camera is ~5 million triangles). Unreal's automatic reduction of
# that is what tore the surface, so the reduction is done here instead, with care:
#   blender --background --python Scripts/Blender_CleanFilmSetModels.py [-- preview] [-- only=FilmCamera]
# In : Art/Meshy/Cameras/...Reel..., Art/Meshy/BoomMic/..., Art/Meshy/WoodlandSlide/...   (Taisho's files, untouched)
# Out: Art/Meshy/FilmSet/Export/SM_DC_<Name>.glb + T_DC_<Name>.png (colour picture, 2048 px) + FilmSet.blend
# Per model:
#   1. weld doubles, then reduce to TARGET triangles with the Decimate modifier (collapse)
#   2. drop any crumbs the reduction left (loose pieces of a few faces)
#   3. smooth shading with hard edges kept where faces meet at more than 40 degrees (no faceting, no smeared corners)
#   4. the camera only: cut away the round grass-and-rocks display base, so the tripod can stand on any floor
#   5. bottom centre to the origin, scaled to its real height for the level
import bpy, bmesh, math, os, sys
import numpy as np

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
OUT = os.path.join(ROOT, "Art", "Meshy", "FilmSet", "Export")
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
PREVIEW = "preview" in ARGS
ONLY = [a.split("=", 1)[1] for a in ARGS if a.startswith("only=")]
os.makedirs(OUT, exist_ok=True)
#        name            source (without extension)                                                                           tris    height m  cut base (fraction of height)
ITEMS = [("FilmCamera", r"Art\Meshy\Cameras\Meshy_AI_Reel_Reflections_20261114_texture_fbx\Meshy_AI_Reel_Reflections_20261114_texture", 110000, 2.05, 0.068),
         ("BoomMic", r"Art\Meshy\BoomMic\Meshy_AI_Silent_Stage_20261114_texture_fbx\Meshy_AI_Silent_Stage_20261114_texture", 30000, 2.20, 0.0),
         ("WoodlandSlide", r"Art\Meshy\WoodlandSlide\Meshy_AI_Woodland_Slide_20261114_texture_fbx\Meshy_AI_Woodland_Slide_20261114_texture", 40000, 1.36, 0.0)]
SHARP_DEG = 40.0
done = []

for name, base, target, height, cut in ITEMS:
    if ONLY and name not in ONLY:
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT, base + ".fbx"))
    obs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for o in obs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = obs[0]
    if len(obs) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    ob.name = ob.data.name = "SM_DC_" + name
    tris0 = sum(len(p.vertices) - 2 for p in ob.data.polygons)

    # 4 (first, while the mesh is still exact): cut the display base off below the tripod
    if cut > 0:
        zs = np.array([v.co.z for v in ob.data.vertices])
        zcut = float(zs.min() + cut * (zs.max() - zs.min()))
        bm = bmesh.new(); bm.from_mesh(ob.data)
        res = bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, zcut), plane_no=(0, 0, 1), clear_inner=True)
        cut_edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        bmesh.ops.holes_fill(bm, edges=cut_edges, sides=0)            # close the cut ends of the legs
        bm.to_mesh(ob.data); bm.free()
        print("  CUT", name, "base removed below z", round(zcut, 3), "cut edges", len(cut_edges))

    # 1: weld + reduce
    bm = bmesh.new(); bm.from_mesh(ob.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bm.to_mesh(ob.data); bm.free()
    tris1 = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    mod = ob.modifiers.new("Reduce", "DECIMATE")
    mod.decimate_type = "COLLAPSE"
    mod.ratio = min(1.0, target / max(tris1, 1))
    mod.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=mod.name)

    # 2: crumbs
    bm = bmesh.new(); bm.from_mesh(ob.data)
    bm.verts.ensure_lookup_table()
    seen, parts = set(), []
    for v in bm.verts:
        if v.index in seen:
            continue
        stack, part = [v], []
        seen.add(v.index)
        while stack:
            x = stack.pop(); part.append(x)
            for e in x.link_edges:
                y = e.other_vert(x)
                if y.index not in seen:
                    seen.add(y.index); stack.append(y)
        parts.append(part)
    parts.sort(key=len, reverse=True)
    islands = len(parts)
    # where the display base was cut away, the tips of its rocks and grass are left as separate pieces: keep only
    # the model itself (the biggest piece). Otherwise only drop pieces of a few faces.
    crumbs = [v for part in parts[1:] for v in part if cut > 0 or len(part) < 12]
    if crumbs:
        bmesh.ops.delete(bm, geom=crumbs, context="VERTS")
    # 3: smooth, hard edges where the angle is big
    for f in bm.faces:
        f.smooth = True
    lim = math.radians(SHARP_DEG)
    sharp = 0
    for e in bm.edges:
        if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim:
            e.smooth = False; sharp += 1
        else:
            e.smooth = True
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    # 5: bottom centre to the origin, real height
    co = np.array([v.co[:] for v in bm.verts])
    mn, mx = co.min(0), co.max(0)
    s = height / (mx[2] - mn[2])
    for v in bm.verts:
        v.co.x = (v.co.x - (mn[0] + mx[0]) / 2) * s
        v.co.y = (v.co.y - (mn[1] + mx[1]) / 2) * s
        v.co.z = (v.co.z - mn[2]) * s
    bm.to_mesh(ob.data); bm.free()
    co = np.array([v.co[:] for v in ob.data.vertices])
    print("MODEL", name, "tris", tris0, "->", len(ob.data.polygons), "islands", islands, "crumb verts removed", len(crumbs), "sharp edges", sharp,
          "size m", (co.max(0) - co.min(0)).round(2).tolist())

    # colour picture: Taisho's base colour, 2048 px, saved next to the model; one plain material for the export
    img = bpy.data.images.load(os.path.join(ROOT, base + ".png"))
    if img.size[0] > 2048:
        img.scale(2048, 2048)
    tex_path = os.path.join(OUT, "T_DC_%s.png" % name)
    img.filepath_raw = tex_path; img.file_format = "PNG"; img.save()
    mat = bpy.data.materials.new("M_" + name)
    mat.use_nodes = True
    tn = mat.node_tree.nodes.new("ShaderNodeTexImage"); tn.image = img
    mat.node_tree.links.new(tn.outputs["Color"], mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
    mat.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
    ob.data.materials.clear(); ob.data.materials.append(mat)
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_%s.glb" % name), export_format="GLB", use_selection=True,
                              export_apply=True, export_image_format="NONE")
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "FilmSet_%s.blend" % name))

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
        scn.world.node_tree.nodes["Background"].inputs[1].default_value = 1.2
        sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3.0
        sun.rotation_euler = (math.radians(50), 0, math.radians(30)); scn.collection.objects.link(sun)
        cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scn.collection.objects.link(cam); scn.camera = cam
        c = (co.max(0) + co.min(0)) / 2; r = float((co.max(0) - co.min(0)).max())
        for tag, ang, el, dist in (("A", 35, 15, 1.9), ("B", 215, 20, 1.9), ("Close", 300, 8, 0.95)):
            a, e = math.radians(ang), math.radians(el)
            loc = (c[0] + r * dist * math.cos(a) * math.cos(e), c[1] + r * dist * math.sin(a) * math.cos(e), c[2] + r * dist * math.sin(e))
            cam.location = loc
            d = [c[i] - loc[i] for i in range(3)]
            cam.rotation_euler = (math.atan2(math.hypot(d[0], d[1]), -d[2]), 0, math.atan2(d[1], d[0]) - math.pi / 2)
            scn.render.filepath = os.path.join(pv, "FilmSet_%s_%s.png" % (name, tag))
            bpy.ops.render.render(write_still=True)
    done.append(name)
print("DONE", done)
