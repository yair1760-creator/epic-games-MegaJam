# Rated PG for Pig - the Director's thrown clapperboard frames (Scripts/Blender_ClapperSpin.py). Run inside the editor:
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\DC_ImportClapper.py").read()); import_clapper()
import os
import unreal

SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Director\Clapper"
DEST = "/Game/DirectorsCut/Characters/Director"
LIKE = "/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_KingCrown"


def import_clapper():
    ref = unreal.load_asset(LIKE)
    out = []
    for i in range(1, 5):
        n = "T_Director_Clapper_%02d" % i
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, "Clapper_%02d.png" % i); t.destination_path = DEST; t.destination_name = n
        t.automated = True; t.replace_existing = True; t.save = False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
        tx = unreal.load_asset(DEST + "/" + n)
        for p in ("compression_settings", "mip_gen_settings", "lod_group", "srgb", "never_stream", "filter", "address_x", "address_y"):
            tx.set_editor_property(p, ref.get_editor_property(p))
        unreal.EditorAssetLibrary.save_loaded_asset(tx)
        out.append(tx.blueprint_get_size_x())
    return out
