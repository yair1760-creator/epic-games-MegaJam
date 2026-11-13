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

The hit flash uses the camera-fade effect. If the Director's "CUT!" uses a camera fade, a hit during it replaces it. Tell us and we'll move the flash to its own overlay.

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
- **Settings** on `BP_DC_Sword` → **Sword | 2D View**: `bUseSpriteView` (off brings back the 3D Meshy sword), the 4 frame textures, Anim FPS, Windup / Smear / Follow ticks, Screen Scale (1 = the 1920×1080 frame fills the screen height), and walk bob / idle sway.
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

**Niagara slots are empty:** the project has no hit/poof/trail effects yet.

**Tested in `Lvl_Test_CombatAI`** with `log LogDCCombat Verbose` (each sound logs `SFX 2D/3D <name>`): a sword kill played swing, hit, squeal, aggro, charge and death; an angry boar played bite and the player's hurt sound; the meat played its blip; the boss played shot, hit and defeat.

**Packaging:** these sounds are referenced from C++ defaults, so `/Game/DirectorsCut/Audio` is in Project Settings → Packaging → Directories to Always Cook (`Config/DefaultGame.ini`, next to the Narrator folder). Any new sound or music placed in that folder ships automatically.
