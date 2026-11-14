# Director's Cut - populate DT_NarratorLines with the full Beat 1-5 script and import VO clips.
#
# Run inside the editor (Output Log, Python) or via UnrealEditor-Cmd:
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_BuildNarratorLines.py").read())
# Re-runnable: updates/adds every row in DT_NarratorLines and imports all RawAudio/VO/*.pcm clips.
import unreal, json, os, wave

TABLE = "/Game/DirectorsCut/Narrator/DT_NarratorLines"
SPEAKER = "THE DIRECTOR"

# (RowId, SubtitleText, DefaultDurationSeconds)
LINES = [
    # Beat 1: Intro Hunt (Kill 10 boars; interrupted at 7)
    ("L1_Intro_01",        "Welcome to the scene. Ah. Our leading actor. Well... casting has made a choice.", 4.5),
    ("L1_Intro_02",        "Those boars ate the village's crops. The villagers are starving. The boars, on the other hand, look fantastic.", 5.5),
    ("L1_Intro_03",        "Fortunately, we have you. And a sword. Apparently that was the entire budget.", 4.5),
    ("L1_Intro_04",        "Fight ten boars. Exactly ten. We're making cinema, not a census. And... action.", 5.5),
    ("L1_FirstHit",        "Ooh, contact. The boar is acting hurt. Very convincing. Somebody nominate the boar.", 5.0),
    ("L1_FirstKill",       "Two down. Hold that pose... no. Not that one. Keep going.", 4.5),
    ("L1_Half",            "Five. Halfway. The audience is on the edge of their seats. Mostly because the seats are broken.", 5.5),
    ("L1_PlayerHurt",      "Try not to die. The insurance on this production is... aspirational.", 4.5),
    ("L1_PlayerDied",      "CUT! Someone get the actor back to their mark. With feeling this time.", 4.5),
    ("L1_Idle",            "Hello? The boars are over there. I can see them from up here. I can see everything from up here.", 5.0),

    # Beat 2: Breakfast / Bacon (at 7/10 kills -> sees meat drop, wants 5 bacons + no-meat joke)
    ("B2_Intro_01",        "Hold it! Wait... did that boar just drop a piece of meat? Look at that.", 4.5),
    ("B2_Intro_02",        "I skipped breakfast for this shoot, and now I want bacon! Forget the ten boars, bring me five pieces of bacon right now.", 5.5),
    ("B2_Intro_02_Alt",    "Actually, I skipped breakfast and that looks delicious. Script change: forget the ten boars, get me five pieces of bacon.", 5.0),
    ("B2_FirstMeat",       "Ooh, yes, look at that marbling. Put it on the craft services table. Three more.", 4.5),
    ("B2_NoMeat_01",       "Wait... seriously? This one got no meat. What is it, stuffed with sawdust?", 4.5),
    ("B2_NoMeat_02",       "Another hollow boar. This one got no meat either. Who did our prop catering, an origami club?", 5.0),
    ("B2_AlmostDone",      "Four out of five. One more piece of bacon and I can finally eat like a real auteur.", 4.5),

    # Beat 3: Too Boring! Sword -> Gun, Hyperspawn, Kill 100 Boars
    ("B3_Intro_01",        "Ugh. Actually, watching you poke pigs with a butter knife is putting me to sleep.", 4.5),
    ("B3_Intro_02",        "New genre. Action blockbuster! Lose the sword, take this gun. Props department, crank the boar spawner to maximum!", 6.0),
    ("B3_Intro_03",        "Give me a hundred dead boars! Make it loud! Make it messy! Action!", 4.5),
    ("B3_Intro_03_Alt",    "One hundred boars! Don't stop shooting until the frame rate begs for mercy!", 5.0),
    ("B3_Progress_25",     "Twenty-five down! Now THAT is cinema! Keep firing!", 4.0),
    ("B3_Progress_50",     "Fifty! Halfway to a hundred! Someone get slow-motion cameras on that pork!", 4.5),
    ("B3_Progress_80",     "Eighty! Twenty more! My blood pressure is finally at a healthy level!", 4.5),

    # Beat 4: Romance! Rescue the Princess from the Shed Guarded by the King Boar
    ("B4_Intro_01",        "Wait, stop! Cut! Cut! Look at this carnage. It's way too violent! The ratings board will crucify me!", 6.0),
    ("B4_Intro_02",        "We need romance! Heart! Whimsy! Quick, turn on the pink lights!", 4.5),
    ("B4_Intro_03",        "There is a lovely princess locked in the shed over there. Go rescue her! Oh, and the King Boar is guarding it. Try to look heroic.", 6.5),
    ("B4_ShedLocked",      "Uh, hello? The giant King Boar with the crown is standing right there. Deal with him first.", 5.0),
    ("B4_KingBoarDead",    "The King Boar is down! Now open that shed and sweep the princess off her feet. Cue the violins!", 5.0),
    ("B4_PrincessRescued", "And the hero saves the... wait. Look at your face. You have zero romantic chemistry.", 5.0),

    # Beat 5: The Director Gets Mad ("Bad Actor") -> Boss Fight
    ("B5_Intro_01",        "CUT! CUT! CUT! Forget it! I can't watch this anymore!", 4.0),
    ("B5_Intro_02",        "That performance was hopeless! Wooden delivery, zero timing, and you missed every single cue!", 5.5),
    ("B5_Intro_03",        "I'm coming down there myself! If you want a scene done right, the Director does it personally! Fight me!", 5.5),
    ("B5_BossTaunt_01",    "That's not your motivation! Hit your mark!", 3.5),
    ("B5_BossTaunt_02",    "You're fired! Recast! Somebody call my agent!", 4.0),
    ("B5_BossHalfHP",      "Ow! Hey! That's against union regulations! You can't shoot the director!", 4.5),
    ("B5_DirectorDefeated","Ugh... fine. You win. Terrible performance... five stars. That's... a wrap.", 5.5),
]


def _load(package_path):
    return unreal.load_object(None, "%s.%s" % (package_path, package_path.rsplit("/", 1)[-1]))


def build_lines():
    dt = _load(TABLE)
    if not dt:
        raise RuntimeError("DataTable not found: " + TABLE)

    existing = {r["Name"]: r for r in json.loads(dt.export_to_json_string())}
    rows = []
    for row_id, text, dur in LINES:
        prev = existing.get(row_id, {})
        rows.append({
            "Name": row_id,
            "Speaker": SPEAKER,
            "Text": text,
            "Duration": prev.get("Duration", dur),
            "Voice": prev.get("Voice", ""),
        })

    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows))
    if not ok:
        raise RuntimeError("fill_data_table_from_json_string failed")
    unreal.EditorLoadingAndSavingUtils.save_packages([dt.get_outermost()], False)
    unreal.log("DC_BuildNarratorLines: wrote %d rows to %s" % (len(rows), TABLE))


build_lines()
exec(open(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts", "DC_ImportVoiceLines.py")).read())
