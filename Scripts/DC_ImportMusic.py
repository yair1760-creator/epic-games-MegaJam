# Director's Cut - import the 5 music cues from Art/Audio/Music_fal/ into /Game/DirectorsCut/Audio/Music.
#
# Defaults to the Stable Audio 3 WAV takes (M_*_sa3.wav).
# To swap any track to a Sonilo take later, convert the .m4a to .wav and re-run or reimport over the same asset.
#
# Run inside the editor (Output Log, Python) or via UnrealEditor-Cmd:
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_ImportMusic.py").read())
# Re-runnable: re-imports over existing assets and sets looping flags.
import os
import unreal

PROJ_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC_DIR = os.path.join(PROJ_DIR, "Art", "Audio", "Music_fal")
DEST = "/Game/DirectorsCut/Audio/Music"

# (AssetName, SourceFilename, bLooping)
TRACKS = [
    ("M_Heroic",  "M_Heroic_sa3.wav",  True),
    ("M_Frantic", "M_Frantic_sa3.wav", True),
    ("M_Romance", "M_Romance_sa3.wav", True),
    ("M_Boss",    "M_Boss_sa3.wav",    True),
    ("M_Victory", "M_Victory_sa3.wav", False),
]


def _load(package_path):
    name = package_path.rsplit("/", 1)[-1]
    return unreal.load_object(None, "%s.%s" % (package_path, name))


def import_music():
    tasks = []
    report = []

    for asset_name, src_file, looping in TRACKS:
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
        tasks.append((asset_name, looping, task))

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, _, t in tasks])

    to_save = []
    for asset_name, looping, _ in tasks:
        pkg_path = "%s/%s" % (DEST, asset_name)
        snd = _load(pkg_path)
        if not snd:
            report.append("FAIL %s: import failed" % asset_name)
            continue

        snd.set_editor_property("looping", looping)
        duration = snd.get_editor_property("duration")
        to_save.append(snd.get_outermost())
        report.append("OK   %s: %.2fs (looping=%s)" % (asset_name, duration, looping))

    if to_save:
        if not unreal.EditorLoadingAndSavingUtils.save_packages(to_save, False):
            report.append("FAIL saving music packages")

    summary = "\n".join(report)
    unreal.log("DC_ImportMusic:\n" + summary)
    return summary


if __name__ == "__main__":
    import_music()
