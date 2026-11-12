# Director's Cut — Vertical Slice Level Design (Lvl_VerticalSlice)

Units: cm, +X = forward along the critical path. Ground top = Z 0. Total critical path ≈ 270 m (~4–6 min walk + combat).

## Design pillars
1. **One weenie.** The Director's Tower (giant film camera on an 80 m tower, red tally light) is visible from spawn, framed by the village gate, and hidden/revealed along the route. The player always knows where the "ending" is.
2. **Every Narrator beat gets its own space.** Each script beat happens in a zone shaped for that beat (open field for hunting, chokepoint for the weapon swap, winding path for the non-lethal section, a chasm for escalation, an arena for the boss).
3. **Read the path without UI.** Dirt path strip + paired lanterns (warm light) = breadcrumbs. Warm light = go, dark/cold = boundary.
4. **Checkpoint before every rule change.** The Director changes the rules at each zone boundary, so the player respawns right at the start of the beat they died in.

## Zone map (top-down)

```
Y+
 ^        [B] BOAR MEADOW            [D] FOREST (winding)                     [F] SOUND STAGE ARENA   TOWER
 |   [A]  watchtower(view)   ___     ~~ P7 ~~      ~~ P9 ~~            [E]     ___________        ||
 |  VILLAGE  stall  burrows |   |  ~~      ~~ P8 ~~      ~~ P10        CHASM  /  arena    \  ====[CAM]
 |  spawn ->gate=====>      |[C]FILM SET     (PEN)                 ==bridge==>EXIT ramp   |  ====||
 |  windmill                |___| chokepoint                                    \___________/       ||
 +------------------------------------------------------------------------------------------------------> X
   -30m     0    30m        90m  110m            170m        180  200 205 215  230    255      295
```

| Zone | X range (m) | Narrator beat | Shape / purpose | Checkpoint |
|---|---|---|---|---|
| A Village | -30 → 30 | Intro, quest from notice board | Safe hub street, houses frame the gate → Tower sightline | CP_00 spawn |
| B Boar Meadow | 30 → 90 | 1 Intro Quest, 2 Meat Drop Joke | Open fenced field, hay-bale cover, watchtower for overview, butcher stall POI, boar burrows at far end | CP_01 gate |
| C Film Set | 90 → 110 | 3 Director Cut #1 (Sword → Shotgun) | Narrow canyon chokepoint, spot-lit director's chair + weapon pedestal, clapperboard | CP_02 entrance |
| D Forest | 110 → 170 | 4 Director Cut #2 (PG, non-lethal) | Winding path through pines, lantern breadcrumbs, PEN clearing (shoo boars in), tree gap reveals Tower | CP_03 forest entry |
| E Chasm | 170 → 215 | 5 Escalation (weather, obstacles, exit moves) | Ledge → narrow 4 m bridge over chasm → EXIT arch that the Director moves | CP_04 ledge |
| F Sound Stage | 215 → 300 | 6 Breaking Point, 7 Boss | Ramp up to a raised circular arena at the Tower's foot, 4 cover pillars, stage-light rigs | CP_05 ramp base |

## Level design toolkit used
- **Landmark (global):** Director's Tower + camera head + red tally light. **Landmark (local):** village windmill, meadow watchtower, EXIT arch.
- **Points of interest:** Notice board (quest), butcher stall (meat), watchtower (vantage), director's chair + weapon pedestal, boar pen, bridge, EXIT arch, arena.
- **Breadcrumbs:** dirt path strip along critical path, lanterns with warm point lights every ~10 m, alternating sides.
- **Player insight / sightlines:** gate frames the Tower from spawn; watchtower overlooks the meadow; forest blocks view then a gap reveals the Tower; ramp crest reveals the arena.
- **Pacing:** safe (village) → open combat (meadow) → squeeze (film set) → tension/winding (forest) → risk (chasm) → release & climax (arena).
- **Boundaries:** dark perimeter cliffs, dense tree walls, chasm with Kill Z.
- **Gameplay markers:** `BoarSpawn` target points per zone, `Checkpoint` trigger boxes (tag + index), editor-only text labels for each zone/POI.

## Colour legend (greybox)
Grey grid = walkable ground (tinted per zone) · Dark = boundary · Brown = critical path · Yellow = breadcrumb lantern · Orange = POI · Cyan = checkpoint · Red = landmark · Green = foliage.

## Open items
- Checkpoints are live: `UCheckpointSubsystem` (Source/EpicGameJamGame/DirectorsCut) reads `Checkpoint` + `CP_NN` tagged triggers and `Respawn` points, progresses forward only, and the Shooter player controller respawns there (death or falling into Kill Z). Debug: console `DC.GotoCheckpoint N`. Hook Narrator/HUD to its `OnCheckpointActivated` event.
- Boar spawn points placed; boar actor not built yet.
- Replace greybox with Meshy art pass once layout is approved.
