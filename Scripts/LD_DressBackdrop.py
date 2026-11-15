# Director's Cut - the painted backdrop flat and the cut-out sun (Meshy nano-banana-2 pictures, Art/Meshy/Backdrop).
# Run inside the editor (Lvl_DirectorsCut open, not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\LD_DressBackdrop.py").read()); dress_backdrop()
# What it does, in the DC_Set_Hunt sublevel only (edits these actors, nothing else; does not re-run LD_BuildDirectorsCut):
#   - Backdrop_Flat (the 100 m x 26 m cube behind the meadow) gets M_DC_BackdropSky: the painted sky picture placed by
#     WORLD position (left edge = Y -5000, right = Y +5000, bottom = Z 0, top = Z 2600), so it cannot come out mirrored
#     or stretched whatever the cube's own UVs are.
#   - Backdrop_Sun becomes a flat 9 m x 9 m plywood cut-out (plane, masked by the picture's alpha, two-sided) 15 cm in front
#     of the flat at Y -700, Z 2100, in open sky. Same actor, same name.
#   - Backdrop_Cloud_0..3 (grey boxes) are made invisible: the clouds are painted on the flat now. Struts are kept.
import unreal

BD_SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\Backdrop\Export"
BD_DIR = "/Game/DirectorsCut/Environment/Backdrop"
FLAT_RECT = (-5000.0, 10000.0, 0.0, 2600.0)          # min Y, width, min Z, height of the flat's front face
SUN_CENTRE = (5985.0, -700.0, 2100.0)
SUN_SIZE = 900.0
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def _import(name, clamp=True):
    t = unreal.AssetImportTask()
    t.filename = "%s\\%s.png" % (BD_SRC, name)
    t.destination_path = BD_DIR; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tx = unreal.load_asset("%s/%s" % (BD_DIR, name))
    tx.set_editor_property("srgb", True)
    tx.set_editor_property("never_stream", True)        # always the full picture: the sun's face blurred while streaming
    if clamp:
        tx.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        tx.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    EAL.save_loaded_asset(tx)
    return tx


def _scalar(mat, name, value, x, y):
    p = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def _paint_material(name, tex, rect, masked):
    """Lit material whose picture is placed by world position: U from world Y, V from world Z (rect = minY, width, minZ, height).
    Base colour = picture; a little emissive (Glow) keeps the paint readable when the beat lighting is dark."""
    path = "%s/%s" % (BD_DIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BD_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if masked:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("opacity_mask_clip_value", 0.5)

    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1100, 0)
    names = ("MinY", "Width", "MinZ", "Height")
    params = [_scalar(mat, n, v, -1100, 120 + 90 * i) for i, (n, v) in enumerate(zip(names, rect))]
    cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -800, 100)
    cu.set_editor_property("code", "return float2((P.y - MinY) / Width, 1.0 - (P.z - MinZ) / Height);")
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    ins = []
    for n in ("P",) + names:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
    cu.set_editor_property("inputs", ins)
    MEL.connect_material_expressions(wp, "", cu, "P")
    for n, p in zip(names, params):
        MEL.connect_material_expressions(p, "", cu, n)

    ts = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 100)
    ts.set_editor_property("parameter_name", "Tex")
    ts.set_editor_property("texture", tex)
    MEL.connect_material_expressions(cu, "", ts, "UVs")

    glow = _scalar(mat, "Glow", 0.35, -500, 400)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 300)
    MEL.connect_material_expressions(ts, "RGB", mul, "A")
    MEL.connect_material_expressions(glow, "", mul, "B")
    MEL.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 500)
    rough.set_editor_property("r", 1.0)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 600)
    spec.set_editor_property("r", 0.0)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    if masked:
        MEL.connect_material_property(ts, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def dress_backdrop():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    sky_tex = _import("T_DC_Backdrop_Sky")
    sun_tex = _import("T_DC_Backdrop_Sun")
    sky_mat = _paint_material("M_DC_BackdropSky", sky_tex, FLAT_RECT, masked=False)
    sx, sy, sz = SUN_CENTRE
    sun_mat = _paint_material("M_DC_BackdropCutout", sun_tex, (sy - SUN_SIZE / 2, SUN_SIZE, sz - SUN_SIZE / 2, SUN_SIZE), masked=True)

    acts = {a.get_actor_label(): a for a in eas.get_all_level_actors() if a.get_level().get_outer().get_name() == "DC_Set_Hunt"}
    out = {}
    with unreal.ScopedEditorTransaction("Painted backdrop"):
        flat = acts["Backdrop_Flat"]
        flat.modify(); flat.static_mesh_component.modify()
        flat.static_mesh_component.set_material(0, sky_mat)
        out["flat"] = "M_DC_BackdropSky"

        sun = acts["Backdrop_Sun"]
        sun.modify(); c = sun.static_mesh_component; c.modify()
        c.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
        c.set_material(0, sun_mat)
        c.set_collision_profile_name("NoCollision")
        c.set_cast_shadow(False)
        sun.set_actor_location_and_rotation(unreal.Vector(sx, sy, sz), unreal.Rotator(roll=0.0, pitch=90.0, yaw=0.0), False, True)
        sun.set_actor_scale3d(unreal.Vector(SUN_SIZE / 100.0, SUN_SIZE / 100.0, 1.0))
        o, e = sun.get_actor_bounds(False)
        out["sun_bounds"] = [[round(o.x - e.x), round(o.y - e.y), round(o.z - e.z)], [round(o.x + e.x), round(o.y + e.y), round(o.z + e.z)]]

        hidden = []
        for i in range(4):
            a = acts.get("Backdrop_Cloud_%d" % i)
            if a:
                a.modify(); a.static_mesh_component.modify()
                a.static_mesh_component.set_editor_property("visible", False)
                hidden.append(a.get_actor_label())
        out["hidden"] = hidden
    return out
