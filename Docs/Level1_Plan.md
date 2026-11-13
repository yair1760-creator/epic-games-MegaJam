# Level 1 ("Intro Quest: Hunt the Boars") Task Board

**Map:** `/Game/DirectorsCut/Maps/Lvl_DirectorsCut`, beat 1 (`DC_Light_Day` + `DC_Set_Hunt`, loaded by default)
**GDD stage 1:** *"Wild boars entered the perimeter! Take up arms and eliminate them!"* The player spawns in the village with a sword, and basic boar AI attacks.
**Level 1 is done when:** the player spawns with a sword, the Director gives the quest by subtitle, the HUD shows HP and "Boars killed: x/10", boars fight back when hit, and at 10/10 the Director says "CUT!" and the game moves on to beat 2.

**Design rule: no signs.** Every instruction and objective comes from the Director's voice (subtitles for now, voiceover later) and the quest tracker on the HUD. Nothing in the world tells the player what to do.

---

## Two-developer split (start here)

| | **Dev 1: Lead (systems, flow, UI logic)** | **Dev 2: Gameplay (combat, boars, sprites)** |
|---|---|---|
| Packages | E (game flow, map, signs) + C (Director system) + D (quest + HUD) | A (sword + player) + B (boar) + F (sprite billboarding) |
| C++ folders | `DirectorsCut/Quest/`, `DirectorsCut/Narrator/`, `DirectorsCut/Beats/`, `DirectorsCut/UI/` | `DirectorsCut/Combat/`, `DirectorsCut/AI/`, `DirectorsCut/Rendering/` |
| Content folders | `/Game/DirectorsCut/Blueprints/` (GameMode, PlayerController), `/Game/DirectorsCut/Narrator/`, `/Game/DirectorsCut/UI/` (logic only; Designer 1 does the visuals) | `/Game/DirectorsCut/Weapons/`, `/Game/DirectorsCut/Characters/`, `/Game/DirectorsCut/Blueprints/BP_DCCharacter` |
| Maps | `Lvl_DirectorsCut.umap` (persistent) and every `DC_*` sublevel until Designer 2 takes over `DC_Set_Hunt` / `DC_Light_Day` | Only test maps in `/Game/Developers/<name>/`. Never saves `Lvl_DirectorsCut` or a `DC_*` sublevel. |
| Shared files | Sole owner of `EpicGameJamGame.Build.cs`, `EpicGameJamGame.uproject`, `Config/*.ini`, `Scripts/*` | Asks Dev 1 for any change to those files. |
| Template code | Doesn't edit `Variant_Shooter/` | Sole owner of `Variant_Shooter/` (subclass it; edit it only if a function must become `virtual`). |

### Step 0: Dev 1, first 30 minutes (before Dev 2 needs anything)
One commit that sets up everything shared, so neither of you edits these files again:
1. Enable the **Paper2D** plugin in `EpicGameJamGame.uproject`. Do **not** commit the local `EngineAssociation` change (the editor rewrites it to your machine's engine ID); commit only the plugin line.
2. In `EpicGameJamGame.Build.cs`, add `"Paper2D"` and `"Niagara"` to the dependencies and `"EpicGameJamGame/DirectorsCut"` to the include paths.
3. Push **stub headers** for Dev 1's contracts, so Dev 2 can call them right away:
   - `DirectorsCut/Quest/DCQuestSubsystem.h/.cpp`, with `ReportEvent(FName)` doing nothing yet.
   - `DirectorsCut/Narrator/DCNarratorSubsystem.h/.cpp`, with `PlayLine(FName)` doing nothing yet.
4. Set the default game and editor map to `Lvl_DirectorsCut`.

Meanwhile, Dev 2 starts on `ADCMeleeWeapon` in a test map. It needs nothing from Dev 1.

### Step 1: in parallel (rest of Friday)
| Dev 1 | Dev 2 |
|---|---|
| Remove the instruction signs (script + map) | `ADCMeleeWeapon` + `BP_Weapon_Sword` (placeholder `SM_Sword`) |
| `BP_DCGameMode` + `BP_DCPlayerController` in the map. Use `BP_ShooterCharacter` as the pawn until Dev 2 pushes `BP_DCCharacter`. | `UDCHealthComponent` + `ADCBoar` (placeholder quad) |
| `UDCNarratorSubsystem` + `DT_NarratorLines` + `WBP_Subtitles` logic | `ADCBoarAIController` + `ST_Boar` (wander → aggro → charge → attack → dead) |
| `UDCQuestSubsystem` + `WBP_QuestTracker` + `WBP_HealthBar` + `WBP_DCHUD` | `BP_DCCharacter` with the sword as the starting weapon, MaxHP 100. **Tell Dev 1 when it's pushed.** |
| `GoToBeat(int)` | `ADCBoarSpawner` (`bActive` flag), boar death → `ReportEvent("Kill.Boar")` |

### Step 2: handoffs (Friday evening)
- **Dev 2 → Dev 1:** the names `BP_DCCharacter` and `BP_DCBoarSpawner`. Dev 1 sets the pawn in the GameMode and places the spawners on `BoarSpawn_0..5` in the persistent map. Dev 2 doesn't place them.
- **Dev 1 → Dev 2:** confirm `ReportEvent` is live, then play the full loop together.
- After that: Dev 2 does hit feedback, the boar HP bar and the billboard sprite component. Dev 1 does the Level 1 flow (intro → quest → reactions → CUT! → beat 2) and the death/respawn rule.

### Rules of thumb for the two of you
- Both of you add C++ to the same module. That's fine, because text files merge, but **pull before you build** and **push only code that compiles**.
- New classes go only into your own folders. If you need a new hook in the other person's class, ask; don't add it yourself.
- Blueprints you need from the other person: reference them by the agreed name, and use your own temporary stand-in until theirs lands.

---

## How the packages stay independent

Each package has **one owner** and **its own files**. Packages talk to each other only through the small contracts below, so nobody waits on anybody. Until the real version of a contract exists, use its stub.

| Contract | Owner | Exact API (push the stub header in the first 30 min) |
|---|---|---|
| Damage | engine, built in | `UGameplayStatics::ApplyDamage` → `TakeDamage` (the shooter code already uses this) |
| Quest | Dev D | `UDCQuestSubsystem` (WorldSubsystem): `StartQuest(FName Id, FText Title, int32 Target, FName EventTag)`, `ReportEvent(FName EventTag)`, `ReplaceQuest(Id, Title, Target, EventTag, bKeepProgress)`, `OnQuestUpdated(FName Id, int32 Current, int32 Target)`, `OnQuestCompleted(FName Id)` |
| Director | Dev C | `UDCNarratorSubsystem` (WorldSubsystem): `PlayLine(FName RowId)`, `PlaySequence(TArray<FName>)`, `OnLineStarted(FText Speaker, FText Text, float Duration)`, `OnLineFinished(FName RowId)`. Row struct `FDCNarratorLine { Speaker, Text, Duration, USoundBase* Voice }` |
| Boar death | Dev B | When a boar dies it calls `QuestSubsystem->ReportEvent("Kill.Boar")`. That is the only thing the boar knows about quests. |
| Beats | Dev E | `GoToBeat(int32 Beat)` loads/unloads sublevels per the table in `LevelDesign_DirectorsCut.md` |
| Art handoff | Designers | Assets are dropped into agreed paths with agreed names (see the designer packages). Programmers reference those paths, and use placeholders with the same names until the real asset arrives, so swapping is just overwriting the file. |

**File rules** (Unreal assets are binary, so Git can't merge them, and two people saving the same file loses one person's work):
- Only the owner saves a file. If you need to touch someone else's file, ask in chat first.
- Pull before opening the editor, then commit and push small and often.
- Experiment in `/Game/Developers/<yourname>/`.
- Push the C++ and the Blueprint that uses it together.

---

## Developer packages

If you have fewer developers than packages, one person takes two packages. Good pairs are **C + D** (both are UI/systems) and **A + E**.

### Dev A: Sword and player combat
Owns: `Source/.../DirectorsCut/Combat/DCMeleeWeapon.*`, `/Game/DirectorsCut/Weapons/Sword/`, `/Game/DirectorsCut/Blueprints/BP_DCCharacter`
1. `ADCMeleeWeapon` (C++), a child of the existing `AShooterWeapon`. It overrides `Fire()` to do a sphere sweep in front of the camera instead of spawning a projectile. Editable properties: `Damage` (25), `Range` (180 cm), `SweepRadius` (40 cm), `SwingCooldown` (0.5 s), `HitDelay`, `Knockback`. No ammo, no reload, one hit per target per swing.
2. `BP_Weapon_Sword` using the placeholder mesh `SM_Sword` and a simple swing (a procedural rotation is fine until Designer 1's mesh is ready).
3. `BP_DCCharacter` (child of `BP_ShooterCharacter`) with `StartingWeaponClass = BP_Weapon_Sword`. Remove the starting gun.
4. Hit feedback: a short freeze on hit, camera shake, hit sound and VFX slots (`NS_HitImpact`, `S_SwordHit`).
5. Player HP: the character already has `MaxHP`, `TakeDamage`, `OnDamaged` and respawn. Set MaxHP to 100 and add a red screen flash on hit.
6. Turn off reload and weapon switching while the player only has the sword. Make sure `AddWeaponClass` can swap in the shotgun later (Level 3 needs it).

Test on your own: put a target dummy with `TakeDamage` in your Developers map.

### Dev B: Boar enemy (health + AI + spawning)
Owns: `Source/.../DirectorsCut/AI/*`, `Source/.../DirectorsCut/Combat/DCHealthComponent.*`, `/Game/DirectorsCut/Characters/Boar/`
1. `UDCHealthComponent`: `MaxHP`, `CurrentHP`, `OnHealthChanged`, `OnDied`. The Director boss in Level 7 reuses it.
2. `ADCBoar`: a Character with the health component (60 HP) and a camera-facing sprite (from Dev F's component, or a plain quad with `T_Boar_Placeholder` until then).
3. `ADCBoarAIController` + `ST_Boar` StateTree. States: **Wander** (neutral) → **Aggro** when hit, and optionally alert nearby boars → **Charge** at the player → **Attack** (10 damage, 1.2 s cooldown) → **Flee** under 25% HP (optional, from the GDD) → **Dead** (death anim, `ReportEvent("Kill.Boar")`, destroy after 3 s). The existing `ShooterAIController` / `ST_Shooter` are a good reference.
4. `ADCBoarSpawner`: keeps up to 5 boars alive across the existing `BoarSpawn_0..5` points. It has a `bActive` flag the level flow can switch off.
5. Small HP bar over the boar that only shows after it has been hit (uses Designer 1's `WBP_BoarHPBar`).

Test on your own: your own test map with a NavMesh, a spawner and the default player.

### Dev C: Director system (subtitles now, voice later)
Owns: `Source/.../DirectorsCut/Narrator/*`, `/Game/DirectorsCut/Narrator/DT_NarratorLines`, the logic in `WBP_Subtitles`
1. `FDCNarratorLine` row struct and the `DT_NarratorLines` data table.
2. `UDCNarratorSubsystem`: a line queue, interrupting lines (the Director cuts himself off, which the GDD jokes need), timing from `Duration` or from the `Voice` sound's length, and playing the sound when one is set.
3. `WBP_Subtitles` logic: speaker name, text with a typewriter effect, portrait slot, fade in/out. Designer 1 styles it, and you own the bindings.
4. Debug console command `DC.PlayLine <RowId>` so writers and designers can test lines without playing.
5. Fill the table with the Level 1 lines from the script section below.

Test on your own: an empty map plus the console command.

### Dev D: Quest system and HUD
Owns: `Source/.../DirectorsCut/Quest/*`, the logic in `WBP_DCHUD`, `WBP_QuestTracker`, `WBP_HealthBar`
1. `UDCQuestSubsystem`: one active quest, `ReportEvent` matching against the quest's event tag, progress, completion. It must support **replacing the quest mid-progress**, which is the GDD's "Mid-Game Objective Swapping" that Level 2 needs.
2. `WBP_QuestTracker`: "Boars killed: x/10". It bumps when the number changes and plays a "complete" state, and it shows a "quest updated" flash when the Director changes the quest.
3. `WBP_HealthBar` bound to the player's `OnDamaged`. The existing `UI_ShooterBulletCounter` already gets life percent, so use it as the reference.
4. `WBP_DCHUD`: one root widget holding the tracker, HP bar, subtitles slot and crosshair, so there's a single HUD to add to the screen.

Test on your own: a debug key that calls `ReportEvent("Kill.Boar")`.

### Dev E (Lead): Game flow, map integration, signs removal
Owns: `Lvl_DirectorsCut.umap` (persistent), `BP_DCGameMode`, `BP_DCPlayerController`, `Source/.../DirectorsCut/Beats/*`, Project Settings, `Scripts/LD_BuildDirectorsCut.py`
1. **First hour, before Designer 2 starts placing art:** remove the signs (details in the signs section below).
2. Set the default game and editor map to `Lvl_DirectorsCut`. Never commit the `EngineAssociation` line in the `.uproject`; it is machine-specific.
3. `BP_DCGameMode` replaces `BP_ShooterGameMode` in the map: no team scores and no shooter NPCs. Uses `BP_DCCharacter` and `BP_DCPlayerController`, and adds `WBP_DCHUD`.
4. `GoToBeat(int)` using Load/Unload Stream Level per the beat table.
5. Level 1 flow, all from the Director:
   - On start, play `L1_Intro` lines → `StartQuest("Q_HuntBoars", "Boars killed", 10)` with event tag `Kill.Boar` → enable the boar spawner.
   - Reactions: first kill, 5/10, player below half HP, player death.
   - At 10/10: disable the spawner, play `L1_Complete`, show the "CUT!" screen, then `GoToBeat(2)`.
6. Player death: respawn at `PlayerStart_Village` and **keep quest progress** (unless the team decides otherwise).
7. Integration playtests at every milestone, plus one packaged build on Saturday.

### Dev F (optional; give it to whoever finishes first): Sprite billboarding
Owns: `Source/.../DirectorsCut/Rendering/DCBillboardSpriteComponent.*`, `M_SpriteBillboard`
1. `UDCBillboardSpriteComponent`: a flipbook that faces the camera (rotates on yaw only), with `SetAnimState(Idle, Walk, Charge, Attack, Hurt, Death)` and a hurt flash. This is GDD Phase 1, "2D sprite billboarding".
2. A masked material so sprites receive the scene lighting.
3. If you use Paper2D, enable the Paper2D plugin in `EpicGameJamGame.uproject`, and coordinate with Dev E first because Dev E owns that file.

Dev B uses a placeholder until this lands. The Director avatar in Levels 6–7 also uses it.

### Script and audio (anyone, in parallel; probably the Lead or Dev C)
Owns: `/Game/DirectorsCut/Audio/`, the text in `DT_NarratorLines`
1. The full Director script for all 7 stages. The GDD calls this the "Next Immediate Action".
2. Level 1 sound effects: `S_SwordSwing`, `S_SwordHit`, `S_BoarGrunt`, `S_BoarSqueal`, `S_BoarCharge`, `S_BoarDeath`, `S_PlayerHurt`, `S_QuestTick`, `S_QuestComplete`, `S_Cut`.
3. Music: a cheesy heroic fantasy loop for beat 1.
4. Stretch: AI voiceover clips, put into the `Voice` column.

---

## Designer packages

The two designers split by **what's on screen**, so they never touch the same files.

### Designer 1: Characters, weapon and UI
Owns: `/Game/DirectorsCut/Art/Characters/`, `/Game/DirectorsCut/Art/Weapons/`, the **visual layout and style** of every `WBP_` widget (Devs C and D own the bindings).

In priority order:
1. **P0 placeholders (first 1–2 hours):** `T_Boar_Placeholder` (one PNG of a boar facing the camera) and `SM_Sword` (blockout). These unblock Devs A and B right away.
2. **P0 boar sprite sheets:** idle, walk, charge, attack, hurt, death. A front view is enough; add a side view if there's time. It must read clearly from 20 m. Names: `T_Boar_<Anim>` (sheet) → `FB_Boar_<Anim>` (flipbook).
3. **P0 HUD style:** `WBP_HealthBar` (bottom-left), `WBP_QuestTracker` (top-right, with a boar icon), `WBP_Subtitles` (bottom-centre, with the speaker name "THE DIRECTOR" and a portrait slot), crosshair, and 2 fonts (display + body; check the licences).
4. **P0 final sword:** chunky low-poly first-person sword with the pivot at the grip. It must fit the `HandGrip_R` socket of the existing first-person arms.
5. **P1 Director portrait:** small sprite for the subtitle box (beret, megaphone, sunglasses). Design the character once, because the same Director is the boss in Level 7.
6. **P1 screens:** `WBP_CutScreen` (a clapperboard snapping shut, reused at the end of every level) and `WBP_DeathScreen` ("CUT! Take 2").
7. **P1** `WBP_BoarHPBar` (small bar shown over the boar).
8. **P2** film-style HUD frame (letterbox bars, a red "REC" dot).

### Designer 2: World, lighting and VFX
Owns: `/Game/DirectorsCut/Art/Environment/`, `/Game/DirectorsCut/Art/VFX/`, and **set dressing inside `DC_Set_Hunt.umap` and `DC_Light_Day.umap`**. This starts only after Dev E has finished the signs removal. Once you've started, nobody re-runs the build script on these two sublevels, because the script would wipe your work.

In priority order:
1. **P0 VFX:** `NS_HitImpact` (a cartoon dust/spark puff, keep it comedic) and `NS_BoarDeathPoof` (smoke and stars).
2. **P1 landmarks that are reused in every level:**
   - The Director's Tower: a giant camera with a red tally light, visible from spawn.
   - The painted sky backdrop: it must look like a theatre flat, because it falls over in beat 6.
3. **P1 village and meadow kit:** stylised ground, grass and dirt-path materials; 2–3 house variations, a windmill, and the butcher stall (a hero prop, reused for the meat joke in Level 2); trees and forest edge, rocks, cliffs; hay bales, fences, the sword rack and boar burrows.
4. **P1 greybox swap:** replace the greybox cubes in `DC_Set_Hunt` with the kit, keeping the paths and spawn sight-lines the same.
5. **P1 beat 1 lighting:** tune `DC_Light_Day` (sun, fog, post-process) for a sunny, slightly over-the-top "fantasy movie" look.
6. **P1** `NS_SwordTrail` and `M_DamageVignette` (red screen-edge flash). Hand them to Dev A by name.
7. **P2** `NS_QuestConfetti` for 10/10.

---

## Signs removal (Dev E, first hour) — DONE

The five instruction signs below are deleted from the script and from `DC_Set_Hunt`, `DC_Set_NoMeat` and `DC_Set_PG`; the blank boards behind them stay as props. Line numbers refer to the script before the removal. The joke signs were kept.

Signs are created by `sign_text(...)` in `Scripts/LD_BuildDirectorsCut.py`. Delete them in the editor **and** delete the matching lines in the script, otherwise re-running the script brings them back.

**Remove.** These are instructions or objectives, which the Director now says instead:

| Script line | Sign | Sublevel | Becomes |
|---|---|---|---|
| 372 | "HUNT THE BOARS!" | Set_Hunt | Director line `L1_Intro_04` + quest tracker |
| 373 | "WANTED: 3 BOAR MEAT" | Set_Hunt | Director line in Level 2 + quest tracker |
| 395 | "MEAT: 0 / 3" | Set_NoMeat | Quest tracker in Level 2 |
| 452 | "SHOO ZONE (no hurting!)" | Set_PG | Director line in Level 4 |
| 479–480 | "RATED PG" / "no violence, please :)" | Set_PG | Director line in Level 4 |

**Decide as a team.** These are set-dressing jokes, not instructions. Recommendation: keep them, because they're props the Director "built" and they sell the film-set gag:
- 382 "SOLD OUT" (butcher stall)
- 393 "HA HA HA"
- 417 "DIRECTOR" (chair)
- 433 "SCENE 3 TAKE 2" (clapperboard)
- 511 "WIND FX" (wind machine)
- 529 "EXIT" (the moving door; it's the gameplay object itself)
- 573–574 "ERROR: SCENE NOT FOUND" / "LOADING..." (the glitch level)
- 603 "FINAL CUT"

Also update `Docs/LevelDesign_DirectorsCut.md` so its beat table no longer lists the removed signs.

---

## Full 5-Beat Game Flow & Quest Plan

All beats happen on `Lvl_DirectorsCut` via `ADCGameMode::GoToBeat(1..5)` and `UDCQuestSubsystem`:

| Beat | Trigger | Sublevels | Weapon | Quest (`Id` / Title / Target / `EventTag`) | Gameplay |
|---|---|---|---|---|---|
| **1** | Game start (`IntroDelay`) | `DC_Light_Day`, `DC_Set_Hunt` | Sword (`BP_DC_Sword`) | `Q_HuntBoars` — `"Boars killed"` (`0/10`, `Kill.Boar`) | Normal boar spawns (`MaxAlive=5`). Interrupts at **7/10** kills! |
| **2** | 7/10 boars killed | `DC_Light_Day`, `DC_Set_Hunt`, `DC_Set_NoMeat` | Sword (`BP_DC_Sword`) | `Q_CollectMeat` — `"Boar meat collected"` (`0/5`, `Collect.Meat`) | Boars drop `ADCBoarMeatPickup`. Kill #2 (and every 4th after) drops no meat and triggers the `"this one got no meat"` joke. |
| **3** | 5/5 boar meat collected | `DC_Light_Day`, `DC_Set_Hunt`, `DC_Set_FilmCrew` | Gun (`BP_ShooterWeapon_Rifle`) | `Q_HyperspawnBoars` — `"Boars eliminated"` (`0/100`, `Kill.Boar`) | Sword replaced by gun; spawner enters hyperspawn (`MaxAlive=12`, auto-aggro, 25 HP boars). |
| **4** | 100/100 boars eliminated | `DC_Light_PG`, `DC_Set_Hunt`, `DC_Set_PG` | Gun (`BP_ShooterWeapon_Rifle`) | 4a: `Q_DefeatKingBoar` — `"Defeat the King Boar"` (`0/1`, `Kill.KingBoar`)<br>4b: `Q_RescuePrincess` — `"Rescue the Princess"` (`0/1`, `Rescue.Princess`) | Pastel romance set. Giant crowned `ADCKingBoar` guards `ADCPrincess`; killing him unlocks her to rescue the Princess. |
| **5** | Princess rescued | `DC_Light_Boss`, `DC_Set_Boss` | Gun (`BP_ShooterWeapon_Rifle`) | `Q_DefeatDirector` — `"Defeat the Director"` (`0/1`, `Kill.Director`) | Director enters the arena, calls you a bad actor, and fights you (`ADCDirectorBoss`). |

### Console Commands (`PIE` / Output Log)

- `DC.GoToBeat <1..5>` — Switches sublevels, weapon, spawner, quest, and Director lines to Beat 1–5.
- `DC.CompleteQuest` — Immediately completes the active quest (advances to the next beat).
- `DC.PlayLine <RowId> [RowId...]` — Queues one or more `DT_NarratorLines` rows with subtitles + VO.
- `DC.InterruptLine` — Cuts off the active line and clears the queue.
- `DC.ListLines` — Logs all row IDs in `DT_NarratorLines`.
- `DC.StartQuest <Id> <Target> <EventTag> [Title...]` — Starts a custom quest.
- `DC.ReportEvent <EventTag> [Count]` — Reports quest events manually.
- `DC.SwapWeapon <Sword|Gun>` — Swaps the player's equipped weapon.

---

## Director script (in `DT_NarratorLines`, voiced)

The Director is deadpan and sarcastic: a weary auteur unimpressed by his lead actor, growing increasingly hungry, bored, dramatic, and finally furious. The "Voice line" column is the exact text-to-speech input, with ElevenLabs v3 audio tags in brackets. The subtitle (the `Text` column in `DT_NarratorLines`) is the same line without the tags.

**Voice settings:** `fal-ai/elevenlabs/tts/eleven-v3`, voice `Callum`, stability `0.3`, output `pcm_24000`. Save each clip as `RawAudio/VO/<RowId>.pcm`, then run `Scripts/DC_BuildNarratorLines.py` + `Scripts/DC_ImportVoiceLines.py` in the editor. It imports the clips as `/Game/DirectorsCut/Audio/VO/VO_<RowId>`, sets the `Voice` column, and sets `Duration` to the clip length + 0.4 s. `RawAudio/` is not committed.

### Beat 1: Intro Hunt (Kill 10 Boars, Interrupted at 7)

| Row ID | Trigger | Voice line |
|---|---|---|
| `L1_Intro_01` | Game start | [deadpan] Welcome to the scene. Ah. Our leading actor. Well... casting has made a choice. |
| `L1_Intro_02` | After 01 | Those boars ate the village's crops. The villagers are starving. The boars, on the other hand, look fantastic. |
| `L1_Intro_03` | After 02 | [sarcastic] Fortunately, we have you. And a sword. Apparently that was the entire budget. |
| `L1_Intro_04` | After 03, quest starts when it ends | Fight ten boars. Exactly ten. We're making cinema, not a census. [sighs] And... action. |
| `L1_FirstHit` | First hit on a boar | Ooh, contact. [deadpan] The boar is acting hurt. Very convincing. Somebody nominate the boar. |
| `L1_FirstKill` | 1/10 | [deadpan] One down. Hold that pose... no. Not that one. Keep going. |
| `L1_Half` | 5/10 | Five. Halfway. [sarcastic] The audience is on the edge of their seats. Mostly because the seats are broken. |
| `L1_PlayerHurt` | First time the player is below 50% HP | Try not to die. The insurance on this production is... [sighs] aspirational. |
| `L1_PlayerDied` | Player death | [exasperated] CUT! Someone get the actor back to their mark. [sighs] With feeling this time. |
| `L1_Idle` | No kill for 30 s | [deadpan] Hello? The boars are over there. I can see them from up here. I can see everything from up here. |

### Beat 2: Breakfast / Bacon (At 7/10 Kills -> Collect 5 Boar Meat + No Meat Joke)

| Row ID | Trigger | Voice line |
|---|---|---|
| `B2_Intro_01` | At 7/10 boar kills | [interrupting] Hold it! Stop hitting them for one second. [sighs] All this boar fighting is making me starving. |
| `B2_Intro_02` | After `B2_Intro_01` (primary) | I skipped breakfast for this shoot. I want bacon. Crispy bacon. Bring me five slabs of ham right now. |
| `B2_Intro_02_Alt` | After `B2_Intro_01` (alternate) | [deadpan] Forget the ten boars, the script is changing. Go butcher five cuts of boar meat for my breakfast. |
| `B2_FirstMeat` | 1/5 meat collected | Ooh, yes, look at that marbling. Put it on the craft services table. Four more. |
| `B2_NoMeat_01` | Boar drops no meat (primary) | [deadpan] Wait... seriously? This one got no meat. What is it, stuffed with sawdust? |
| `B2_NoMeat_02` | Boar drops no meat (alternate) | [sarcastic] Another hollow boar. This one got no meat either. Who did our prop catering, an origami club? |
| `B2_AlmostDone` | 4/5 meat collected | Four out of five. One more ham and I can finally eat like a real auteur. |

### Beat 3: Too Boring! Sword -> Gun, Hyperspawn, Kill 100 Boars

| Row ID | Trigger | Voice line |
|---|---|---|
| `B3_Intro_01` | 5/5 meat collected | [groans] Ugh. Actually, watching you poke pigs with a butter knife is putting me to sleep. |
| `B3_Intro_02` | After `B3_Intro_01` | New genre. Action blockbuster! Lose the sword, take this gun. Props department, crank the boar spawner to maximum! |
| `B3_Intro_03` | After `B3_Intro_02` (primary) | [excited] Give me a hundred dead boars! Make it loud! Make it messy! Action! |
| `B3_Intro_03_Alt` | After `B3_Intro_02` (alternate) | [shouts] One hundred boars! Don't stop shooting until the frame rate begs for mercy! |
| `B3_Progress_25` | 25/100 boars | [impressed] Twenty-five down! Now THAT is cinema! Keep firing! |
| `B3_Progress_50` | 50/100 boars | Fifty! Halfway to a hundred! Someone get slow-motion cameras on that pork! |
| `B3_Progress_80` | 80/100 boars | Eighty! Twenty more! My blood pressure is finally at a healthy level! |

### Beat 4: Romance! Rescue the Princess from the Shed Guarded by the King Boar

| Row ID | Trigger | Voice line |
|---|---|---|
| `B4_Intro_01` | 100/100 boars | [gasps] Wait, stop! Cut! Cut! Look at this carnage. It's way too violent! The ratings board will crucify me! |
| `B4_Intro_02` | After `B4_Intro_01` | We need romance! Heart! Whimsy! Quick, turn on the pink lights! |
| `B4_Intro_03` | After `B4_Intro_02` | There is a lovely princess locked in the shed over there. Go rescue her! Oh, and the King Boar is guarding it. Try to look heroic. |
| `B4_ShedLocked` | Touch shed while King Boar lives | [deadpan] Uh, hello? The giant King Boar with the crown is standing right there. Deal with him first. |
| `B4_KingBoarDead` | King Boar defeated | The King Boar is down! Now open that shed and sweep the princess off her feet. Cue the violins! |
| `B4_PrincessRescued` | Princess rescued | And the hero saves the... wait. Look at your face. You have zero romantic chemistry. |

### Beat 5: The Director Gets Mad ("Bad Actor") -> Boss Fight

| Row ID | Trigger | Voice line |
|---|---|---|
| `B5_Intro_01` | After `B4_PrincessRescued` | [angry] CUT! CUT! CUT! Forget it! I can't watch this anymore! |
| `B5_Intro_02` | After `B5_Intro_01` | [sighs] That performance was hopeless! Wooden delivery, zero timing, and you missed every single cue! |
| `B5_Intro_03` | After `B5_Intro_02` | [furious] I'm coming down there myself! If you want a scene done right, the Director does it personally! Fight me! |
| `B5_BossTaunt_01` | Boss combat taunt (primary) | [shouts] That's not your motivation! Hit your mark! |
| `B5_BossTaunt_02` | Boss combat taunt (alternate) | [angry] You're fired! Recast! Somebody call my agent! |
| `B5_BossHalfHP` | Director Boss at 50% HP | [strained] Ow! Hey! That's against union regulations! You can't shoot the director! |
| `B5_DirectorDefeated` | Director Boss defeated | [weakly] Ugh... fine. You win. Terrible performance... five stars. That's... a wrap. |

---

## Timeline

| When | Milestone | Done means |
|---|---|---|
| **Fri, first hour** | Contracts | Contract stubs pushed, placeholders `T_Boar_Placeholder` and `SM_Sword` in, signs removed, `BP_DCGameMode` in the map. |
| **Fri evening** | Ugly but playable | Sword hits a boar, the boar fights back and dies, x/10 counts up, the Director subtitle plays. |
| **Sat morning** | Feels good | Boar sprites, HUD style, hit feedback, the "CUT!" screen → beat 2. |
| **Sat afternoon** | Level 1 done | Environment art, lighting, SFX, music. A packaged build test. Start Level 2 with the same systems. |
| **Sun** | Polish and submit | |

---

## Still to decide (Lead)

        1. **Quest flow:** Beat 1 = kill 10 boars (interrupted at 7/10), Beat 2 = collect 5 boar meat (with the "this one got no meat" joke), Beat 3 = gun + hyperspawn kill 100 boars, Beat 4 = rescue the princess from the shed guarded by the King Boar, Beat 5 = fight the Director boss.
2. **The sword:** start with it, or pick it up from the rack after the Director's "Take up arms!"? The plan assumes the player starts with it. The rack stays as decoration either way.
3. **Player death:** keep quest progress on respawn. Recommended.
4. **Starting numbers:** player 100 HP, sword 25 damage, boar 60 HP, boar bite 10 damage every 1.2 s, at most 5 boars alive at once.
5. **The joke signs** listed in the "Decide as a team" part of the signs section.
