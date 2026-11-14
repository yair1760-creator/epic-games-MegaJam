# Director's Cut - import all 16 fal.ai SFX cues from Art/Audio/SFX_fal/ into /Game/DirectorsCut/Audio/SFX.
#
# Replaces the 13 synthesized placeholders with the fal.ai takes and imports the 3 bonus cues
# (S_Cut, S_QuestTick, S_QuestComplete).
#
# To pick a different take for any cue, change its number (1, 2, or 3) in PICKS and re-run:
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_ImportSFX.py").read())
import os
import unreal

PROJ_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC_DIR = os.path.join(PROJ_DIR, "Art", "Audio", "SFX_fal")
DEST = "/Game/DirectorsCut/Audio/SFX"

# AssetName -> take index (1, 2, or 3) from Art/Audio/SFX_fal/<AssetName>_t<N>.wav
PICKS = {
    "S_SwordSwing":    2,
    "S_SwordHit":      1,
    "S_BoarGrunt":     1,
    "S_BoarAggro":     3,
    "S_BoarCharge":    1,
    "S_BoarBite":      1,
    "S_BoarSqueal":    1,
    "S_BoarDeath":     1,
    "S_PlayerHurt":    2,
    "S_MeatPickup":    3,
    "S_BossShot":      3,
    "S_BossHit":       2,
    "S_BossDefeat":    3,
    "S_Cut":           1,
    "S_QuestTick":     2,
    "S_QuestComplete": 1,
}


def _load(package_path):
    name = package_path.rsplit("/", 1)[-1]
    return unreal.load_object(None, "%s.%s" % (package_path, name))


def import_sfx():
    tasks = []
    report = []

    for asset_name, take in PICKS.items():
        src_file = "%s_t%d.wav" % (asset_name, take)
        src_path = os.path.join(SRC_DIR, src_file)
        if not os.path.isfile(src_path):
            report.append("MISSING %s: %s" % (asset_name, src_path))
            continue

        task = unreal.AssetImportTask()
        task.filename = src_path
        task.destination_path = DEST
        task.destination_name = asset_name
        task.replace_existing = True
        task.automated = True
        tasks.append((asset_name, src_file, task))

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, _, t in tasks])

    to_save = []
    for asset_name, src_file, _ in tasks:
        pkg_path = "%s/%s" % (DEST, asset_name)
        snd = _load(pkg_path)
        if not snd:
            report.append("FAIL %s: import failed" % asset_name)
            continue

        snd.set_editor_property("looping", False)
        duration = snd.get_editor_property("duration")
        to_save.append(snd.get_outermost())
        report.append("OK   %-16s <- %-22s (%.2fs)" % (asset_name, src_file, duration))

    if to_save:
        if not unreal.EditorLoadingAndSavingUtils.save_packages(to_save, False):
            report.append("FAIL saving SFX packages")

    summary = "\n".join(report)
    unreal.log("DC_ImportSFX:\n" + summary)
    return summary


if __name__ == "__main__":
    import_sfx()
