# Rated PG for Pig - replaces the greybox placeholders of sections A-D of Docs/Placeholder_List_For_Meshy.md with the
# Meshy art in Art/Meshy/Placeholders (see README_Wiring.md there).
# Run inside the editor (Lvl_DirectorsCut open, not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\LD_DressPlaceholders.py").read())
#   ph_setup()                 # imports + shared materials (safe to re-run)
#   ph_dress("A")              # or "B", "C", "D"
# Rules (from the wiring task): every greybox actor keeps its place, size and collision. Its material is swapped, or a
# card / mesh is put on top and the greybox is made invisible (collision stays). Never re-runs LD_BuildDirectorsCut.py.
# Section E and B1 (the Director boss) are not touched.
import math, os, unreal

ART = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\Placeholders"
DIR = "/Game/DirectorsCut/Environment/Placeholders"
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
V = unreal.Vector

# picture -> (kind, tile size in cm for tiled pictures)
TILED = {"T_DC_ScaffoldPipe": 120, "T_DC_Windmill_Wall": 300, "T_DC_FlatBack_Plywood": 480, "T_DC_Sandbags": 80,
         "T_DC_ArenaWall": 300, "T_DC_RibbonPost": 60, "T_DC_DollyRail": 120,
         "T_DC_Windmill_Wall2": 1400, "T_DC_Windmill_Roof": 250}   # Wall2: one picture per tower height (stones at the foot)
UVMAPPED = ["T_DC_RoadCase", "T_DC_PropsTable"]                     # one picture per face (mesh UVs, M_DC_InkedProp)
CARDS = ["T_DC_PineTree", "T_DC_WindmillSail", "T_DC_Burrow", "T_DC_PGShed_Facade", "T_DC_Rainbow", "T_DC_PGSign",
         "T_DC_Clapperboard", "T_DC_MeatHook", "T_DC_EmptyPlate", "T_DC_WindmillSail2"]
# picture orientation on cards stood up with _upright(): swap U/V and flip V reads correctly from the side the plane's
# normal points away from (checked on the PG sign); FlipU mirrors it for cards read from the other side
UPRIGHT_UV = {"T_DC_PGSign": {"Swap": 1.0, "FlipV": 1.0}, "T_DC_PGShed_Facade": {"Swap": 1.0, "FlipV": 1.0, "FlipU": 1.0},
              "T_DC_Rainbow": {"Swap": 1.0, "FlipV": 1.0}, "T_DC_Clapperboard": {"Swap": 1.0, "FlipV": 1.0, "FlipU": 0.0},
              "T_DC_WindmillSail2": {"Swap": 1.0}}          # the sail picture is tall: its height runs along the blade
FLOWERS = ["T_DC_Flower_0", "T_DC_Flower_1", "T_DC_Flower_2", "T_DC_Flower_3"]
MESHES = ["SM_DC_StudioSpotlight", "SM_DC_CameraCrane", "SM_DC_DirectorChair", "SM_DC_PineTree"]


def _tex(name):
    return unreal.load_asset("%s/Textures/%s" % (DIR, name))


def _import_textures():
    tasks = []
    for n in list(TILED) + UVMAPPED + CARDS:
        src = os.path.join(ART, "Export", n + ".png")
        if n == "T_DC_PGShed_Facade" and os.path.exists(os.path.join(ART, "Sized", "T_DC_PGShed_Facade_Door.png")):
            src = os.path.join(ART, "Sized", "T_DC_PGShed_Facade_Door.png")          # the copy with the door cut out
        tasks.append((n, src))
    tasks += [(n, os.path.join(ART, "Sized", n + ".png")) for n in FLOWERS]
    its = []
    for n, src in tasks:
        t = unreal.AssetImportTask(); t.filename = src; t.destination_path = DIR + "/Textures"; t.destination_name = n
        t.automated = True; t.replace_existing = True; t.save = True; its.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(its)
    for n, _ in tasks:
        tx = _tex(n)
        tx.set_editor_property("srgb", True)
        tx.set_editor_property("never_stream", True)
        if n not in TILED:
            tx.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
            tx.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        EAL.save_loaded_asset(tx)


def _import_meshes():
    for n in MESHES:
        d = "%s/%s" % (DIR, n)
        if EAL.does_asset_exist(d + "/" + n):
            continue
        t = unreal.AssetImportTask(); t.filename = os.path.join(ART, "Export", n + ".fbx"); t.destination_path = d
        t.destination_name = n; t.automated = True; t.replace_existing = True; t.save = True
        o = unreal.FbxImportUI()
        o.set_editor_property("import_mesh", True); o.set_editor_property("import_as_skeletal", False)
        o.set_editor_property("import_materials", True); o.set_editor_property("import_textures", True)
        o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        sm = o.get_editor_property("static_mesh_import_data")
        sm.set_editor_property("combine_meshes", True); sm.set_editor_property("auto_generate_collision", False)
        t.options = o
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
        m = unreal.load_asset(d + "/" + n)
        ns = SMS.get_nanite_settings(m)
        if ns.enabled:
            ns.enabled = False; SMS.set_nanite_settings(m, ns, apply_changes=True)
        EAL.save_loaded_asset(m)


def _scalar(mat, name, value, x, y):
    p = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name); p.set_editor_property("default_value", value)
    return p


def _material(name, code, inputs, masked):
    """Lit painted material: Tex sampled at the UVs the Custom node returns; a little emissive (Glow) so the paint
    reads in dark beats. inputs: list of (input name, expression) for the Custom node."""
    path = "%s/%s" % (DIR, name)
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())
    if masked:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("opacity_mask_clip_value", 0.4)
    cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -700, 0)
    cu.set_editor_property("code", code)
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    ins = []
    for n, _ in inputs:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
    cu.set_editor_property("inputs", ins)
    for i, (n, mk) in enumerate(inputs):
        MEL.connect_material_expressions(mk(mat, -1000, 100 * i), "", cu, n)
    ts = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, 0)
    ts.set_editor_property("parameter_name", "Tex")
    ts.set_editor_property("texture", _tex("T_DC_Burrow" if masked else "T_DC_ScaffoldPipe"))
    MEL.connect_material_expressions(cu, "", ts, "UVs")
    glow = _scalar(mat, "Glow", 0.3, -450, 300)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 250)
    MEL.connect_material_expressions(ts, "RGB", mul, "A"); MEL.connect_material_expressions(glow, "", mul, "B")
    MEL.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    r = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 450); r.set_editor_property("r", 1.0)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    s = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 550); s.set_editor_property("r", 0.0)
    MEL.connect_material_property(s, "", unreal.MaterialProperty.MP_SPECULAR)
    if masked:
        MEL.connect_material_property(ts, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def _mi(name, parent, tex, scalars=None):
    path = "%s/Materials/%s" % (DIR, name)
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIR + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_texture_parameter_value(mi, "Tex" if "BaseTex" not in [str(x) for x in MEL.get_texture_parameter_names(parent)] else "BaseTex", tex)
    for k, v in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


def ph_setup():
    _import_textures()
    _import_meshes()
    wp = lambda m, x, y: MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, x, y)
    nrm = lambda m, x, y: MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, x, y)
    tc = lambda m, x, y: MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, x, y)
    tiled = _material("M_DC_PaintTiled",
                      # the U sign follows the face's side so pictures with writing (the flat-back stencil) never read mirrored
                      "float3 a = abs(N); float2 uv = (a.z > a.x && a.z > a.y) ? P.xy : ((a.x > a.y) ? float2(N.x > 0 ? -P.y : P.y, -P.z) : float2(N.y > 0 ? P.x : -P.x, -P.z)); return uv / Tile;",
                      [("P", wp), ("N", nrm), ("Tile", lambda m, x, y: _scalar(m, "Tile", 200.0, x, y))], masked=False)
    card = _material("M_DC_PaintCard",
                     "float2 t = UV; if (Swap > 0.5) t = t.yx; if (FlipU > 0.5) t.x = 1 - t.x; if (FlipV > 0.5) t.y = 1 - t.y; return t;",
                     [("UV", tc), ("Swap", lambda m, x, y: _scalar(m, "Swap", 0.0, x, y)),
                      ("FlipU", lambda m, x, y: _scalar(m, "FlipU", 0.0, x, y)), ("FlipV", lambda m, x, y: _scalar(m, "FlipV", 0.0, x, y))], masked=True)
    inked = unreal.load_asset("/Game/DirectorsCut/Rendering/Materials/M_DC_InkedProp")
    out = {}
    for n, tile in TILED.items():
        out[n] = _mi("MI_" + n[2:], tiled, _tex(n), {"Tile": float(tile)}).get_name()
    for n in UVMAPPED:
        out[n] = _mi("MI_" + n[2:], inked, _tex(n)).get_name()
    for n in CARDS:
        out[n] = _mi("MI_" + n[2:] + "_Card", card, _tex(n), UPRIGHT_UV.get(n)).get_name()
    return out


# ---------------------------------------------------------------------------------------------------------------- helpers
MAIN = "Lvl_DirectorsCut"
CUBE = "/Engine/BasicShapes/Cube"


def _acts(level, prefixes):
    return [a for a in EAS.get_all_level_actors()
            if isinstance(a, unreal.StaticMeshActor) and a.get_level().get_outer().get_name() == level
            and a.get_actor_label().startswith(prefixes)]


def _hide(a):
    """greybox piece goes invisible; it keeps its collision and its place on the nav mesh"""
    a.modify(); a.static_mesh_component.modify()
    a.static_mesh_component.set_editor_property("visible", False)


def _paint(actors, mi_name):
    mi = unreal.load_asset("%s/Materials/%s" % (DIR, mi_name))
    for a in actors:
        a.modify(); c = a.static_mesh_component; c.modify()
        for i in range(c.get_num_materials()):
            c.set_material(i, mi)
    return len(actors)


def _no_collision(a):
    c = a.root_component
    if isinstance(c, unreal.PrimitiveComponent):
        c.set_editor_property("can_ever_affect_navigation", False)
        c.set_collision_profile_name("NoCollision")


def _clear(level, folder):
    for a in EAS.get_all_level_actors():
        if a.get_level().get_outer().get_name() == level and str(a.get_folder_path()) == folder and a.get_actor_label().startswith("PH_"):
            EAS.destroy_actor(a)


def _place(a, label, folder):
    a.set_actor_label(label); a.set_folder_path(folder)
    _no_collision(a)
    return a


def _sprite(tex_name, x, y, z, w, h, label, folder, shadow=True):
    """camera-facing card (same actor as the quest mark): turns to face the camera around its vertical axis"""
    tex = unreal.load_asset("%s/Textures/%s" % (DIR, tex_name))
    a = EAS.spawn_actor_from_class(unreal.DCSpriteActor, V(x, y, z), unreal.Rotator())
    spr = a.get_editor_property("sprite")
    for p in ("front_texture", "side_texture", "back_texture"):
        spr.set_editor_property(p, tex)
    spr.set_editor_property("front_size", unreal.Vector2D(w, h))
    spr.set_editor_property("side_size", unreal.Vector2D(w, h))
    spr.set_editor_property("cast_shadow", shadow)
    spr.set_editor_property("pivot_at_bottom", True)
    return _place(a, label, folder)


PLANE = "/Engine/BasicShapes/Plane"


def _card(mi_name, loc, rot, size_x, size_y, label, folder, shadow=True):
    """fixed flat card: the engine plane (100 x 100 in its own X/Y, picture from its UVs) with a masked two-sided
    material. A plane has no edges, so no outline shows (a thin box showed its sides as a frame)."""
    a = EAS.spawn_actor_from_object(unreal.load_asset(PLANE), loc, rot)
    a.set_actor_scale3d(V(size_x / 100.0, size_y / 100.0, 1.0))
    a.static_mesh_component.set_material(0, unreal.load_asset("%s/Materials/%s" % (DIR, mi_name)))
    a.static_mesh_component.set_cast_shadow(shadow)
    return _place(a, label, folder)


def _upright(rot):
    """rotation for a plane standing up: its normal along the given rotation's X axis (plane X -> world Z)"""
    return unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=90.0, yaw=0.0), rot)


def _mesh(asset_path, loc, rot, scale, label, folder):
    a = EAS.spawn_actor_from_object(unreal.load_asset(asset_path), loc, rot)
    a.set_actor_scale3d(V(scale, scale, scale))
    return _place(a, label, folder)


def _bounds(actors):
    lo = [1e9] * 3; hi = [-1e9] * 3
    for a in actors:
        o, e = a.get_actor_bounds(False)
        for i, (c, h) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
            lo[i] = min(lo[i], c - h); hi[i] = max(hi[i], c + h)
    return lo, hi


def _in_level(level):
    if level != MAIN:
        LES.set_current_level_by_name(level)


def _back_to_main():
    LES.set_current_level_by_name(MAIN)



PINE = DIR + "/SM_DC_PineTree/SM_DC_PineTree"


def _pine(x, y, h, label, folder, z=0.0):
    """3D pine (Meshy model made from the pine picture), standing on the ground, scaled to height h.
    Each pine gets its own turn and a small size change so the row does not look copied."""
    m = unreal.load_asset(PINE)
    b = m.get_bounding_box()
    k = sum(ord(c) for c in label)
    s = h / max(b.max.z - b.min.z, 1.0) * (0.92 + (k % 9) * 0.02)
    a = _mesh(PINE, V(x, y, z - b.min.z * s), unreal.Rotator(roll=0.0, pitch=0.0, yaw=float((k * 47) % 360)), s, label, folder)
    return a


def ph_pines_3d():
    """Swap each existing pine cut-out (PH_Pine_*) for the 3D pine at the cut-out's current place and height."""
    n = 0
    for a in list(EAS.get_all_level_actors()):
        if isinstance(a, unreal.DCSpriteActor) and a.get_actor_label().startswith("PH_Pine_"):
            lab = a.get_actor_label(); f = a.get_folder_path(); loc = a.get_actor_location()
            h = a.get_editor_property("sprite").get_editor_property("front_size").y
            EAS.destroy_actor(a)
            _pine(loc.x, loc.y, h, lab, f, loc.z)
            n += 1
    return n


# ---------------------------------------------------------------------------------------------------------------- A
def ph_dress_A():
    """Seen in every beat (persistent level)."""
    F = "DC/SetDressing_Placeholders"
    _clear(MAIN, F)
    out = {}
    # A1 pines: one camera-facing pine card per greybox pine (crown + trunk hidden, they keep blocking)
    n = 0
    for trunk in _acts(MAIN, ("Edge_Pine_",)):
        lab = trunk.get_actor_label()
        if not lab.endswith("_Trunk"):
            continue
        crown = next((x for x in _acts(MAIN, (lab[:-6] + "_Crown",))), None)
        o, _ = trunk.get_actor_bounds(False)
        top = 1000.0
        if crown:
            co, ce = crown.get_actor_bounds(False); top = co.z + ce.z; _hide(crown)
        _hide(trunk)
        h = top * 1.05                                    # the picture has a small clear margin at the top
        _pine(o.x, o.y, h, "PH_Pine_" + lab[10:13], F)
        n += 1
    out["pines"] = n

    # A2 Director's Tower: the shaft and base get the scaffold paint; the camera on top becomes Taisho's film camera
    out["tower_painted"] = _paint(_acts(MAIN, ("Tower_Shaft", "Tower_Base")), "MI_DC_ScaffoldPipe")
    cam_parts = _acts(MAIN, ("Tower_CameraBody", "Tower_CameraLens", "Tower_LensRim", "Tower_Reel", "Tower_TallyLight"))
    lo, hi = _bounds(cam_parts)
    for a in cam_parts:
        _hide(a)
    shaft_top = 7300.0
    cam = unreal.load_asset("/Game/DirectorsCut/Environment/FilmSet/SM_DC_FilmCamera")
    b = cam.get_bounding_box()
    scale = (hi[2] - shaft_top) / max(b.max.z - b.min.z, 1.0)
    # yaw 0: the model's lens points along -X, towards the set (checked with a capture from the set side)
    _mesh("/Game/DirectorsCut/Environment/FilmSet/SM_DC_FilmCamera", V(11000, 0, shaft_top), unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), scale, "PH_Tower_FilmCamera", F)
    out["tower_camera_scale"] = round(scale, 2)

    # A3 windmill: wall paint on tower + cap; each blade becomes a sail card with the same transform
    # second windmill art (Nov 15), styled on the village house: plaster + timber tower, red-tile cap, canvas sails
    out["windmill_painted"] = _paint(_acts(MAIN, ("Windmill_Tower",)), "MI_DC_Windmill_Wall2") + _paint(_acts(MAIN, ("Windmill_Cap",)), "MI_DC_Windmill_Roof")
    for i, blade in enumerate(sorted(_acts(MAIN, ("Windmill_Blade",)), key=lambda a: a.get_actor_label())):
        t = blade.get_actor_transform()
        s = t.scale3d
        # the blade is a thin slab, thin along its own X: a plane standing in its Y/Z, plane X along the blade's Z.
        # Each sail sits 6 cm further out than the one before, so the crossing sails don't flicker at the hub.
        c = _card("MI_DC_WindmillSail2_Card", t.translation, _upright(t.rotation.rotator()), s.z * 100.0, s.y * 100.0, "PH_" + blade.get_actor_label(), F)
        c.set_actor_location(c.get_actor_location() + c.get_actor_up_vector() * (6.0 * i), False, False)
        _hide(blade)

    # A4 scaffold behind the low east wall
    out["scaffold_painted"] = _paint(_acts(MAIN, ("Scaffold_",)), "MI_DC_ScaffoldPipe")

    # A5 boar burrows: the brown mound is hidden (keeps blocking), a burrow card lies flat on the ground
    for bur in _acts(MAIN, ("Burrow",)):
        o, e = bur.get_actor_bounds(False)
        _card("MI_DC_Burrow_Card", V(o.x, o.y, 2.0), unreal.Rotator(), e.x * 2.4, e.y * 2.4, "PH_" + bur.get_actor_label(), F, shadow=False)
        _hide(bur)

    # A6 backs of the house flats: plywood boards and braces, sandbag paint
    out["flat_backs"] = _paint(_acts(MAIN, tuple("House_%d_Board" % i for i in range(4)) + tuple("House_%d_Brace" % i for i in range(4))), "MI_DC_FlatBack_Plywood")
    out["sandbags"] = _paint(_acts(MAIN, tuple("House_%d_Sandbag" % i for i in range(4))), "MI_DC_Sandbags")
    return out


# ---------------------------------------------------------------------------------------------------------------- B
SPOT = DIR + "/SM_DC_StudioSpotlight/SM_DC_StudioSpotlight"
CRANE = DIR + "/SM_DC_CameraCrane/SM_DC_CameraCrane"
CHAIR = DIR + "/SM_DC_DirectorChair/SM_DC_DirectorChair"
SPOT_YAW_OFFSET = 0.0          # added to the real spotlight's yaw so the lamp model points the same way
CRANE_YAW = 0.0                # crane model yaw so its arm points to -X (towards the platform end)
CHAIR_YAW = -90.0              # chair model yaw so it faces -X (its back on +X, like the greybox; checked with a capture)


def _fit_mesh(asset, lo, hi, label, folder, yaw, by="z"):
    """put a model (centred on its origin) over the box lo..hi: scaled to the box height (or x length), standing on lo.z"""
    m = unreal.load_asset(asset)
    b = m.get_bounding_box()
    size = {"z": (b.max.z - b.min.z, hi[2] - lo[2]), "x": (b.max.x - b.min.x, max(hi[0] - lo[0], hi[1] - lo[1]))}[by]
    s = size[1] / max(size[0], 1.0)
    loc = V((lo[0] + hi[0]) / 2.0, (lo[1] + hi[1]) / 2.0, lo[2] - b.min.z * s)
    return _mesh(asset, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), s, label, folder)


def _spot_rigs(level, pole_prefix, head_prefix, light_prefix, folder):
    poles = sorted(_acts(level, (pole_prefix,)), key=lambda a: a.get_actor_label())
    heads = _acts(level, (head_prefix,))
    lights = [a for a in EAS.get_all_level_actors() if a.get_level().get_outer().get_name() == level and a.get_actor_label().startswith(light_prefix)]
    n = 0
    for pole in poles:
        if pole.get_actor_label().startswith(head_prefix):
            continue
        po, pe = pole.get_actor_bounds(False)
        head = min(heads, key=lambda h: (h.get_actor_location() - po).length()) if heads else None
        light = min(lights, key=lambda l: (l.get_actor_location() - po).length()) if lights else None
        parts = [pole] + ([head] if head else [])
        lo, hi = _bounds(parts)
        yaw = (light.get_actor_rotation().yaw if light else 0.0) + SPOT_YAW_OFFSET
        _fit_mesh(SPOT, lo, hi, "PH_" + pole.get_actor_label(), folder, yaw)
        for p in parts:
            _hide(p)
        n += 1
    return n


def ph_dress_B():
    """Beat 5: the Director fight arena (DC_Set_Boss)."""
    L, F = "DC_Set_Boss", "DC/Set_Boss"
    _clear(L, F)
    out = {}
    out["walls"] = _paint(_acts(L, ("Arena_Wall",)), "MI_DC_ArenaWall")
    out["cover"] = _paint(_acts(L, ("Arena_Cover", "Arena_LowCover")), "MI_DC_RoadCase")
    _in_level(L)
    try:
        out["light_rigs"] = _spot_rigs(L, "Arena_LightRig", "Arena_LightRig_Head", "Arena_Spot", F)
        crane = _acts(L, ("Crane_",))
        lo, hi = _bounds(crane)
        _fit_mesh(CRANE, lo, hi, "PH_CameraCrane", F, CRANE_YAW, by="x")
        for p in crane:
            _hide(p)
        out["crane"] = len(crane)
    finally:
        _back_to_main()
    return out


# ---------------------------------------------------------------------------------------------------------------- C
FLOWER_ASPECT = [306.0 / 768.0, 344.0 / 768.0, 344.0 / 768.0, 318.0 / 768.0]


def _reimport(names):
    tasks = []
    for n in names:
        src = os.path.join(ART, "Export", n + ".png")
        if n == "T_DC_PGShed_Facade":
            src = os.path.join(ART, "Sized", "T_DC_PGShed_Facade_Door.png")
        t = unreal.AssetImportTask(); t.filename = src; t.destination_path = DIR + "/Textures"; t.destination_name = n
        t.automated = True; t.replace_existing = True; t.save = True; tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for n in names:
        tx = _tex(n)
        tx.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP); tx.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
        tx.set_editor_property("never_stream", True)
        EAL.save_loaded_asset(tx)


def ph_dress_C():
    """Beat 4: the pink romance set (DC_Set_PG)."""
    L, F = "DC_Set_PG", "DC/Set_PG"
    # the final facade (with Saar's doorway cut out) and the final PG sign
    _reimport(["T_DC_PGShed_Facade", "T_DC_PGSign"])
    _clear(L, F)
    out = {}
    out["posts"] = _paint(_acts(L, ("Romance_Post",)), "MI_DC_RibbonPost")
    _in_level(L)
    try:
        # C1 shed: the facade card stands 26 cm in front of the front wall (outside = the wall's yaw direction)
        fl = _acts(L, ("Shed_Wall_Front_L",))[0].get_actor_location(); fr = _acts(L, ("Shed_Wall_Front_R",))[0].get_actor_location()
        yaw = _acts(L, ("Shed_Wall_Front_L",))[0].get_actor_rotation().yaw
        n = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0)
        c = V((fl.x + fr.x) / 2.0 + n.x * 26.0, (fl.y + fr.y) / 2.0 + n.y * 26.0, 350.0)
        _card("MI_DC_PGShed_Facade_Card", c, _upright(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)), 700.0, 700.0, "PH_Shed_Facade", F)
        # C2 flowers: a camera-facing flower per greybox flower (stem + head hidden)
        k = 0
        for stem in sorted(_acts(L, ("Flower_",)), key=lambda a: a.get_actor_label()):
            lab = stem.get_actor_label()
            if not lab.endswith("_Stem"):
                continue
            head = next(iter(_acts(L, (lab[:-5] + "_Head",))), None)
            o, _ = stem.get_actor_bounds(False)
            top = 200.0
            if head:
                ho, he = head.get_actor_bounds(False); top = ho.z + he.z; _hide(head)
            _hide(stem)
            _sprite("T_DC_Flower_%d" % (k % 4), o.x, o.y, 0.0, top * FLOWER_ASPECT[k % 4], top, "PH_" + lab[:-5], F)
            k += 1
        out["flowers"] = k
        # C3 rainbow: one big card over the 125 blocks
        bows = _acts(L, ("Rainbow",))
        lo, hi = _bounds(bows)
        wx, wy = hi[0] - lo[0], hi[1] - lo[1]
        width = max(wx, wy) * 1.05
        height = width * 768.0 / 1376.0
        face_yaw = 0.0 if wx < wy else 90.0                         # the arc stands in the plane of its longer side
        _card("MI_DC_Rainbow_Card", V((lo[0] + hi[0]) / 2.0, (lo[1] + hi[1]) / 2.0, lo[2] + height / 2.0),
              _upright(unreal.Rotator(roll=0.0, pitch=0.0, yaw=face_yaw)), height, width, "PH_Rainbow", F)
        for b in bows:
            _hide(b)
        out["rainbow"] = [round(width), round(height), face_yaw, len(bows)]
        # C4 PG sign: a heart sign card in front of the board (board hidden, poles kept)
        board = _acts(L, ("PGBoard",))
        board = [b for b in board if b.get_actor_label() == "PGBoard"][0]
        bo, be = board.get_actor_bounds(False)
        _card("MI_DC_PGSign_Card", V(bo.x - be.x - 4.0, bo.y, bo.z), _upright(unreal.Rotator()), 760.0, 760.0, "PH_PGSign", F)
        _hide(board)
    finally:
        _back_to_main()
    return out


# ---------------------------------------------------------------------------------------------------------------- D
def ph_dress_D():
    """Beats 2 and 3: the empty butcher stall (DC_Set_NoMeat) and the film crew set (DC_Set_FilmCrew)."""
    out = {}
    # the clapperboard is read from -X like the PG sign, so it uses the sign's orientation (no mirror)
    mi = unreal.load_asset(DIR + "/Materials/MI_DC_Clapperboard_Card")
    for k, v in UPRIGHT_UV["T_DC_Clapperboard"].items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    EAL.save_loaded_asset(mi)

    L, F = "DC_Set_NoMeat", "DC/Set_NoMeat"
    _clear(L, F)
    _in_level(L)
    try:
        # D1 meat hooks: a camera-facing hook per greybox hook (rod + tip hidden); the plate becomes a flat plate card
        n = 0
        for hook in sorted(_acts(L, ("MeatHook_",)), key=lambda a: a.get_actor_label()):
            lab = hook.get_actor_label()
            if "_Tip_" in lab:
                continue
            tip = next(iter(_acts(L, ("MeatHook_Tip_" + lab[-1],))), None)
            parts = [hook] + ([tip] if tip else [])
            lo, hi = _bounds(parts)
            h = (hi[2] - lo[2]) * 1.1
            _sprite("T_DC_MeatHook", (lo[0] + hi[0]) / 2.0, (lo[1] + hi[1]) / 2.0, lo[2] - 4.0, h * 578.0 / 768.0, h, "PH_" + lab, F)
            for p in parts:
                _hide(p)
            n += 1
        out["hooks"] = n
        plate = _acts(L, ("EmptyPlate",))[0]
        po, pe = plate.get_actor_bounds(False)
        _card("MI_DC_EmptyPlate_Card", V(po.x, po.y, po.z + pe.z + 0.5), unreal.Rotator(), pe.x * 2.3, pe.y * 2.3 * 798.0 / 768.0, "PH_EmptyPlate", F, shadow=False)
        _hide(plate)
    finally:
        _back_to_main()

    L, F = "DC_Set_FilmCrew", "DC/Set_FilmCrew"
    _clear(L, F)
    out["dolly_rails"] = _paint(_acts(L, ("Dolly_Rail_",)), "MI_DC_DollyRail")
    out["pedestal"] = _paint(_acts(L, ("POI_WeaponPedestal",)), "MI_DC_PropsTable")
    _in_level(L)
    try:
        # D2 director's chair model over the box chair
        chair = _acts(L, ("DirChair_",))
        lo, hi = _bounds(chair)
        _fit_mesh(CHAIR, lo, hi, "PH_DirectorChair", F, CHAIR_YAW)
        for p in chair:
            _hide(p)
        out["chair"] = len(chair)
        # D3 clapperboard: one upright card in front of board + stick (read from -X, where its sign text faces)
        clap = _acts(L, ("Clapper_",))
        lo, hi = _bounds(clap)
        size = max(hi[1] - lo[1], hi[2] - lo[2])
        _card("MI_DC_Clapperboard_Card", V(lo[0] - 4.0, (lo[1] + hi[1]) / 2.0, lo[2] + size / 2.0), _upright(unreal.Rotator()), size, size, "PH_Clapperboard", F)
        for p in clap:
            _hide(p)
        # the picture has its own "RATED PG FOR PIG" slate text; the old text label would print over it
        for a in EAS.get_all_level_actors():
            if a.get_level().get_outer().get_name() == L and a.get_actor_label() == "SIGN_SCENE_3__TAKE_2":
                a.modify(); a.root_component.modify(); a.root_component.set_editor_property("visible", False)
        # D5 light stands: the studio spotlight model (same as the boss arena)
        out["light_stands"] = _spot_rigs(L, "LightStand_Pole", "LightStand_Head", "LightStand_Spot", F)
    finally:
        _back_to_main()
    return out
