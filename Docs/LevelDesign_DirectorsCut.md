# Director's Cut: Single-Map Level Design (Lvl_DirectorsCut)

**Map:** `/Game/DirectorsCut/Maps/Lvl_DirectorsCut`
**Builder:** `Scripts/LD_BuildDirectorsCut.py`. It is re-runnable; each piece clears its own `DC/<Piece>` outliner folder.

The older linear greybox, `Lvl_VerticalSlice`, is untouched and kept for reference.

## Concept

The whole game happens in **one valley that the player never leaves**. The Director doesn't move the player to a new area. He **rewrites the set around them**.

Each script beat is a different "environment state" of the same space. Each state is built as a **streaming sublevel**, so the programmer can swap them with the standard Blueprint nodes `Load Stream Level` and `Unload Stream Level`. No custom code is needed for the environment changes.

Units are cm, and +X points toward the Director's Tower. The playable bowl is about 120 × 100 m, from (-6000,-5000) to (6000,5000).

```
        N (+Y)   cliffs + forest edge          Exit C (1500,4880)
   +---------------------------------------------------+
   | butcher stall         watchtower (2800,3600)      |  painted
   | (-2500,2800)                                       |  BACKDROP
W  | VILLAGE   ======  ( THE SET )  ======  Exit A ->   |  x=6000   scaffolding  TOWER
   | spawn             plinth r12m                      |  (falls   x 6300-6900  (11000,0)
   | (-5200,0)         (1000,0)        boar pen (PG)    |  beat 6)
   | windmill                          (3000,-3200)     |
   +---------------------------------------------------+
        S (-Y)            Exit B (-1000,-4880)
```

## Always present (persistent level)

These stay the same in every beat:

- **Layout:** ground, dirt paths, the village (spawn), the butcher stall, the watchtower, the central **Set** plinth, boar burrows, and the boundary cliffs with a forest edge.
- **Backstage:** scaffolding at x 6300–6900. It's hidden behind the painted backdrop until beat 6.
- **Landmark:** the Director's Tower (giant camera on a 70 m shaft, red tally light). It's visible from spawn, above the backdrop.
- **Sky:** SkyAtmosphere and SkyLight (real-time capture). Sun, fog and post-process live in the light sublevels.
- **Gameplay:**
  - `PlayerStart_Village` at (-5200,0), facing +X, tagged `RED` and `BLUE` to match the Shooter respawn lookup.
  - 6 × `BoarSpawn` target points, `NavMeshBounds_Bowl`, Kill Z -1500, and the GameMode override `BP_DCGameMode` (pawn `BP_ShooterCharacter` until `BP_DCCharacter` lands).
  - `Narrator_Focus_TheSet` (tag `NarratorFocus`): a point at The Set for camera or voice focus if needed.

## Beat → sublevel table (the contract for the programmer)

Load exactly these sublevels for each beat and unload everything else. Use streaming level names; all of them live in `/Game/DirectorsCut/Maps/`.

| Beat | Narrator moment | Load these | What the player sees change |
|---|---|---|---|
| 1 | Intro quest: kill 10 boars (sword; interrupted at 7/10) | `DC_Light_Day`, `DC_Set_Hunt` | Sunny fantasy meadow, painted sky backdrop, hay bales, sword rack |
| 2 | Breakfast / meat-drop joke: collect 5 boar meat | `DC_Light_Day`, `DC_Set_Hunt`, `DC_Set_NoMeat` | Stall gets "SOLD OUT", empty meat hooks and a spotlit empty plate; a giant red "HA HA HA" sign; a blank scoreboard next to The Set |
| 3 | Director cut #1: too boring! Sword → gun, hyperspawn, kill 100 boars | `DC_Light_Day`, `DC_Set_Hunt`, `DC_Set_FilmCrew` | Film crew takes over The Set: spotlit weapon pedestal, 4 light stands, director's chair, dolly track and camera, boom mic, "SCENE 3 TAKE 2" clapperboard |
| 4 | Director cut #2: too violent, need romance! Rescue the Princess from the shed guarded by the King Boar | `DC_Light_PG`, `DC_Set_Hunt`, `DC_Set_PG` | Pastel pink light, cardboard flowers, rainbow arch over the main path, romantic shed holding the Princess, guarded by the giant crowned King Boar |
| 5 | Climax / Boss: the Director gets mad, enters the scene ("bad actor"), fight the Director | `DC_Light_Boss`, `DC_Set_Boss` | Red light. The Set becomes an arena: a curved red wall with 4 entrances, 4 tall and 8 low cover blocks, 3 red spotlight rigs, and the Director's crane perch east of The Set with a "FINAL CUT" sign |

`DC_Light_Day` and `DC_Set_Hunt` are set to **Initially Loaded + Visible**, so beat 1 is live as soon as the map opens. All other sublevels start unloaded.

**Rule:** only one `DC_Light_*` sublevel may be loaded at a time. Each one has its own Sun, Fog and unbound PostProcess.

## Gameplay markers inside the sublevels

These are placed by level design / spawned by `ADCGameMode` for the full 5-beat flow.

| Actor | Sublevel / Owner | Tags | Use |
|---|---|---|---|
| `WeaponPickup_Sword` | Set_Hunt | `WeaponPickup`, `Sword` | Beat 1 sword spawn, by the village sword rack |
| `WeaponPickup_Shotgun` | Set_FilmCrew | `WeaponPickup`, `Shotgun` | Beat 3 weapon pedestal on The Set (player is auto-swapped to gun) |
| `DCPrincess` + `DCKingBoar` | Beat 4 (`Set_PG` or persistent) | `PrincessSpawn`, `KingBoarSpawn` | Beat 4: Place an `ADCPrincess` directly or a `TargetPoint` tagged `PrincessSpawn` (e.g. inside a shed or tower), and optionally `KingBoarSpawn`. If omitted, `ADCGameMode` spawns them at the default Beat 4 clearing. |
| `Boss_DirectorSpawn` | Set_Boss | `Boss` | Beat 5: Director boss spawns in front of the crane perch on The Set arena |
| `BoarSpawn_0..5` | persistent | `BoarSpawn` | Boar spawn points around the edge, facing The Set |
| `DC_BoarSpawner` | persistent | — | Normal spawning in Beats 1–2 (`MaxAlive=5`), hyperspawn in Beat 3 (`MaxAlive=12`, auto-aggro) |

Editor-only text labels (`LBL_*`) are hidden in game. The `SIGN_*` text actors are real in-game signage.

**No instruction signs.** Objectives and instructions come only from the Director (subtitles/voice) and the HUD quest tracker. The remaining `SIGN_*` actors are set-dressing jokes the Director "built": "SOLD OUT", "HA HA HA", "DIRECTOR", "SCENE 3 TAKE 2", "WIND FX", "EXIT", "ERROR: SCENE NOT FOUND", "LOADING..." and "FINAL CUT".

## Level design notes

- **One weenie:** the Tower is visible from spawn above the backdrop. It stays the "ending" the whole game. In beat 5, the Director attacks from in front of it.
- **The Set is the stage for every rule change:** the weapon swap (beat 3) and the boss arena (beat 5) both happen on the same central plinth. The player learns that's where the Director acts.
- **Each beat changes mood and rules:**
  - Beat 2 mocks the meat quest at the butcher stall.
  - Beat 3 brings in the film crew, swaps the sword for a gun, and triggers boar hyperspawn.
  - Beat 4 turns the world pastel pink for the romance shed & King Boar fight.
  - Beat 5 closes The Set behind a red boss arena wall with 4 entrances and cover blocks.
- **Lighting carries the mood:** sunny day (beats 1–3), then pastel PG romance (beat 4), then boss red (beat 5).

## Previewing beats in the editor

- **Levels panel:** use **Window > Levels** and toggle sublevel visibility (the eye icon).
- **Python:**
  ```python
  exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\LD_BuildDirectorsCut.py").read())
  show_beat(5)
  ```
  This shows exactly beat 5's set. `show_beat(1)` returns to the start state.

## Open items

- Your programmer needs to drive the beat changes: Narrator beat event → Load/Unload Stream Level per the table above.
- The checkpoint system written for `Lvl_VerticalSlice` isn't used in this map. It has no `Checkpoint` actors, so respawn falls back to `PlayerStart_Village`.
- Meshy art pass: swap the greybox for generated props per set. Hero pieces: the Tower camera, the butcher stall, the director's chair, the wind machine and the boss crane.

## Screenshots
Same elevated camera for every beat: `Docs/Screenshots/DC_beat1.png` … `DC_beat7.png`, plus `DC_beat1_spawn.png` (the spawn view) and `DC_beat7_player.png` (approaching the boss arena).
