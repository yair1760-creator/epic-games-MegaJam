# Writes Docs/Plan_CombatAI_Sprites.xlsx with only the standard library (no openpyxl on this machine).
import zipfile
from xml.sax.saxutils import escape

OUT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Docs\Plan_CombatAI_Sprites.xlsx"

# ---------------------------------------------------------------- styles (cellXfs indices)
S_DEF, S_TITLE, S_ITAL, S_HDR, S_BODY, S_BODY_G, S_NUM, S_NUM_G, S_APPR, S_BOLD, S_BOLDNUM, S_LEG_Y, S_LEG_G, S_BLUE, S_H2 = range(15)
STYLES = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
<numFmts count="1"><numFmt numFmtId="164" formatCode="0.0"/></numFmts>
<fonts count="7">
<font><sz val="10"/><name val="Arial"/></font>
<font><b/><sz val="14"/><name val="Arial"/></font>
<font><i/><sz val="10"/><name val="Arial"/></font>
<font><b/><sz val="10"/><color rgb="FFFFFFFF"/><name val="Arial"/></font>
<font><sz val="10"/><color rgb="FF0000FF"/><name val="Arial"/></font>
<font><b/><sz val="10"/><name val="Arial"/></font>
<font><b/><sz val="13"/><name val="Arial"/></font>
</fonts>
<fills count="5">
<fill><patternFill patternType="none"/></fill>
<fill><patternFill patternType="gray125"/></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FF1F2937"/><bgColor indexed="64"/></patternFill></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FFFFFF00"/><bgColor indexed="64"/></patternFill></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FFD1FAE5"/><bgColor indexed="64"/></patternFill></fill>
</fills>
<borders count="2">
<border><left/><right/><top/><bottom/><diagonal/></border>
<border><left style="thin"><color rgb="FFBFBFBF"/></left><right style="thin"><color rgb="FFBFBFBF"/></right><top style="thin"><color rgb="FFBFBFBF"/></top><bottom style="thin"><color rgb="FFBFBFBF"/></bottom><diagonal/></border>
</borders>
<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>
<cellXfs count="15">
<xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>
<xf numFmtId="0" fontId="1" fillId="0" borderId="0" xfId="0" applyFont="1"/>
<xf numFmtId="0" fontId="2" fillId="0" borderId="0" xfId="0" applyFont="1"/>
<xf numFmtId="0" fontId="3" fillId="2" borderId="1" xfId="0" applyFont="1" applyFill="1" applyBorder="1" applyAlignment="1"><alignment vertical="center" wrapText="1"/></xf>
<xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0" applyBorder="1" applyAlignment="1"><alignment vertical="top" wrapText="1"/></xf>
<xf numFmtId="0" fontId="0" fillId="4" borderId="1" xfId="0" applyFill="1" applyBorder="1" applyAlignment="1"><alignment vertical="top" wrapText="1"/></xf>
<xf numFmtId="164" fontId="4" fillId="0" borderId="1" xfId="0" applyNumberFormat="1" applyFont="1" applyBorder="1" applyAlignment="1"><alignment vertical="top"/></xf>
<xf numFmtId="164" fontId="4" fillId="4" borderId="1" xfId="0" applyNumberFormat="1" applyFont="1" applyFill="1" applyBorder="1" applyAlignment="1"><alignment vertical="top"/></xf>
<xf numFmtId="0" fontId="0" fillId="3" borderId="1" xfId="0" applyFill="1" applyBorder="1" applyAlignment="1"><alignment vertical="top" wrapText="1"/></xf>
<xf numFmtId="0" fontId="5" fillId="0" borderId="0" xfId="0" applyFont="1"/>
<xf numFmtId="164" fontId="5" fillId="0" borderId="0" xfId="0" applyNumberFormat="1" applyFont="1"/>
<xf numFmtId="0" fontId="0" fillId="3" borderId="1" xfId="0" applyFill="1" applyBorder="1"/>
<xf numFmtId="0" fontId="0" fillId="4" borderId="1" xfId="0" applyFill="1" applyBorder="1"/>
<xf numFmtId="0" fontId="4" fillId="0" borderId="0" xfId="0" applyFont="1"/>
<xf numFmtId="0" fontId="6" fillId="0" borderId="0" xfId="0" applyFont="1"/>
</cellXfs>
<cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles>
</styleSheet>"""


def col(n):
    s = ""
    while n:
        n, r = divmod(n - 1, 26)
        s = chr(65 + r) + s
    return s


class Sheet:
    def __init__(self, name):
        self.name, self.cells, self.widths, self.merges, self.dv, self.freeze = name, {}, {}, [], [], None

    def set(self, r, c, v, s=S_DEF):
        self.cells[(r, c)] = (v, s)

    def xml(self):
        out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',
               '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
               'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">']
        if self.freeze:
            r, c = self.freeze
            out.append('<sheetViews><sheetView workbookViewId="0"><pane xSplit="%d" ySplit="%d" topLeftCell="%s%d" '
                       'activePane="bottomRight" state="frozen"/></sheetView></sheetViews>' % (c - 1, r - 1, col(c), r))
        if self.widths:
            out.append("<cols>" + "".join('<col min="%d" max="%d" width="%s" customWidth="1"/>' % (c, c, w)
                                          for c, w in sorted(self.widths.items())) + "</cols>")
        out.append("<sheetData>")
        rows = sorted({r for r, _ in self.cells})
        for r in rows:
            out.append('<row r="%d">' % r)
            for c in sorted(c for rr, c in self.cells if rr == r):
                v, s = self.cells[(r, c)]
                ref = "%s%d" % (col(c), r)
                if v is None:
                    out.append('<c r="%s" s="%d"/>' % (ref, s))
                elif isinstance(v, str) and v.startswith("="):
                    out.append('<c r="%s" s="%d"><f>%s</f></c>' % (ref, s, escape(v[1:])))
                elif isinstance(v, (int, float)):
                    out.append('<c r="%s" s="%d"><v>%s</v></c>' % (ref, s, v))
                else:
                    out.append('<c r="%s" s="%d" t="inlineStr"><is><t xml:space="preserve">%s</t></is></c>' % (ref, s, escape(str(v))))
            out.append("</row>")
        out.append("</sheetData>")
        if self.merges:
            out.append('<mergeCells count="%d">' % len(self.merges) + "".join('<mergeCell ref="%s"/>' % m for m in self.merges) + "</mergeCells>")
        if self.dv:
            out.append('<dataValidations count="%d">' % len(self.dv))
            for ref, lst in self.dv:
                out.append('<dataValidation type="list" allowBlank="1" showDropDown="0" sqref="%s"><formula1>"%s"</formula1></dataValidation>' % (ref, escape(lst)))
            out.append("</dataValidations>")
        out.append("</worksheet>")
        return "".join(out)


# ---------------------------------------------------------------- Plan sheet
plan = Sheet("Plan")
plan.set(1, 1, "Director's Cut: Sword + player HP, Boar AI, camera-facing sprites. Build plan, one system at a time", S_TITLE)
plan.set(2, 1, "Each system is built, tested in Lvl_Test_CombatAI, and approved before the next one starts. "
               "Yellow cells are yours: set Approval to Approved (or write notes) per system.", S_ITAL)
plan.merges.append("A2:L2")
cols = ["#", "System", "Package", "What it does", "Code folder (Source/EpicGameJamGame/...)",
        "Content folder (/Game/...)", "How to test in the test map", "Done when", "Depends on",
        "Est. hours", "Status", "Approval (you)"]
widths = [5, 24, 10, 48, 30, 26, 40, 40, 11, 10, 14, 18]
HR = 4
for i, (c, w) in enumerate(zip(cols, widths), 1):
    plan.set(HR, i, c, S_HDR)
    plan.widths[i] = w

rows = [
 (0, "Test map", "Setup",
  "Private map Lvl_Test_CombatAI with one zone per system: A sword lane (dummies, 1 m range ticks, damage pad), "
  "B fenced boar arena (6 BoarSpawn points, rocks to path around, nav mesh), F sprite gallery (walk-around ring, slots at 3 distances/heights).",
  "(none; builder: Scripts/Test_BuildCombatAIMap.py)", "DirectorsCut/TestMaps", "Open the map, press Play, walk to each zone.",
  "Map opens and plays; main map untouched.", "-", 0.5, "Done (review)"),
 (1, "Damage + health component", "B (shared)",
  "Reusable health component: max HP, current HP, damage, heal, death, OnDamaged / OnDied events. Listens to Unreal's standard damage, so anything that deals damage works on it.",
  "DirectorsCut/Combat", "DirectorsCut/Combat",
  "Put the component on a test dummy; a console command damages the actor you look at. Watch HP in the log.",
  "Dummy loses HP, dies at 0, events fire once.", "0", 1.5, "Done (review)"),
 (2, "Player HP + hit feedback", "A",
  "Player keeps the Shooter character's existing HP and respawn. Add hit feedback (red screen flash + small camera shake) and expose HP % / died events for the HUD team (they draw the bar).",
  "Variant_Shooter (ShooterCharacter), DirectorsCut/Combat", "DirectorsCut/Combat",
  "Stand on the red damage pad in zone A: HP drops, flash + shake play, player dies and respawns at spawn.",
  "Hit feedback visible; death and respawn work; HP event documented for HUD.", "1", 1.5, "Done (review)"),
 (3, "Sword (melee weapon)", "A",
  "Sword built on the existing ShooterWeapon class: editable damage, range and cooldown, swing trace in front of the player, hit feedback on the target. Player starts holding the sword.",
  "DirectorsCut/Combat, Variant_Shooter/Weapons", "DirectorsCut/Combat",
  "Zone A: swing at the dummies from the yellow line; use the 1 m ticks to tune range; check cooldown.",
  "Hits only inside range, respects cooldown, damage editable in the Blueprint, player spawns with it.", "1, 2", 3, "Done (review)"),
 (4, "Boar: body + wander", "B",
  "Boar character with the health component, an AI controller, and a calm wander inside the nav mesh. Placeholder body until the sprite is in (system 9).",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Place one boar in zone B; it wanders around the rocks without getting stuck.",
  "Wanders smoothly for 2 minutes; never leaves the fence.", "1", 2, "Done (review)"),
 (5, "Boar: angry, charge, bite", "B",
  "When hit, the boar turns angry, charges the player, and bites at close range (damages player HP).",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Hit a boar with the sword in zone B: it charges and bites; your HP drops.",
  "Clear state change on hit; bite damage and cooldown editable.", "3, 4", 2.5, "Done (review)"),
 (6, "Boar: flee at low HP + death", "B",
  "Below an editable HP threshold the boar runs away from the player; at 0 HP it dies (simple death, then removed). Fires a 'boar killed' event the quest team can count.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Hit a boar until low HP: it runs; finish it: it dies and the event is logged.",
  "Flee + death work; 'boar killed' event documented for the quest team.", "5", 1.5, "Done (review)"),
 (7, "Boar spawner", "B",
  "Spawner that uses every actor tagged BoarSpawn (the 6 points in the main map and in the test map). Editable max alive, respawn delay, start/stop.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Zone B: spawner keeps N boars alive using the 6 yellow spawn marks.",
  "Uses tagged points only (no main-map edits); count and delay editable.", "6", 1.5, "Done (review)"),
 (8, "Boar HP bar", "B",
  "Small HP bar floating over each boar, shown when it is damaged, hidden when full.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Hit a boar: bar appears and shrinks; readable at 10 m.",
  "Bar follows the boar, faces the player, hides at full HP.", "6", 1, "Done (review)"),
 (9, "Camera-facing sprite", "F",
  "Reusable sprite component that always turns to face the camera (upright, yaw only). Put it on the boar now; the Director uses it later.",
  "DirectorsCut/Rendering", "DirectorsCut/Rendering",
  "Zone F: walk the yellow ring around the centre slot; check the slots at different distances and heights; then a boar in zone B.",
  "Sprite never shows its edge or back; works on the boar.", "4", 2, "Done (review)"),
 (11, "Boar meat drops (bacon quest)", "B",
  "Built (SetMeatDropsEnabled + ADCBoarMeat), then REMOVED: the lead's ADCBoarMeatPickup (spawned by DCGameMode) already does this, so only one meat system remains.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "n/a (removed)",
  "Our meat code is gone; boars still report Kill.Boar.", "6", 1.5, "Removed"),
 (10, "Integration pass + handoff notes", "A, B, F",
  "Full loop in the test map: start with sword, boars spawn, fight, flee, die, kill events. Short handoff doc for the team (classes, events, tags) so they can drop it into the main map.",
  "-", "Docs", "Play zones A and B start to finish.",
  "Everything works together; doc in Docs/; nothing saved in the main map.", "1-9", 1.5, "Done (review)"),
 # ---- round 2 (lead's Dev 2 list, 2026-11-13) ----
 (13, "P0 Production cleanup", "B",
  "Boar debug label off on BP_DC_Boar. Lead's ADCBoarMeatPickup (Combat) shows a camera-facing ham sprite (UDCBillboardSprite) instead of cube/cylinder parts. BP_DCBoarSpawner reaches main with our next PR.",
  "DirectorsCut/Combat", "DirectorsCut/Combat, Characters/Boar/Sprites",
  "Zone B: boars have no floating label; DC.StartQuest + meat beat: the meat is a ham sprite that faces you.",
  "No debug text on boars; meat is a sprite; pickup still reports Collect.Meat.", "11", 1, "Done (review)"),
 (14, "P0 Balance + weapon rules", "A, B",
  "Sword 25 dmg so boars (60 HP) hit the 35% flee point on the 2nd hit; boar bite 10; player MaxHP 100. Tune King Boar / Director boss HP, speed, projectile and Beat 3 hyperspawn (25 HP). Sword-only in Beats 1-2 (no weapon switch / reload), gun ammo + reload work in Beats 3-5.",
  "DirectorsCut/Combat, DirectorsCut/AI, Variant_Shooter", "DirectorsCut/AI, Combat",
  "Test map: hit a boar twice -> it flees, third hit kills. Full Level 1 playtest in PIE (no main-map save) for boss numbers.",
  "Numbers match the plan; flee happens; a full run is beatable but not trivial.", "13", 2, "Testing"),
 (15, "P0 King Boar visuals", "B",
  "King Boar = boar sprite scaled up + gold tint + a sprite crown on top; remove the rotating cylinder and the forced 'KING BOAR' debug text.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Spawn the King Boar in zone B: big golden boar with a crown, no debug text, still faces the camera.",
  "Reads as 'king boar' at 15 m; no primitives left.", "13", 1.5, "Done (review)"),
 (16, "P0 Director boss + projectile visuals", "B, F",
  "Swap the cube/cylinder Director for a UDCBillboardSprite with idle / attack / hurt / death frames, and give the projectile a sprite + trail + hit puff. NEEDS Designer 1's Director frames; until then a placeholder drawing.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Spawn the boss in the test map: sprite faces you, swaps frames on attack/hurt/death; shots are visible and pop on hit.",
  "No primitives on the boss or shots.", "9", 2.5, "Waiting art"),
 (17, "P0 Princess visuals", "F",
  "Swap the cylinder/cube Princess for a camera-facing sprite (idle bob). Optional: shed door opens. NEEDS a Princess drawing; until then a placeholder.",
  "DirectorsCut/AI", "DirectorsCut/AI",
  "Place the Princess in the test map: sprite faces you and bobs.",
  "No primitives on the Princess.", "9", 1, "Done (review)"),
 (18, "P1 Audio + VFX hooks (code)", "A, B",
  "Editable sound / Niagara slots + playback on the sword (swing, hit, trail), boar (grunt, charge, squeal, death, death poof), player (hurt), boss (attack, hit, defeat). Empty slot = silent, so it works before the files exist.",
  "DirectorsCut/Combat, DirectorsCut/AI, Variant_Shooter", "DirectorsCut/Audio (slots only)",
  "Test map with placeholder sounds: each action plays its sound once, at the right moment, with distance falloff.",
  "Every event in the list has a slot and plays it; nothing spams.", "13", 2, "Done (review)"),
 (19, "P1 SFX + music files", "Audio (shared)",
  "Make S_* sound effects and the 5 music loops (heroic, frantic, pastel romance, boss, victory sting). Sources allowed by the jam: AI tools (fal: ElevenLabs SFX v3, Stable Audio, Sonilo SFX/music, Lyria, ElevenLabs Music) and licensed assets (WeLoveIndies free non-commercial pack, Fab). List every source in the submission.",
  "-", "DirectorsCut/Audio/SFX, DirectorsCut/Audio/Music",
  "Plug into the slots from system 18; play Level 1 and listen.",
  "All 13 SFX + 5 music cues in; sources listed in Docs.", "18", 3, "Needs owner"),
 (20, "P1 Boar side/back views + hit flash overlay", "B, F",
  "Plug side/back boar frames into the sprite when Designer 1 delivers them. Move the red hit flash from camera fade to a HUD overlay so a Director 'CUT!' fade can't be overridden.",
  "DirectorsCut/Combat, DirectorsCut/AI", "DirectorsCut/Characters/Boar",
  "Walk around a boar: back/side frames show. Get hit during a CUT! fade: both show.",
  "Correct view per angle; flash never cancels another fade.", "9", 1.5, "In progress"),
]
APPROVED = {1: "Approved", 2: "Approved", 3: "Approved", 4: "Approved", 5: "Approved", 6: "Approved", 7: "Approved", 8: "Approved", 9: "Approved", 10: "Approved", 11: "Approved", 13: "Approved", 14: "Approved", 15: "Approved", 17: "Approved", 18: "Approved"}   # approvals the user gave in chat (2026-11-13)
r = HR + 1
for row in rows:
    done = row[10].startswith("Done")
    for j, v in enumerate(row, 1):
        if j == 10:
            plan.set(r, j, v, S_NUM_G if done else S_NUM)
        else:
            plan.set(r, j, v, S_BODY_G if done else S_BODY)
    plan.set(r, 12, APPROVED.get(row[0]), S_APPR)
    r += 1
last = r - 1
plan.set(r, 9, "Total hours", S_BOLD)
plan.set(r, 10, "=SUM(J%d:J%d)" % (HR + 1, last), S_BOLDNUM)
plan.set(r + 1, 9, "Approved", S_BOLD)
plan.set(r + 1, 10, '=COUNTIF(L%d:L%d,"Approved")&" of "&COUNTA(A%d:A%d)' % (HR + 1, last, HR + 1, last), S_BOLD)
plan.set(r + 2, 9, "Note", S_BOLD)
plan.set(r + 2, 10, "Hours are rough jam-pace estimates by Claude, not measured. Deadline: Sun Nov 15, 5:59 PM PST.", S_ITAL)
plan.dv.append(("L%d:L%d" % (HR + 1, last), "Approved,Change it,Skip"))
plan.dv.append(("K%d:K%d" % (HR + 1, last), "Not started,Waiting art,Needs owner,In progress,Testing,Done (review),Done,Removed"))
plan.freeze = (HR + 1, 3)
lg = r + 5
plan.set(lg, 1, "Legend", S_BOLD)
plan.set(lg + 1, 2, "Yellow cell", S_LEG_Y); plan.set(lg + 1, 4, "Yours to fill: Approved / Change it / Skip, or type notes.")
plan.set(lg + 2, 2, "Green row", S_LEG_G); plan.set(lg + 2, 4, "Built and waiting for your review.")
plan.set(lg + 3, 2, "Blue number", S_BLUE); plan.set(lg + 3, 4, "Estimate in hours; change it and the total updates.")

# ---------------------------------------------------------------- Polish backlog sheet
# Everything left out of each system on purpose, to polish once all systems work.
BACKLOG = [
 (0, "Test map", "Art", "Greybox only (flat colors, no props)", "Test-only map; fine as is unless we demo from it", "Low"),
 (0, "Test map", "UX", "Dummies show a placeholder 'Text' label in the editor before Play", "Cosmetic, editor only", "Low"),
 (1, "Health component", "Audio", "No hurt / death sounds", "Hook OnHealthChanged / OnDied", "Medium"),
 (1, "Health component", "VFX", "No hit effect on the target (ink blood splat, sparks)", "Hook OnHealthChanged; ink style to match the look", "High"),
 (1, "Health component", "Art", "Training dummy is a plain cylinder with floating text", "Test prop only", "Low"),
 (2, "Player HP + hit feedback", "Code", "Done 2026-11-14 (system 20b): hit flash + death tint are a UMG overlay, no longer the camera fade, so Director fades are never replaced", "", "Low"),
 (2, "Player HP + hit feedback", "Audio", "No player hurt grunt, heartbeat or death sound", "", "Medium"),
 (2, "Player HP + hit feedback", "VFX", "No low-HP warning (red vignette / ink cracks at the screen edge)", "", "Medium"),
 (2, "Player HP + hit feedback", "Animation", "Death shows the template third-person ragdoll; no first-person death camera", "Comedy beat opportunity (Director: 'cut!')", "Medium"),
 (2, "Player HP + hit feedback", "UI", "Template ammo/HP widget still on screen; real HP bar is the HUD team's", "Coordinate with HUD package", "Low"),
 (3, "Sword", "Art", "Done 2026-11-13: 2D hand + sword frames from the designer video replace the 3D sword in first person", "", "Low"),
 (3, "Sword", "Animation", "Done 2026-11-13: idle / wind-up / slash smear / follow-through frames with the video's timing, plus walk bob", "", "Low"),
 (3, "Sword", "Texture", "3D Meshy sword is no longer seen in first person (2D frames now); its texture issue only matters for a world pickup", "Keep for a pickup / third person, or drop", "Low"),
 (3, "Sword", "Audio", "No whoosh / flesh-hit / miss sounds", "Hook OnSwing / OnHit", "High"),
 (3, "Sword", "VFX", "No slash trail or impact effect", "", "Medium"),
 (3, "Sword", "UI", "Done 2026-11-13: template ammo counter hides for melee weapons; the test map still shows the template team score (0 | 0) because it uses BP_ShooterGameMode", "Real game uses BP_DCGameMode (no team score)", "Low"),
 (3, "Sword", "Code", "No third-person sword mesh (only the first-person view shows it)", "Only matters for mirrors / other players", "Low"),
 (3, "Sword", "Code", "Tuned hold/swing values live in BP_DC_Sword; C++ defaults updated but not yet compiled", "Compiles with the next build", "Low"),
 (3, "Sword", "Design", "Template rifle pickups still exist in the project", "Decide if the shotgun swap (Director beat 3) reuses them", "Low"),
 (4, "Boar: body + wander", "Art", "Boar is two brown boxes (body + head)", "Camera-facing boar sprite in system 9; ink style", "High"),
 (4, "Boar: body + wander", "Animation", "Done 2026-11-13: idle (blink) + 3-frame run cycle from the designer's frames, run speed follows walking speed", "No sniffing / grazing frames", "Low"),
 (4, "Boar: body + wander", "Audio", "No grunts, snorts or footsteps", "Hook OnStateChanged", "Medium"),
 (4, "Boar: body + wander", "Design", "Wander is purely random; no grazing spots, no herds, calm boars ignore the player", "Could add points of interest (food, mud) later", "Low"),
 (4, "Boar: body + wander", "Code", "Done 2026-11-13 (system 13): floating state label is off by default and on BP_DC_Boar", "Tick Show Debug State on a boar to see its AI state", "Low"),
 (4, "Boar: body + wander", "Integration", "Main map needs a wander area (a TriggerBox) so boars stay in the bowl; spawner (system 7) will pass it", "Main map is the teammate's: ask before adding", "Medium"),
 (5, "Boar: angry, charge, bite", "Animation", "Anger is a hop, the bite is a forward lunge; no snort, run or bite animation", "Sprite frames (system 9) for angry / charge / bite", "High"),
 (5, "Boar: angry, charge, bite", "Audio", "No angry snort, charge thunder or bite chomp", "Hook OnStateChanged (Angry / Charge / Bite)", "High"),
 (5, "Boar: angry, charge, bite", "VFX", "No dust trail while charging, no bite impact effect", "", "Medium"),
 (5, "Boar: angry, charge, bite", "Balance", "Numbers are first guesses: boar 60 HP (2 sword hits), bite 40 vs player 500 HP, charge 650", "Tune once spawner + quest exist; player HP 500 is the template value", "Medium"),
 (5, "Boar: angry, charge, bite", "Design", "Only the boar you hit gets angry; nearby boars don't join in", "Group aggro would make the hunt harder/funnier", "Low"),
 (5, "Boar: angry, charge, bite", "Feel", "Being bitten doesn't push the player back", "Small knockback + the existing red flash", "Low"),
 (6, "Boar: flee + death", "Animation", "Death is the box rolling onto its side; flee is just running faster", "Death / panic sprite frames (system 9)", "High"),
 (6, "Boar: flee + death", "Audio", "No panic squeal when fleeing, no death squeal", "Hook OnStateChanged (Flee) / OnBoarDied", "High"),
 (6, "Boar: flee + death", "VFX", "No ink-blood splat on death; the body pops out of existence after 4 s", "Fade/sink or a comic 'poof' before removal", "Medium"),
 (6, "Boar: flee + death", "Design", "Nothing drops on death (no meat); fleeing boars don't heal", "Meat drop is the Director's joke in beat 2 (quest team); decide if any drop exists", "Low"),
 (6, "Boar: flee + death", "Code", "Body rotates around its centre when it falls, can clip into the ground a little", "Fine for a sprite later", "Low"),
 (6, "Boar: flee + death", "Integration", "Quest/HUD team must bind DCBoarSubsystem.OnBoarKilled to count kills", "Covered in the handoff doc (system 10)", "High"),
 (7, "Boar spawner", "VFX", "Boars pop into existence with no entrance (no dust puff, no running in from the trees)", "A 'stagehand pushes a boar on set' gag would fit the film theme", "Medium"),
 (7, "Boar spawner", "Code", "'Not near the player' is distance only (8 m); a boar can still appear in view", "Add a line-of-sight / camera-frustum check", "Low"),
 (7, "Boar spawner", "Integration", "Main map has the 6 BoarSpawn points but no spawner or wander area yet", "Teammate's map: ask, then place one spawner + one TriggerBox", "High"),
 (7, "Boar spawner", "Design", "Spawns one by one at random free points; no waves or difficulty ramp", "Quest can set Total To Spawn (e.g. 10) and call Start/Stop", "Low"),
 (8, "Boar HP bar", "Art", "Plain flat red/black strips; not in the inked comic style", "Hand-drawn frame texture or ink outline to match the look", "Medium"),
 (8, "Boar HP bar", "UI", "Fixed world size (90 cm): gets small past ~15 m; no fade in/out", "Scale with distance or fade; optional", "Low"),
 (8, "Boar HP bar", "UX", "Debug state label sits right above the bar and crowds it", "Goes away when bShowDebugState is turned off", "Low"),
 (9, "Camera-facing sprite", "Art", "Boar now uses the designer's run_01 (front view) from every angle; no side or back view drawn yet", "Set Side / Back textures on BP_DC_Boar's Sprite when drawn; no code change", "Medium"),
 (9, "Camera-facing sprite", "Animation", "No dedicated bite frame (bite reuses the charge frames); no back view, so a fleeing boar still shows its face", "Ask the artist for a bite frame and a back/side run", "Medium"),
 (9, "Camera-facing sprite", "Design", "The 'Sleep' frames (fall_02, lying) are set up but no boar ever sleeps", "Add a sleeping boar that wakes up angry when hit or when the player gets close", "Medium"),
 (9, "Camera-facing sprite", "Look", "In game the boar art reads lighter/pinker than the PNG (unlit + auto exposure)", "Lower the sprite Brightness (~0.8) or use a lit material", "Low"),
 (9, "Camera-facing sprite", "Tech", "Frames are 1042 px, imported uncompressed (UI group): ~4 MB each in memory", "Downsize to 512 px or switch to default compression before shipping", "Low"),
 (5, "Boar: angry, charge, bite", "Balance", "Team plan numbers differ from ours: player 100 HP, sword 25 dmg, boar bite 10 dmg / 1.2 s, max 5 boars, flee under 25%", "Ours: player 500 (template), sword 35, bite 40 / 1.0 s, spawner 4, flee 35%. Align with the lead", "High"),
 (9, "Camera-facing sprite", "Lighting", "Sprites are unlit: they don't darken in the storm or turn red in the boss beat", "Lit two-sided version, or tint per beat from the Director's lighting sublevels", "Medium"),
 (9, "Camera-facing sprite", "Code", "Three views (front / side / back); no 8-direction sprites", "Enough for boars; 8 directions only if the art exists", "Low"),
 (9, "Camera-facing sprite", "Art", "Sprite gallery actors in the test map (DCSpriteActor) still show the placeholder boar drawing", "Test map only; point them at T_Boar_* if useful", "Low"),
 (10, "Integration", "Integration", "Boars spawn during the Director's ~34 s intro and kills before the quest starts don't count", "Lead: spawner Auto Start off + SetActive(true) in ADCGameMode::StartHuntQuest (handoff doc section 1)", "High"),
 (10, "Integration", "Balance", "Sword 35 vs boar 60 HP = 2 hits, so fleeing is almost never seen", "With the plan's 25 damage the 2nd hit leaves 17% and the boar flees; align numbers", "High"),
 (10, "Integration", "Integration", "Main map has no spawner / wander area yet; BP_DCBoarSpawner (max 5) is ready for the lead to place", "Handoff doc section 1", "High"),
 (10, "Integration", "UX", "Standing right on top of a boar, its card fills the bottom of the screen", "Expected for 2.5D; could push the player back on bite", "Low"),
 (12, "2D sword view", "Art", "Done 2026-11-13: designer PNGs (1920x1080, transparent) replace the frames cut from the video", "Source PSD + GIF in Art/Player/Sword/Reference", "Low"),
 (12, "2D sword view", "Animation", "Only 4 poses (idle, wind-up, smear, follow-through); no in-betweens, no impact frame, one swing only", "More frames / a second swing that alternates", "Medium"),
 (12, "2D sword view", "Feel", "Designer timing (12 fps: windup 3 ticks, smear 1, follow 3): the hit lands 250 ms after the click, next attack after 583 ms", "Change Windup Ticks / Anim FPS on BP_DC_Sword if it feels slow", "Medium"),
 (12, "2D sword view", "Integration", "Teammate HUD (WBP_DCHUD, main 6098a67) not yet merged into design-yair; weapon layer sits under the HUD (Z -10) but overlap not checked", "Pull main and check the health bar / subtitles vs the hand", "Medium"),
 (12, "2D sword view", "Repo", "Hand_Sword.psd and Dream 14.gif are stored in plain git (.gitattributes has no LFS rule for psd/gif)", "Add *.psd / *.gif LFS rules if more source art is coming (shared file: ask the lead)", "Low"),
 (12, "2D sword view", "Design", "Other weapons (shotgun beat 3, PG tool) need their own hand frames", "Same widget, new frame set per weapon", "Medium"),
 (9, "Camera-facing sprite", "Art", "Same component can draw the missing sword hand (system 3) and the Director", "Needs the art", "Medium"),
 (14, "Balance + weapon rules", "Design", "King Boar 900 HP and Director 1500 HP are worked out from rifle damage, not from a real run", "Full Level 1 playtest in the main map (PIE, don't save); tune in DCKingBoar.cpp / DCDirectorBoss.cpp", "High"),
 (14, "Balance + weapon rules", "Design", "A weapon pickup touched during a sword-only beat is used up but gives nothing", "Only matters if the main map keeps template rifle pickups in Beats 1-2", "Low"),
(15, "King Boar visuals", "Art", "Gold is a tint over the normal (red-brown) boar frames, so it reads orange-gold; the crown is a placeholder drawing", "Designer: a gold King Boar frame set and a crown; swap on Sprite / CrownSprite", "Medium"), (15, "King Boar visuals", "Animation", "Crown is fixed to the head position of the idle/charge frames; it doesn't follow hit/hurt frames", "Fine at 2.5x scale; real art could draw the crown into the frames", "Low"),
(18, "Audio + VFX hooks", "Audio", "All 13 S_* sounds are synthesized placeholders (beeps, noise, buzz)", "System 19: reimport real files over the same S_* assets", "High"), (18, "Audio + VFX hooks", "VFX", "Done 2026-11-14: hand-drawn sprite effects (hit splat, boar poof, gold boss poof) via ADCSpriteFX; frames are placeholder drawings; no sword trail", "Artist can redraw T_FX_* frames (same names)", "Low"), (18, "Audio + VFX hooks", "Repo", "Done 2026-11-13: /Game/DirectorsCut/Audio added to Directories to Always Cook (Config/DefaultGame.ini), so the sounds ship in packaged builds", "Check once in the first packaged build", "Low"), (18, "Audio + VFX hooks", "Audio", "The killing blow on the boss also plays a hit sound under the defeat sound", "Cosmetic", "Low"),
 (17, "Set dressing: butcher stall", "Art", "Beat 2 meat hooks and the empty plate are still greybox; nothing is on the counter (no meat, cleaver, scales)", "Same method as the stall: Blender + a Meshy picture", "Medium"),
 (17, "Set dressing: butcher stall", "Art", "SOLD OUT letters look grey when the awning shadow falls on the board", "Turn off shadow casting on the awning edge or brighten the sign material", "Low"),
 (17, "Set dressing: butcher stall", "Audio", "No laugh track or sting when the HA HA HA sign appears in beat 2", "Hook the beat 2 sublevel load", "Medium"),
 (19, "2D shotgun view", "Art", "No reload drawing: the idle frame stays on screen during the 1.5 s reload", "One or two reload frames from the designer; same tick grid", "Medium"),
 (19, "2D shotgun view", "Code", "Pellets leave from the hidden 3D gun muzzle, not from the drawn barrel", "Fire from the camera centre while the 2D view is on", "Low"),
 (19, "2D shotgun view", "Audio", "Blast is still the template grenade launcher sound", "Use one of the fal shotgun takes once picked", "Medium"),
 (17, "Set dressing: notice board", "Animation", "The quest mark over the notice board is static (no bob or spin) and never goes away", "Slow bob in a small actor class; hide it once the King Boar quest starts or ends", "Low"),
 (17, "Set dressing: notice board", "Design", "The wanted board is only a prop: walking up to it does nothing", "Optional Director line when the player first looks at it", "Low"),
 (17, "Set dressing: well + sword rack", "Design", "The three swords on the rack stay there after the player takes the sword", "Hide one rack sword when the sword pickup is collected", "Low"),
 (17, "Set dressing: well + sword rack", "Code", "The well roof and posts have no collision; only the stone ring blocks", "Fine for a prop; add two thin blockers if players get stuck visually", "Low"),
 (17, "Set dressing: film cameras + boom mics", "Art", "The cameras and mics keep Meshy's realistic colour picture; not redrawn in the inked comic style", "Repaint or run the colour picture through an ink pass; add an outline shell like the sword", "Medium"),
 (17, "Set dressing: film cameras + boom mics", "Animation", "Film reels do not turn and cameras never pan to follow the player", "Split the reels in Blender and spin them; optional slow pan", "Low"),
 (17, "Set dressing: film cameras + boom mics", "Art", "Woodland slide and brick wall picture are imported but not used in the level", "Slide needs about 3x scale to replace the watchtower; wall picture could cover the boundary walls", "Low"),
 (20, "Main menu + pause menu", "Audio", "No hover or click sound on START / RESUME / QUIT and no menu music", "UDCFXLibrary::PlayUISound in the button handlers; start a music track in ADCMainMenuGameMode", "Medium"),
 (20, "Main menu + pause menu", "Design", "The main menu has no Quit, and the pause menu Quit closes the game instead of returning to the menu", "Decide the flow; both are one-line changes", "Low"),
 (20, "Main menu + pause menu", "Art", "The main menu background is a flat dark colour", "Key art, or the level seen from a camera behind the logo", "Medium"),
 (20, "Main menu + pause menu", "Code", "Not tested with real key presses or in a packaged build; in the editor Esc stops Play so P is the stand-in", "Test in Standalone / a packaged build", "High"),
]
pol = Sheet("Polish backlog")
pol.set(1, 1, "Polish backlog: what each system still lacks (art, animation, textures, audio, VFX, UI)", S_TITLE)
pol.set(2, 1, "Filled in as each system is finished. Work through it after all systems are approved.", S_ITAL)
pheads = ["#", "System", "Area", "What's missing", "Note", "Priority", "Status"]
for j, (h, w) in enumerate(zip(pheads, [5, 24, 12, 60, 50, 10, 12]), 1):
    pol.set(4, j, h, S_HDR)
    pol.widths[j] = w
for i, row in enumerate(BACKLOG, 5):
    for j, v in enumerate(row, 1):
        pol.set(i, j, v, S_BODY)
    pol.set(i, 7, "Open", S_APPR)
pl = 4 + len(BACKLOG)
pol.dv.append(("F5:F%d" % pl, "High,Medium,Low"))
pol.dv.append(("G5:G%d" % pl, "Open,In progress,Done,Won't do"))
pol.freeze = (5, 3)
pol.set(pl + 2, 3, "Open items", S_BOLD)
pol.set(pl + 2, 4, '=COUNTIF(G5:G%d,"Open")' % pl, S_BOLD)
pol.set(pl + 3, 3, "High priority open", S_BOLD)
pol.set(pl + 3, 4, '=COUNTIFS(F5:F%d,"High",G5:G%d,"Open")' % (pl, pl), S_BOLD)

# ---------------------------------------------------------------- Rules sheet
rules = Sheet("Rules")
rules.set(1, 1, "Working rules (so we do not overlap the rest of the team)", S_H2)
for i, (k, v) in enumerate([
 ("Our systems", "Sword and player HP, boar AI with health and a spawner, sprites that face the camera."),
 ("Our code folders", "Source/EpicGameJamGame/DirectorsCut/Combat, /AI, /Rendering. We also own Variant_Shooter/ and build on top of it."),
 ("Our content folders", "/Game/DirectorsCut/Combat, /AI, /Rendering, /TestMaps."),
 ("Our map", "Only Lvl_Test_CombatAI. We never save Lvl_DirectorsCut or any DC_ sublevel."),
 ("Shared things", "Ask the teammate before changing anything outside the folders above (project settings, Build.cs, main map, other packages)."),
 ("Branch", "design-yair. Merge to main through a pull request."),
 ("Order", "One system at a time: build, test in the test map, show you, wait for approval, then the next."),
 ("Hooks for other packages", "We fire events (player HP changed / died, boar killed) and document them; the Quest/HUD and Director packages connect to them."),
], 3):
    rules.set(i, 1, k, S_BOLD)
    rules.set(i, 2, v, S_BODY)
rules.widths = {1: 24, 2: 100}

# ---------------------------------------------------------------- Test map sheet
tm = Sheet("Test map")
tm.set(1, 1, "Lvl_Test_CombatAI (/Game/DirectorsCut/TestMaps). Spawn on the cyan pad facing the zones", S_H2)
for j, h in enumerate(["Zone", "For system", "What's there", "Tags placed"], 1):
    tm.set(3, j, h, S_HDR)
for i, z in enumerate([
 ("A: Sword + player HP (north lane)", "2, 3", "Yellow firing line, white 1 m range ticks out to 8 m, 3 orange dummies, red damage pad", "TestDummy, TestDamagePad"),
 ("B: Boar arena (centre, fenced)", "4-8", "Waist-high fence with a gate on the spawn side, 2 rocks and a stump to path around, 6 yellow spawn marks, nav mesh", "BoarSpawn (x6, same tag as the main map)"),
 ("F: Sprite gallery (south lane)", "9", "Centre slot with a yellow walk-around ring (6 m radius), 3 slots further out, the last one raised 2.5 m", "SpriteSlot"),
 ("Hub", "all", "Cyan spawn pad, PlayerStart, sun/sky/fog, Shooter game mode override, Kill Z -1000", "PlayerStart: RED, BLUE, Player0"),
], 4):
    for j, v in enumerate(z, 1):
        tm.set(i, j, v, S_BODY)
tm.set(9, 1, "Screenshot: Docs/Screenshots/Test_CombatAI_overview.png. Rebuild: Scripts/Test_BuildCombatAIMap.py, build_test_map()", S_ITAL)
tm.widths = {1: 32, 2: 12, 3: 70, 4: 34}

# ---------------------------------------------------------------- package
sheets = [plan, pol, rules, tm]
with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("[Content_Types].xml",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="xml" ContentType="application/xml"/>'
        '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
        '<Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>'
        + "".join('<Override PartName="/xl/worksheets/sheet%d.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>' % (i + 1) for i in range(len(sheets)))
        + "</Types>")
    z.writestr("_rels/.rels",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
        "</Relationships>")
    z.writestr("xl/workbook.xml",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
        "<sheets>" + "".join('<sheet name="%s" sheetId="%d" r:id="rId%d"/>' % (escape(s.name), i + 1, i + 1) for i, s in enumerate(sheets)) + "</sheets>"
        '<calcPr calcId="191029" fullCalcOnLoad="1"/></workbook>')
    z.writestr("xl/_rels/workbook.xml.rels",
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        + "".join('<Relationship Id="rId%d" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet%d.xml"/>' % (i + 1, i + 1) for i in range(len(sheets)))
        + '<Relationship Id="rId%d" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>' % (len(sheets) + 1)
        + "</Relationships>")
    z.writestr("xl/styles.xml", STYLES)
    for i, s in enumerate(sheets):
        z.writestr("xl/worksheets/sheet%d.xml" % (i + 1), s.xml())
print(OUT)
