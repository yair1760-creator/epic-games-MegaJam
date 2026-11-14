# Handoff: sword, player HP, boars, sprites (packages A, B, F)

This doc covers how the lead and the HUD/quest side connect to the gameplay systems, and how to place them in `Lvl_DirectorsCut`.
- **Everything was built and tested** in the private test map `/Game/DirectorsCut/TestMaps/Lvl_Test_CombatAI`.
- **The main map has none of it placed yet** (see section 1).
- **What is still missing** (art, audio, VFX, balance) is tracked in `Docs/Plan_CombatAI_Sprites.xlsx` → **Polish backlog**.

## 1. Put boars into the main map (lead, 5 minutes)

The 6 `BoarSpawn_0..5` target points are already in the persistent level.

1. Place **`BP_DCBoarSpawner`** (`/Game/DirectorsCut/AI/`) anywhere in the persistent level.
2. Place a **TriggerBox** that covers the playable bowl, so wandering boars stay in the valley. The bowl runs from about (-6000,-5000) to (6000,5000). On the spawner, set **Wander Area** to this box.
3. On the spawner, set **Max Alive** to 5 (the plan's number). Turn **Auto Start off** if the Director should start the hunt.
4. In `ADCGameMode::StartHuntQuest()` (the `TODO(Dev2 hook)`), call `Spawner->SetActive(true)`. To find the spawner: `UGameplayStatics::GetActorOfClass(this, ADCBoarSpawner::StaticClass())`.

The spawner finds the spawn points by their tag, so nothing else needs editing.

## 2. Player

| What | Where |
|---|---|
| Pawn | `BP_ShooterCharacter` (the plan's `BP_DCCharacter` is not a separate asset; use this one). **Starting Weapon = `BP_DC_Sword`.** |
| HP bar for the HUD | Bind `OnHealthChanged(CurrentHP, MaxHP, Delta)` on the Shooter character. Pull with `GetHPPercent()`, `GetCurrentHP()`, `GetMaxHP()`. |
| Death | `OnDied(Character)` fires once. Respawn after `RespawnTime` (5 s) is the existing Shooter flow. |
| Hit feedback | `DCHitFeedbackComponent` on the character: red flash, shake, death fade. |
| Max HP | `BP_ShooterCharacter` → Health → **Max HP** (100 since system 14; was the template's 500). |

The hit flash and the death tint are a full-screen UMG overlay (`UDCScreenTintWidget`, z -5: above the 2D sword, below the HUD), **not** the camera fade, so the Director can use camera fades ("CUT!", beat transitions) freely: a hit during a fade shows the red flash on top and the fade stays. The overlay is removed with the pawn on respawn. Tested 2026-11-14 (`Docs/Screenshots/Sys20_HitFlashOverlay.png`).

## 3. Sword (`BP_DC_Sword`, class `ADCSwordWeapon`)

**Look in first person:** the sword is drawn as **2D hand + sword frames on screen**, like Forgive Me Father 2 / Doom. They are the designer's 4 poses, exported from `Hand_Sword.psd`: transparent PNGs on the full 1920×1080 canvas, all with the same anchor. Source art and the `Dream 14.gif` reference are in `Art/Player/Sword/`.

**Timing:** a **12 fps grid** with hard cuts and no in-betweens.

| Pose | Starts at | Held for |
|---|---|---|
| `T_Sword_Windup` | 0 ms, on the click frame itself | 3 ticks |
| `T_Sword_Swing` (smear) | 250 ms | 1 tick |
| `T_Sword_Follow` | 333 ms | 3 ticks |
| `T_Sword_Idle` | 583 ms | Until the next attack |

The frames live in `/Game/DirectorsCut/Combat/Sword/Sprites/`.
- **One attack per press:** clicks during an attack are ignored.
- **The smear tick can't be skipped,** even at very low frame rates.
- **Hit timing:** the hit lands when the smear appears, **250 ms after the click**.
- **Movement:** the hand bobs while walking and sways slightly when standing still. The attack poses keep their exact drawn placement (no bob).
- **Layering:** the screen layer (`UDCWeaponSpriteWidget`, built in C++) sits **under** the HUD (viewport Z -10).
- **Settings** on `BP_DC_Sword` → **Sword | 2D View**: `bUseSpriteView` (off brings back the 3D Meshy sword), the 4 frame textures, Anim FPS, Windup / Smear / Follow ticks, Screen Scale (1 = the 1920×1080 frame fills the screen height), walk bob / idle sway, and Edge Bleed Pixels. Every pose is pushed (bob + sway + bleed) pixels past the bottom-right corner, so walking never pulls the drawing's cut-off arm edge into view (fixed 2026-11-13; before, the sleeve showed a straight edge while walking).
- **For tests / UI:** `GetSpritePose()` returns Idle / Windup / Swing / Follow, plus `GetAttackElapsed()` and `GetAttackDuration()`.
- **Template ammo counter:** it is hidden while a melee weapon is held (`ShooterPlayerController`), so it doesn't overlap the HUD.

| Setting | Default | Plan |
|---|---|---|
| Damage | 25 (system 14) | 25 |
| Range | 220 cm | |
| Swing radius | 45 cm | |
| Cooldown | 0.55 s | |
| Knockback | 350 | |
| Hold to keep swinging | on | |

- **Events:** `OnSwing()` and `OnHit(Actor, Damage)`, for sounds and Director lines such as "L1_FirstHit".
- **Damage path:** the sword deals damage through `UGameplayStatics::ApplyDamage`, so anything with a `DCHealthComponent` (or the player) reacts.

## 4. Boar (`BP_DC_Boar`, class `ADCBoar` + `ADCBoarAIController`)

- **States:** Idle ↔ Wander → (hit) Angry → Charge → Bite → … → Flee (low HP) → Dead. You can read the state with `GetBoarState()`, and `OnStateChanged(Boar, State)` fires on every change.
- **Quest contract:** on death the boar calls `DCQuestSubsystem->ReportEvent("Kill.Boar")` **exactly once**.
- **Kill events for HUD/Director:** `DCBoarSubsystem` (world subsystem) gives you `OnBoarKilled(Boar, Killer)`, `OnBoarSpawned`, `GetAliveCount()` and `GetKilledCount()`. Each boar also has its own `OnBoarDied`.
- **First hit (for "L1_FirstHit"):** bind `OnStateChanged` and react to the first `Angry`, or bind the sword's `OnHit`.

| Setting | Default | Plan |
|---|---|---|
| HP | 60 (Health → Max HP) | 60 |
| Bite | 10 dmg / 1.0 s (system 14) | 10 / 1.2 s |
| Flee below | 35% HP | 25% |
| Wander speed | 180 | |
| Charge speed | 650 | |
| Body removed after | 4 s | 3 s |

- **Debug label:** off by default and on `BP_DC_Boar`. Tick `Show Debug State` on a boar to see its AI state (Idle / Wander / Angry ...).

## 4b. Meat drops (the "collect boar meat for bacon" part of the hunt)

Boar meat comes from the lead's `ADCBoarMeatPickup` (Combat), which `ADCGameMode` spawns during the bacon beat.

The pickup shows a camera-facing ham picture (`UDCBillboardSprite` + `T_DC_BoarMeat_Placeholder`, 70 cm) with a warm glow; the cube/cylinder parts and the "BOAR HAM" label are gone. Swap the picture via the `Sprite` component's Front Texture when the artist's meat sprite exists. Our earlier `ADCBoarMeat` / `SetMeatDropsEnabled` was removed so there is only one meat system. Boars still report `Kill.Boar` when they die.

## 5. Sprites (`DCBillboardSprite`)

- **What it is:** a 2.5D card that faces the camera. It shows the front, side or back picture depending on the angle and mirrors the side picture by heading. It also plays named frame animations (`PlayAnimation`, `PlayOneShot`).
- **Boar frames:** the boar uses all 10 designer frames (`/Game/DirectorsCut/Characters/Boar/Sprites/T_Boar_*`):
  - **Idle** (with a blink) and a **Run** cycle that speeds up with walking speed
  - **Angry** and **Charge**
  - **Hit** flinch
  - **Hurt** (standing at low HP)
  - **Dead** (falls, then stays down)
  - **Sleep**, set up but not used yet
- **Changing frames:** `BP_DC_Boar` → Sprite → **Animations**.
- **Other uses:** the same component works for the Director (boss) and props: `DCSpriteActor`, or add the component to anything.

## 6. Test commands (Play-in-Editor console)

| Command | Does |
|---|---|
| `DC.Damage 25` / `DC.Heal 25` / `DC.Kill` | Hits, heals or kills whatever DC Health actor you are looking at |
| `DC.HurtMe 50` | Damages the player |
| `DC.StartQuest Q_HuntBoars 10 Kill.Boar Boars killed` | Starts the hunt quest (lead's command) |
| `DC.ReportEvent Kill.Boar 5` | Fakes kills (lead's command) |

## 7. Integration test results (2026-11-13, merged main `ce84e14`)

**Full loop with our test setup** (`BP_ShooterGameMode`):

| Check | Result |
|---|---|
| Player spawns with | `BP_DC_Sword`, 500 HP |
| Spawner at start | 4 boars alive |
| 5 boars killed with the sword | 2 swings each, about 1.3 s per boar |
| Quest | 1 → 5 / 10 |
| `DCBoarSubsystem` killed count | 5 |
| Boar states seen | Angry, Charge, Bite, Flee, Dead |
| 4.5 s after the last kill | Spawner refilled to 4 |
| Player death (`DC.HurtMe 1000`) | Respawned at the PlayerStart with 500 HP and the sword; quest stayed at 5 |

**Same map with the lead's `BP_DCGameMode`** (temporary test only, then restored):

| Check | Result |
|---|---|
| Pawn | `BP_ShooterCharacter` with the sword |
| Intro | The Director's 4 intro lines played, then the hunt quest **started on its own after about 34 s** |
| 2 boar kills | Quest 2 / 10 |

**Things this showed:**
1. **Boars spawn during the 34 s intro, and kills before the quest starts are ignored.** Fix (lead): set **Auto Start = off** on the spawner and call `SetActive(true)` in `StartHuntQuest()` (section 1).
2. **Fixed in system 14:** sword 25 vs boar 60 HP now goes 60 -> 35 (charges) -> 10 (flees) -> dead, tested.
3. **When you stand right on top of a boar, its card fills the bottom of the screen** (expected for 2.5D sprites). The sword's 2.2 m reach lets you fight from further back.

## 8. Test map zones (`Lvl_Test_CombatAI`)

| Zone | What's there |
|---|---|
| A | Sword lane: 1 m ticks, 3 training dummies with HP readouts, a red damage pad that kills the player |
| B | Fenced boar arena: 6 `BoarSpawn` points, a wander-area box, a `DCBoarSpawner` (4 alive) |
| F | Sprite gallery: a spinning sprite plus fixed ones at different angles and heights (these still use the placeholder boar drawing) |

## 9. Balance + weapon rules (system 14)

| What | Value | Where to change it |
|---|---|---|
| Player HP | 100 | `BP_ShooterCharacter` → Max HP |
| Sword | 25 per hit | `BP_DC_Sword` → Damage |
| Boar | 60 HP, bite 10 / 1 s, flees below 35% | `BP_DC_Boar` |
| Beat 3 hyperspawn | 12 alive, 25 HP (1 rifle bullet), bite 5, respawn 0.25 s | Spawner → **Spawner / Hyperspawn** (`HyperMaxAlive`, `HyperBoarHP`, `HyperBiteDamage`, `HyperRespawnDelay`) |
| King Boar | 900 HP (was 400), bite 25 | `DCKingBoar.cpp` |
| Director boss | 1500 HP (was 750), shot 15, every 1.35 s (3-shot spread below half HP) | `DCDirectorBoss.cpp`, `DCDirectorProjectile.h` |
| Rifle (Beats 3-5) | 25 per bullet, 10 per second, 40 per magazine | `BP_ShooterWeapon_Rifle` |

Why the bosses went up: the rifle does about 250 damage per second, so a 400 HP King Boar died in under 2 s and a 750 HP Director in 3 s. Now the King takes about one magazine plus a reload, and the Director about two magazines.

**Sword-only beats:** while the player holds a melee weapon, weapon switching, reload and weapon pickups are ignored (`AShooterCharacter::bLockLoadoutWhileMelee`, `IsLoadoutLocked()`). The level flow's `ReplaceWeaponsWithClass` still swaps to the rifle in Beat 3, and switching and reload work normally with the gun.

**Tested in `Lvl_Test_CombatAI`:** player 100/100; switching and reload do nothing with the sword; the rifle swap unlocks (40-round magazine); a hyperspawn swarm of 12 took 20 HP from a player standing still for 10 s. **Not tested yet:** a full Level 1 run (boss and King Boar feel). That needs a playtest in the main map.

## 10. King Boar visuals (system 15)

- The King Boar is a normal boar sprite at 2.5x, tinted gold (`KingTint`, `KingBrightness`), with a separate camera-facing **crown sprite** (`CrownSprite`, placeholder `T_DC_KingCrown_Placeholder`) on its head. The rotating cylinder and the forced "KING BOAR" text are gone; the debug label follows `bShowDebugState` like any boar.
- On death the crown slides down onto the lying head (`DeadCrownOffset`).
- Tunables on `DCKingBoar`: `CrownOffset`, `DeadCrownOffset`, `GuardRadius` (was a fixed 1800 cm).
- To use real art: set the crown's Front Texture, or give the King its own boar frames on `Sprite`.
- **Fix in `UDCBillboardSprite`:** each sprite now gives its picture a unique name. Before, a second sprite on the same actor replaced the first one's picture, so the boar went invisible and the crown took its place. Any actor can now carry several sprites (useful for the Director boss).
- Screenshots: `Docs/Screenshots/Sys15_King_Alive.png`, `Sys15_King_Dead.png`.

## 11. Sound + effect slots (system 18)

Every slot is a normal property (empty = silent / no effect). All of them play through `UDCFXLibrary` (Combat), which gives world sounds a shared distance falloff (full volume within 3 m, silent at about 40 m), a little random pitch, and a short per-sound cooldown so a 12-boar swarm can't stack the same sound. Blueprint can call `PlayWorldSound`, `PlayUISound` and `SpawnWorldFX` too.

| Class | Slot | When | Default |
|---|---|---|---|
| `DCSwordWeapon` | Swing Sound (2D) / Swing FX | every swing | `S_SwordSwing` / empty (NS_SwordTrail) |
| | Hit Sound / Hit FX | each actor hit, at the impact point | `S_SwordHit` / empty (NS_HitImpact) |
| `DCBoar` | Grunt Sound | now and then while calm (Grunt Interval 4-10 s) | `S_BoarGrunt` |
| | Angry Sound | turns angry | `S_BoarAggro` |
| | Charge Sound | a charge starts | `S_BoarCharge` |
| | Bite Sound | each bite | `S_BoarBite` |
| | Hurt Sound | every hit that doesn't kill | `S_BoarSqueal` |
| | Death Sound / Death FX | dies | `S_BoarDeath` / empty (NS_BoarDeathPoof) |
| | Sound Volume / Sound Pitch | all of the boar's sounds | 1 / 1 (King Boar 1.6 / 0.7) |
| `ShooterCharacter` | Hurt Sound / Death Sound (2D) | takes damage (max every 0.25 s) / dies | `S_PlayerHurt` / empty |
| `DCDirectorBoss` | Attack / Hit / Defeat Sound, Defeat FX | fires / takes damage (max every 0.15 s) / defeated | `S_BossShot` / `S_BossHit` / `S_BossDefeat` / empty |
| `DCBoarMeatPickup` | Pickup Sound (2D) | collected | `S_MeatPickup` |

**The 13 `S_*` sounds in `/Game/DirectorsCut/Audio/SFX` are synthesized placeholders** (`Scripts/Make_PlaceholderSFX.py`, sources in `Art/Audio/SFX_Placeholder`). To use real audio, reimport the real file over the same asset (right-click → Reimport with a new file) and every slot picks it up. Or point a slot at a different asset on the Blueprint.

**Hand-drawn effects (`ADCSpriteFX`, Rendering):** a camera-facing card that plays its frames once and removes itself, in the same ink style as the boars (no Niagara needed). Ready-made and set as defaults:

| Effect | Class | Slot | Frames |
|---|---|---|---|
| Comic impact star → shards, 0.25 s | `ADCFX_HitSplat` | Sword → Hit Sprite FX | `T_FX_HitSplat_0..3` |
| Dust poof, 0.4 s, scaled with the boar (King 2.5x) | `ADCFX_BoarPoof` | Boar → Death Sprite FX | `T_FX_Poof_0..4` |
| Big golden poof, 0.55 s | `ADCFX_BossDefeat` | Director → Defeat Sprite FX | `T_FX_Poof_0..4` (gold tint) |

Frames are placeholders drawn by `Scripts/Make_PlaceholderFX.py` (`Art/FX`, imported to `/Game/DirectorsCut/FX/Textures`); reimport the artist's frames over the same names. New effects: a Blueprint child of `DC Sprite FX` with its own Frames / FPS / Size / Tint, spawned with `SpawnSpriteFX`. The Niagara slots still exist (empty) for anyone who wants particles. Screenshots: `Docs/Screenshots/Sys18b_FX_*.png`.

**Tested in `Lvl_Test_CombatAI`** with `log LogDCCombat Verbose` (each sound logs `SFX 2D/3D <name>`): a sword kill played swing, hit, squeal, aggro, charge and death; an angry boar played bite and the player's hurt sound; the meat played its blip; the boss played shot, hit and defeat.

**Packaging:** these sounds are referenced from C++ defaults, so `/Game/DirectorsCut/Audio` is in Project Settings → Packaging → Directories to Always Cook (`Config/DefaultGame.ini`, next to the Narrator folder). Any new sound or music placed in that folder ships automatically.

## 12. Princess, bacon and set props (system 17 + Taisho's grass / hay)

**Janitor Princess** (`ADCPrincess`, Beats): the cylinder dress, cube head and cylinder crown are replaced by one camera-facing sprite of the designer's drawing (`T_DC_Princess_Janitor`, 1024 x 1536, about 1.9 m tall, `PrincessSprite`). Rescue trigger, romance light, the "PRINCESS (GUARDED)" / "RESCUE THE PRINCESS!" label and all events are unchanged. The old bob animation was dropped (it moved the dress mesh).

**Bacon pickup** (`ADCBoarMeatPickup`): now shows the designer's bacon (`/Game/DirectorsCut/Combat/Pickups/T_DC_Bacon`, 80 cm) instead of the placeholder ham.

**Set props** (`DCSetProps.h`, Rendering): Taisho's grass and hay bale concepts (`Art/Meshy/Grass`, `Art/Meshy/HayBale`, flattened PSD -> PNG) as camera-facing cut-outs with their drawn wooden stands, like film-set props:

| Class (drop in a level) | Texture | Size | Collision |
|---|---|---|---|
| `DC Prop Grass 01` | `T_DC_Prop_Grass01` | ~1.4 m wide, random 85-115% per copy | none |
| `DC Prop Hay Bale 01` | `T_DC_Prop_HayBale01` | ~3 m wide | blocks player + boars (capsule), affects nav |
| `DC Prop Apple Tree 02` | `T_DC_Prop_AppleTree02` (Taisho's Apple_Tree_02) | ~5.5 m tall, random 90-110% | trunk only (walk under the canopy) |
| `DC Set Prop` | set Sprite -> Front Texture / Front Size per instance | any | `bBlocksPawns` |

Textures are in `/Game/DirectorsCut/Environment/Props`. The test map has them in zone P (`Scripts/Test_BuildCombatAIMap.py`, `build_test_map("props")`): 16 grass tufts in the boar arena, 6 around the house, 2 hay bales and a janitor Princess by the house. **Main map (done 2026-11-14, on Yair's request):** `Scripts/LD_DressMainMap.py` (`dress_main_map()`, re-runnable):
- the 4 greybox village houses (cube + chamfer roof) are now Taisho's painted house_01 as film-set flats facing the square, with plywood boards, braces and sandbags behind (`DC/Village_Flats`, persistent level);
- the hunt set's 10 greybox hay bales (yellow cylinders in `DC_Set_Hunt`) are now `DC Prop Hay Bale 01` at the same spots, still in that sublevel (`Hunt_HayProp_*`);
- 78 grass tufts over the meadow (off paths, POIs, boar spawns, set pieces) and 3 more hay bales around the village (`DC/SetDressing_Props`, persistent level);
- 5 apple trees behind the village houses and along the meadow border (`DC/SetDressing_Trees`, `dress_main_map("trees")`);
- nav mesh rebuilt; all 6 boar spawns are still reachable from the village. Only `Lvl_DirectorsCut` and `DC_Set_Hunt` were saved.
Re-running the lead's `LD_BuildDirectorsCut.py` village / hunt stages would bring the greybox back; run `dress_main_map()` again afterwards.

Screenshots: `Docs/Screenshots/Sys17_*.png` (`Sys17_Main_*` = the main map).

### 12b. The props in 3D (Blender)

Taisho's three drawings are rebuilt as real 3D models painted with the drawings themselves (Forgive Me Father style, ink lines included), so every side looks right. `Scripts/Blender_BuildProps3D.py` (`blender -b --python Scripts/Blender_BuildProps3D.py -- preview`) builds them, keys out the white sticker card, crops off the wooden stands, exports `Art/Meshy/<Prop>/Export/SM_*.glb` + textures, and renders each prop from 4 sides into `Saved/PropPreview/`.

| Mesh (`/Game/DirectorsCut/Environment/Props/3D`) | Built from the drawing | Size |
|---|---|---|
| `SM_DC_HayBale01_3D` | lying round bale (spiral end caps, rolled sides, rounded rims) + square bale + pitchfork | 2.2 x 1.6 x 1.5 m |
| `SM_DC_Grass01_3D` | 5 crossed cut-out cards of the drawn blades: a full clump from any side | 1.25 m wide |
| `SM_DC_AppleTree02_3D` | flared trunk + 5 branches (drawn bark), leaf-painted canopy core, 4 crossed cards of the drawn leafy outline, fallen apples, grass skirt | 5.2 m tall |

Materials: `MI_DC_*_3D` on `M_DC_InkedProp` (lit, masked, two-sided). The `DC Prop ...` classes now show the 3D mesh by default (`bUse3DModel`; off = the 2D camera-facing card), so every placed copy (main map, hunt set, test map) switched without re-placing anything. Collision is still the invisible capsule (hay blocks, tree trunk blocks, grass none), so the nav mesh is unchanged.

### 12c. Round film-set stage (Meshy picture + Blender)

`SM_DC_SetStage` (`/Game/DirectorsCut/Environment/SetFloor`, material `MI_DC_SetStage` on `M_DC_InkedProp`, texture `T_DC_SetStage_Planks`): the round wooden stage in the middle of the platform, in the Forgive Me Father comic style (inked floorboards, nails, scratches). 24 m across and 40 cm tall - the same footprint and height as the grey plinth `POI_TheSet_Plinth` ("THE SET") it covers.

- **Made with Meshy:** one text-to-image plank picture (nano-banana-2, 6 credits): `Art/Meshy/SetFloor/Source/Planks_meshy_image.png`.
- **Built in Blender:** `Scripts/Blender_BuildSetStage.py` (`blender -b --python ... -- preview`): a round platform with 30 cm planks running across the top (the picture repeats every 2.4 m), a ring of boards around the top edge and one board all the way round the side. Exports `Art/Meshy/SetFloor/Export/SM_DC_SetStage.glb` + `.blend` for hand edits.
- **Placed on the playable level (2026-11-14):** `Stage_SetFloor` in `DC/SetDressing_Stage`, via `dress_main_map("stage")`: no collision of its own; `POI_TheSet_Plinth` is invisible but still collides and carries the nav mesh. Checked in play: the player walks onto it and nav reaches its centre.
- Limits: from far away the 2.4 m repeat of the picture shows as a faint grid; there are no gaffer-tape marks or other stage dressing on it yet.
- Screenshots: `Docs/Screenshots/SetStage_Play_Approach.png`, `SetStage_Play_On.png`.
- **Superseded:** the first attempt `SM_DC_SetFloor` / `MI_DC_SetFloor` / `T_DC_SetFloor` (a Meshy 3D model stretched to 24 m by `Scripts/Blender_AdjustSetFloor.py`: orange, soft, 1-2 m planks) is still in the project but no longer used anywhere - safe to delete.

### 12d. The whole platform's floor (Meshy + Blender)

`SM_DC_SetGround` (`/Game/DirectorsCut/Environment/SetGround`): ONE flat floor mesh for the whole playing platform, 120 m x 100 m, in the Forgive Me Father comic style (thick ink lines, flat muted colours), with three materials:

| Surface | Covers | Texture | Made from |
|---|---|---|---|
| Grass | the meadow (`G_Meadow`) | `T_DC_Ground_Grass`, repeats every 8 m | Meshy text-to-image (nano-banana-2): the sparse picture `Grass_sparse_meshy_image.png`, adjusted by the build script |
| Dirt | the six paths (`Path_00..05`) | `T_DC_Ground_Dirt`, one picture across the 3.5 m path | Meshy text-to-image (nano-banana-2) |
| Cobblestone | the village square (`G_VillageSquare`) | `T_DC_Ground_Cobble`, repeats every 6 m | Meshy text-to-3D tile (meshy-t2.5 + texture), photographed square-on in Blender and mirrored 2 x 2 |

- `Scripts/Blender_BuildSetGround.py` builds it (`-- preview` renders checks): makes each texture seamless, then builds the floor to the level's own layout (the `MEADOW` / `SQUARE` / `PATHS` numbers at the top are the greybox pieces, read from the level on 2026-11-14). If paths move, update those numbers and re-run.
- The mesh is **visual only**: it lies 0.6 cm above the greybox ground, which keeps doing collision and navigation. Place it at (0, 0, 0) with no rotation and collision off; hiding the greybox meshes is optional (they are fully covered).
- Meshy credits: 45 for three 3D tiles (the grass tile failed - loose tufts only - and the dirt tile came out as a grey disc; both are kept unused in `Art/Meshy/Ground/Source`) + 12 for the two pictures that replaced them. + 6 for the sparse grass picture.
- **Placed on the playable level (2026-11-14):** `Ground_SetFloor` in `DC/SetDressing_Ground` (persistent level), via `dress_main_map("ground")` in `Scripts/LD_DressMainMap.py`. The greybox `G_Meadow`, `G_VillageSquare` and `Path_00..05` are set invisible but still collide and carry the nav mesh; in play the player stands at the same height and all 6 boar spawns are reachable. The backstage / tower-hill areas behind the backdrop and the lilac set disc are not covered.
- Screenshots: `Docs/Screenshots/SetGround_*.png` (a temporary copy in the level, deleted afterwards).

- **Path junction flicker fixed (2026-11-14):** the six path strips overlap where they meet and were all at the same height, so the overlapping faces flickered as the camera moved (z-fighting). Each strip now sits 0.5 cm above the one before it (`PATH_STEP` in the build script; the top one is 2.5 cm higher than the first). The mesh was reimported over the same asset; nothing in the level changed. Screenshot: `Docs/Screenshots/SetGround_PathJunction.png`.
- **Sparser grass (2026-11-14):** the first grass picture (tufts edge to edge, `Grass_meshy_image.png`, kept unused) read as crowded in play. `sparse_grass()` in `Scripts/Blender_BuildSetGround.py` now builds the meadow texture from the sparse picture: three shifted copies laid over each other (about 27 tufts per 8 m), ink lines thickened, ground colour matched to the old picture. Only the texture changed; the floor mesh is the same. Limit: up close the 1024 px picture over 8 m shows stepped ink edges. Screenshots: `Docs/Screenshots/SetGround_Grass_Sparse_Meadow.png`, `SetGround_Grass_Sparse_Close.png`.

### 12e. Butcher stall, SOLD OUT and HA HA HA signs (Meshy pictures + Blender)

Four models in `/Game/DirectorsCut/Environment/ButcherStall`, built by `Scripts/Blender_BuildButcherStall.py` (`blender -b --python ... -- preview`) and placed by `dress_main_map("stall")`:

| Mesh | What | Where it lives |
|---|---|---|
| `SM_DC_ButcherStall` | plank counter (8 x 4 x 1.1 m), 4 posts, beams, sloped red-and-white striped awning with a scalloped front edge | `Lvl_DirectorsCut`, `Stall_ButcherStall` in `DC/SetDressing_Stall` (always there) |
| `SM_DC_ButcherSign` | the stall's name board: brown planks with cream "BUTCHER", a cleaver and a ham, 5.4 x 1.7 m on two stubs above the back of the awning | `Lvl_DirectorsCut`, `Stall_ButcherSign` in `DC/SetDressing_Stall` (always there) |
| `SM_DC_SoldOutSign` | plank board with white inked "SOLD OUT", hanging on two ropes over the counter front | `DC_Set_NoMeat` sublevel, `NoMeat_Sign_SoldOut` (beat 2 only) |
| `SM_DC_LaughSign` | 12.6 m dark billboard on two poles with red dripping "HA HA HA", top at 11.2 m, behind the stall | `DC_Set_NoMeat` sublevel, `NoMeat_Sign_Laugh` (beat 2 only) |

- **Meshy (24 credits):** four text-to-image pictures (nano-banana-2) in `Art/Meshy/ButcherStall/Source`: awning cloth, SOLD OUT board, HA HA HA board, BUTCHER board. The SOLD OUT board came out as a photo, so only its letters are used: the script cuts them out, outlines them in ink and puts them on the stage's inked plank picture. All wood reuses `MI_DC_SetStage`.
- **Greybox:** the old counter, posts, roof, sign boards, poles and the two text actors are invisible, not deleted (12 actors), so collision and the nav mesh are unchanged. The new meshes have no collision.
- **Checked in play:** stall alone in beat 1; with `DC_Set_NoMeat` loaded both signs appear and read correctly; nav still reaches the stall front.
- Limits: the SOLD OUT letters look grey when the awning's shadow falls on the board; the meat hooks and empty plate of beat 2 are still greybox; nothing is on the counter (no meat, cleaver or scales).
- Screenshots: `Docs/Screenshots/ButcherStall_Beat1.png`, `ButcherStall_Beat2_Close.png`, `ButcherStall_Beat2_Far.png`.

**Merge note (2026-11-14):** Saar's commit `b10990e` and the stall work both changed `Lvl_DirectorsCut.umap`. Our version of the level was kept, and Saar's one level change was re-created by hand exactly as his `LD_BuildDirectorsCut.py` does it: the `BoarWanderArea` trigger box (centre 0, 0, 500; extent 5700 x 4700 x 500; tag `BoarWanderArea`; folder `DC/Gameplay`) and `DC_BoarSpawner.wander_area` pointing at it. Checked in play: a spawned boar has the area set.

## 13. 2D shotgun view (designer frames, 12 fps)

`ADCShotgunWeapon` (Saar's Beat 3-5 shotgun) now draws the designer's hand-drawn frames on screen, the same way the sword does (`UDCWeaponSpriteWidget`, full-screen layer under the HUD). The 3D gun and the template arms are hidden in first person; pellets, ammo, sound and camera kick are unchanged.

| Frame | Starts | Held |
|---|---|---|
| `T_Shotgun_Idle` | before input | until the player fires |
| `T_Shotgun_Flash` (flash drawn in) | 0 ms, on the accepted shot itself | 2 ticks (167 ms) |
| `T_Shotgun_Recoil` (peak) | 167 ms | 1 tick (83 ms) |
| `T_Shotgun_Recovery` | 250 ms | 2 ticks (167 ms) |
| `T_Shotgun_Idle` | 417 ms | until the next shot |

- Frames: `/Game/DirectorsCut/Combat/Shotgun/Sprites/` (1920 x 1080, UI group, no streaming). Source art, the PSD, the reference GIF and the designer's `Timing.txt`: `Art/Player/Shotgun`.
- The sequence starts in `FireProjectile`, so it plays once per accepted shot: an empty magazine, a reload or a click during the refire delay does not play it. `RefireRate` (0.55 s) is kept at least as long as the animation (0.417 s), so a shot never cuts the previous one off.
- Hard cuts on the tick grid; the one-tick peak recoil is always shown, even if a slow frame would skip it.
- Walk bob and idle sway only while idle; the shot drawings keep their exact drawn placement.
- Tunables on the weapon (`Shotgun|2D View`): `bUseSpriteView` (off = the old 3D gun), `AnimFPS`, `FlashTicks`, `RecoilTicks`, `RecoveryTicks`, `SpriteScreenScale`, `WalkBobPixels`, `IdleSwayPixels`, `EdgeBleedPixels`. For tests: `GetSpritePose()`, `GetShotElapsed()`, `GetShotAnimDuration()`.
- Checked in play (`DC.SwapWeapon Gun`): pose changes measured at 0 / 168 / 251 / 418 ms; a second click 200 ms into the animation does not restart it; swapping back to the sword removes the shotgun layer.
- Limits: no reload drawing (the idle frame stays up while reloading); the pellets still leave from the hidden 3D gun's muzzle, not from the drawn barrel; Saar's automation tests were not re-run.
- Screenshots: `Docs/Screenshots/Shotgun2D_Idle.png`, `Shotgun2D_Flash.png`, `Shotgun2D_Recoil.png`, `Shotgun2D_Recovery.png`.
