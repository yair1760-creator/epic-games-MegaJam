# Rated PG for Pig - the Director's boss frames and dialogue portrait (team art, Nov 16). Run inside the editor:
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\DC_ImportDirectorArt.py").read()); import_director_art()
# - Art/Director/Trimmed/Director_<Pose>.png (Scripts/Blender_TrimDirectorFrames.py) -> /Game/DirectorsCut/Characters/Director/T_Director_<Pose>
# - Art/Director/Portrait/Director_Profile.png -> replaces /Game/DirectorsCut/UI/Art/T_DC_Director_Portrait (victory card, dialogue)
import os
import unreal

ART = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director"
DEST = "/Game/DirectorsCut/Characters/Director"
POSES = ("Angry", "Run_1", "Run_2", "Throw_1", "Throw_2", "Hit", "Dying", "Dead")
LIKE = "/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_KingCrown"
PROPS = ("compression_settings", "mip_gen_settings", "lod_group", "srgb", "never_stream", "filter", "address_x", "address_y")


def _imp(src, dest, name, like):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tx = unreal.load_asset(dest + "/" + name)
    if like:
        ref = unreal.load_asset(like)
        for p in PROPS:
            tx.set_editor_property(p, ref.get_editor_property(p))
    unreal.EditorAssetLibrary.save_loaded_asset(tx)
    return [tx.blueprint_get_size_x(), tx.blueprint_get_size_y()]


def import_director_art():
    out = {}
    for p in POSES:
        out[p] = _imp(os.path.join(ART, "Trimmed", "Director_%s.png" % p), DEST, "T_Director_" + p, LIKE)
    out["portrait"] = _imp(os.path.join(ART, "Portrait", "Director_Profile.png"), "/Game/DirectorsCut/UI/Art", "T_DC_Director_Portrait", None)
    return out
