# Rated PG for Pig - brings Taisho's Nov 15 art into the game (run inside the editor, not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\DC_ImportTaishoArt.py").read()); import_taisho_art(); place_railroad()
# Frames come from Scripts/Blender_ExportTaishoArt.py (Art/Meshy/<Name>/Frames).
# - T_FX_SmokePuff_01..11  -> /Game/DirectorsCut/FX/Textures   (boar death puff, Director defeat puff)
# - T_FX_StarsPuff_01..04  -> /Game/DirectorsCut/FX/Textures   (sword hit)
# - T_DC_KingCrown         -> /Game/DirectorsCut/Characters/Boar/Sprites  (crown on Crown King Bacon Jr)
# - T_UI_BaconBar_<hp>     -> /Game/DirectorsCut/UI/Boss  (the King's boss bar; <hp> = the HP drawn on it, out of 120)
# - SM_DC_RailroadTrack    -> /Game/DirectorsCut/Environment/Placeholders/SM_DC_RailroadTrack (Nanite on)
import os
import unreal

PROJECT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
ART = os.path.join(PROJECT, "Art", "Meshy")
EAL = unreal.EditorAssetLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

# the bacon bar layers are named 90..0, but the numbers drawn on them are what count (Taisho's 30 layer reads 48/120)
BACON = {"90": 108, "80": 96, "70": 84, "60": 72, "50": 60, "30": 48, "20": 24, "10": 12, "0": 0}
RAIL = "/Game/DirectorsCut/Environment/Placeholders/SM_DC_RailroadTrack/SM_DC_RailroadTrack"


def _import_png(src, dest_dir, name, like):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest_dir; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tx = unreal.load_asset(dest_dir + "/" + name)
    ref = unreal.load_asset(like)
    for p in ("compression_settings", "mip_gen_settings", "lod_group", "srgb", "never_stream", "filter", "address_x", "address_y"):
        tx.set_editor_property(p, ref.get_editor_property(p))
    EAL.save_loaded_asset(tx)
    return tx


def import_taisho_art():
    out = {}
    fx = "/Game/DirectorsCut/FX/Textures"
    like_fx = fx + "/T_FX_Poof_0"
    for i in range(1, 12):
        _import_png(os.path.join(ART, "PuffOfSmoke", "Frames", "T_FX_SmokePuff_%02d.png" % i), fx, "T_FX_SmokePuff_%02d" % i, like_fx)
    for i in range(1, 5):
        _import_png(os.path.join(ART, "StarsPuff", "Frames", "T_FX_StarsPuff_%02d.png" % i), fx, "T_FX_StarsPuff_%02d" % i, like_fx)
    crown = _import_png(os.path.join(ART, "JustTheCrown", "Frames", "Just_The_Crown.png"), "/Game/DirectorsCut/Characters/Boar/Sprites",
                        "T_DC_KingCrown", "/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_KingCrown_Placeholder")
    out["crown"] = [crown.blueprint_get_size_x(), crown.blueprint_get_size_y()]
    for layer, hp in BACON.items():
        _import_png(os.path.join(ART, "BaconBar", "Frames", "T_DC_BaconBar_%s.png" % layer), "/Game/DirectorsCut/UI/Boss",
                    "T_UI_BaconBar_%d" % hp, "/Game/DirectorsCut/UI/Menu/T_UI_Button_Start")

    if not EAL.does_asset_exist(RAIL):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(ART, "RailroadTrack", "Meshy_AI_Rusty_Railroad_Track_20261115_texture_fbx", "Meshy_AI_Rusty_Railroad_Track_20261115_texture.fbx")
        t.destination_path = RAIL.rsplit("/", 1)[0]; t.destination_name = "SM_DC_RailroadTrack"
        t.automated = True; t.replace_existing = True; t.save = True
        o = unreal.FbxImportUI()
        o.set_editor_property("import_mesh", True); o.set_editor_property("import_as_skeletal", False)
        o.set_editor_property("import_materials", True); o.set_editor_property("import_textures", True)
        o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        sm = o.get_editor_property("static_mesh_import_data")
        sm.set_editor_property("combine_meshes", True); sm.set_editor_property("auto_generate_collision", False)
        t.options = o
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    m = unreal.load_asset(RAIL)
    ns = SMS.get_nanite_settings(m)
    if not ns.enabled:                      # 245k triangles: Nanite keeps ten copies cheap
        ns.enabled = True; SMS.set_nanite_settings(m, ns, apply_changes=True)
    EAL.save_loaded_asset(m)
    b = m.get_bounding_box()
    out["rail_bounds_cm"] = [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)]
    return out


def place_railroad(scale=2.0):
    """Beat 3 (DC_Set_FilmCrew): one track segment per painted rail pair (Dolly_Rail_In_k / Out_k), centred between them,
    turned like them; the painted bars are hidden (no collision change)."""
    L, F = "DC_Set_FilmCrew", "DC/Set_FilmCrew"
    acts = [a for a in EAS.get_all_level_actors() if a.get_level().get_outer().get_name() == L]
    for a in acts:
        if a.get_actor_label().startswith("PH_Railroad_"):
            EAS.destroy_actor(a)
    rails = {a.get_actor_label(): a for a in acts if a.get_actor_label().startswith("Dolly_Rail_")}
    m = unreal.load_asset(RAIL)
    b = m.get_bounding_box()
    LES.set_current_level_by_name(L)
    n = 0
    try:
        for k in range(10):
            a_in, a_out = rails.get("Dolly_Rail_In_%d" % k), rails.get("Dolly_Rail_Out_%d" % k)
            if not a_in or not a_out:
                continue
            c = (a_in.get_actor_location() + a_out.get_actor_location()) * 0.5
            yaw = a_in.get_actor_rotation().yaw
            seg = EAS.spawn_actor_from_object(m, unreal.Vector(c.x, c.y, -b.min.z * scale), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
            seg.set_actor_scale3d(unreal.Vector(scale, scale, scale))
            seg.set_actor_label("PH_Railroad_%d" % k); seg.set_folder_path(F)
            c2 = seg.static_mesh_component
            c2.set_collision_profile_name("NoCollision"); c2.set_editor_property("can_ever_affect_navigation", False)
            for a in (a_in, a_out):
                a.modify(); a.static_mesh_component.modify(); a.static_mesh_component.set_editor_property("visible", False)
            n += 1
    finally:
        LES.set_current_level_by_name("Lvl_DirectorsCut")
    return n
