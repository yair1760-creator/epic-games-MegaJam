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
| Max HP | `BP_ShooterCharacter` → Health → **Max HP** (template value 500; the plan says 100). |

The hit flash uses the camera-fade effect. If the Director's "CUT!" uses a camera fade, a hit during it replaces it. Tell us and we'll move the flash to its own overlay.

## 3. Sword (`BP_DC_Sword`, class `ADCSwordWeapon`)

| Setting | Default | Plan |
|---|---|---|
| Damage | 35 | 25 |
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
| Bite | 40 dmg / 1.0 s | 10 / 1.2 s |
| Flee below | 35% HP | 25% |
| Wander speed | 180 | |
| Charge speed | 650 | |
| Body removed after | 4 s | 3 s |

- **Debug label:** the floating label is on (`bShowDebugState`). **Turn it off on `BP_DC_Boar` for the real game.**

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
2. **With our numbers (sword 35, boar 60 HP) a boar dies in 2 hits, so you almost never see it flee.** With the plan's sword damage of 25, the second hit leaves 10 HP (17%), so the boar flees and dies on the third. Align the sword damage and the flee threshold together.
3. **When you stand right on top of a boar, its card fills the bottom of the screen** (expected for 2.5D sprites). The sword's 2.2 m reach lets you fight from further back.

## 8. Test map zones (`Lvl_Test_CombatAI`)

| Zone | What's there |
|---|---|
| A | Sword lane: 1 m ticks, 3 training dummies with HP readouts, a red damage pad that kills the player |
| B | Fenced boar arena: 6 `BoarSpawn` points, a wander-area box, a `DCBoarSpawner` (4 alive) |
| F | Sprite gallery: a spinning sprite plus fixed ones at different angles and heights (these still use the placeholder boar drawing) |
