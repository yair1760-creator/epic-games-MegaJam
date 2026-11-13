# Director's Cut - import Director voice lines and assign them in DT_NarratorLines.
#
# Input: RawAudio/VO/<RowId>.pcm (raw s16le mono, e.g. ElevenLabs "pcm_24000") or RawAudio/VO/<RowId>.wav
# Output: /Game/DirectorsCut/Audio/VO/VO_<RowId> sound waves, set as the Voice of row <RowId>.
#         Each voiced row's Duration becomes clip length + TAIL, so the subtitle outlasts the last syllable.
#
# Run inside the editor (Output Log, Python):
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_ImportVoiceLines.py").read())
# Re-runnable: re-imports over existing assets. Files with no matching row are reported and skipped.
import unreal, os, json, wave

PCM_RATE = 24000
TAIL = 0.4
RAW_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawAudio", "VO")
WAV_DIR = os.path.join(RAW_DIR, "wav")
DEST = "/Game/DirectorsCut/Audio/VO"
TABLE = "/Game/DirectorsCut/Narrator/DT_NarratorLines"


def _load(package_path):
    return unreal.load_object(None, "%s.%s" % (package_path, package_path.rsplit("/", 1)[-1]))


def _to_wav(src, row):
    if src.lower().endswith(".wav"):
        return src
    os.makedirs(WAV_DIR, exist_ok=True)
    dst = os.path.join(WAV_DIR, row + ".wav")
    with open(src, "rb") as f:
        frames = f.read()
    with wave.open(dst, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(PCM_RATE)
        w.writeframes(frames)
    return dst


def import_voice_lines():
    dt = _load(TABLE)
    rows = json.loads(dt.export_to_json_string())
    by_name = {r["Name"]: r for r in rows}

    files = {}
    for fn in sorted(os.listdir(RAW_DIR)):
        stem, ext = os.path.splitext(fn)
        if ext.lower() in (".pcm", ".wav"):
            files[stem] = os.path.join(RAW_DIR, fn)

    report, tasks = [], []
    for row, src in files.items():
        if row not in by_name:
            report.append("SKIP %s: no row in DT_NarratorLines" % row)
            continue
        t = unreal.AssetImportTask()
        t.filename = _to_wav(src, row)
        t.destination_path = DEST
        t.destination_name = "VO_" + row
        t.replace_existing = True
        t.automated = True
        tasks.append((row, t))

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, t in tasks])

    for row, t in tasks:
        path = "%s/VO_%s" % (DEST, row)
        snd = _load(path)
        if not snd:
            report.append("FAIL %s: import failed" % row)
            continue
        length = snd.get_editor_property("duration")
        by_name[row]["Voice"] = snd.get_path_name()
        by_name[row]["Duration"] = round(length + TAIL, 2)
        report.append("OK   %s: %.2fs" % (row, length))

    for name in by_name:
        if name not in files:
            report.append("NOTE %s: no voice file" % name)

    unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows))
    to_save = [dt.get_outermost()] + [s.get_outermost() for s in (_load("%s/VO_%s" % (DEST, r)) for r, _ in tasks) if s]
    if not unreal.EditorLoadingAndSavingUtils.save_packages(to_save, False):
        report.append("FAIL saving packages")
    return "\n".join(report)


result = import_voice_lines()
print(result)
result
