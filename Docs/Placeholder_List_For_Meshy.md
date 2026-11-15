# Placeholders still in the game: list for Meshy

**Updated Nov 15, evening.** Rescanned the open level (every visible static mesh that still uses the lead's flat-colour `MI_LD_*` greybox materials, in the main level and the five beat sets) and the code (meshes, materials and textures loaded by path). The first scan was the same morning; its items A1 to E5 are kept below with their status.

Which set is visible when comes from `ADCGameMode::BeatSublevels`:

| Beat | Sets streamed in |
|---|---|
| 1 Boar Hunt | `DC_Set_Hunt` |
| 2 Breakfast | `DC_Set_Hunt`, `DC_Set_NoMeat` |
| 3 Action Blockbuster | `DC_Set_Hunt`, `DC_Set_FilmCrew` |
| 4 Rated PG Romance | `DC_Set_Hunt`, `DC_Set_PG` |
| 5 Final Cut | `DC_Set_Boss` |

The persistent level (village, boundary, tower, windmill, watchtower) is visible in every beat. `DC_Set_Storm`, `DC_Set_Broken` and `DC_Exit_A/B/C` are never streamed in, so their greybox is never seen and is not listed.

**Meshy balance: 846 credits** (Nov 15 evening; the team shares the pool). A picture (nano-banana-2) costs **6 credits**, a 3D game prop (meshy-t2 + meshy-n1) **15**. "Picture + paint" means one Meshy picture used as the material on the greybox pieces where they stand, so size and collision stay the same (how the arena walls, posts and dolly track were done).

## Still to make

Ranked by how much of the game they cover. Total if everything below is made: **about 70 to 90 credits**.

| # | What | Where / seen | Now | Suggested job | Credits |
|---|---|---|---|---|---|
| 1 | **The Director (the boss)** | `ADCDirectorBoss`, beat 5: the main enemy of the last fight | Built in code from boxes and cylinders (`MI_LD_Dark`, `_White`, `_Landmark`, `_Breadcrumb`) | Designer sprite if one is coming; otherwise a Meshy picture of the Director (red beret, megaphone, scarf, same face as the HUD portrait), transparent background, front view, as a camera-facing sprite like the boars. A hit pose is a second picture | 6 to 12 |
| 2 | **Director's thrown prop** | `ADCDirectorProjectile`, beat 5: every volley he fires | A flat red box (`MI_LD_Landmark`) | One picture (a spinning film reel or a crumpled script page), camera-facing like the boars | 6 |
| 3 | **Shed sides, back and roof** (22 pieces) | `DC_Set_PG`, beat 4: seen whenever the player walks around the shed; only the front has the drawn facade | Pink walls and gables, lilac roof halves, white ridge, sills and door frame, red heart, brown floor and ramp | Two tiled pictures painted onto the pieces where they stand: "pink painted wooden planks, hand-inked" for walls and gables, "lilac roof shingles, hand-inked" for the roof; ridge, sills and frame can reuse the ribbon-post paint, floor and ramp the plywood paint | 12 |
| 4 | **Shed doors** | `ADCShedDoors` (Saar's code), beat 4: seen through the facade's doorway and when they open | Lilac boxes with gold handles | One "pink fairytale door, hand-inked" picture on the door panels. Saar's code and size stay; ask Saar first | 6 |
| 5 | **King Boar crown** (was E1) | `ADCKingBoar` | Code-drawn `T_DC_KingCrown_Placeholder` | Picture, inked gold crown, transparent background | 6 |
| 6 | **Boar HP bar** (was E3) | `UDCFloatingHPBar` over every boar | Plain red and black strips (engine cubes) | One small drawn bar frame, like the player's HP bar | 6 |
| 7 | **Director portrait** in the speech box | `T_DC_Director_Portrait` (Saar's HUD), shown with every line he says, in every beat | Simple flat vector drawing, no ink or hatching like the rest | One inked portrait picture (same beret, sunglasses, megaphone), Meshy image-to-image from the current one; best made together with item 1 so the boss and the portrait match | 6 |
| 8 | **Victory card** ("THAT'S A WRAP!") (was part of E4) | `UDCVictoryWidget` / `WBP_CutScreen`, end of beat 5 | Flat panel; the REC strip, ammo counter and death card got drawn frames on Nov 14 (handoff section 32), this one did not | One drawn card picture like the death card | 6 |
| 9 | **Hit and dust-poof effects** | `T_FX_HitSplat_0..3`, `T_FX_Poof_0..4`: every sword hit, every boar death, the boss defeat | Drawn by script: plain cream shapes with an outline, no hatching | Designer pass fits best (they are short 2D animations); Meshy can make a matching set but frame-to-frame consistency is a risk | 6 to 12 |
| 10 | **Boar side and back views** (was E2) | Every boar: the front drawing shows from all sides | Only the designer's front frames | Designer's job; Meshy image-to-image from the front frames is possible but may not match their style | 6 to 12 per pose |

### Free fixes (no Meshy credits)

| What | Where | Now | Fix |
|---|---|---|---|
| Ground outside the playable area (was A7) | `DC/Ground`: `G_Backstage`, `G_TowerHill` | Flat greybox colours | Reuse the existing ground pictures |
| PG sign poles | `DC_Set_PG` `PGBoard_Pole` (2) | White greybox posts under the heart sign | Ribbon-post paint (`MI_DC_RibbonPost`) |
| Backdrop struts | `DC_Set_Hunt` `Backdrop_Strut` (5), behind the painted backdrop | Brown greybox beams; only seen from behind the backdrop | Plywood paint (`MI_DC_FlatBack_Plywood`) |
| Loading screen backdrop (was E5) | Loading screen | Plain parchment colour behind the running boar | Optional: reuse the main menu picture |
| Shotgun blast sound | `ADCShotgunWeapon` | Epic template sound `FirstPersonTemplateWeaponFire02` | Not Meshy: a fal sound effect (like the other 16 game sounds) |

## All 2D art: where each piece stands

Every flat picture in the game (sprites, HUD, menus, effects), checked Nov 15 evening.

| Group | Pieces | Status |
|---|---|---|
| Player weapons | Sword: 4 frames. Shotgun: 4 shot frames + 2 reload frames | **Final** (designer). Shotgun moved off the crosshair Nov 15 |
| Player death | Dying and dead poses | **Final** (designer) |
| Boar | 10 front frames (idle, run, charge, hit, fall) | **Final** (designer). Side and back views missing: item 10 |
| King Boar crown | 1 picture | **Placeholder**, drawn by code: item 5 |
| The Director (boss) | none | **Missing**, built from boxes: item 1 |
| Director portrait (speech box) | 1 picture | **Placeholder look**, flat vector: item 7 |
| Princess, bacon | 2 pictures | **Final** (designer) |
| Grass, hay bale, apple tree | 3 pictures (+ 3D versions) | **Final** (Taisho) |
| Main menu | Logo, START, QUIT, RESUME, PAUSED panel | **Final** (designer) |
| Main menu | Background, OPTIONS button, "not enough budget" card with the crying boar | **Meshy**, styled on the team's art |
| Loading screen | Running boar (8 frames) + "LOADING" text | **Final** (designer). Background is a plain colour (free fix) |
| HUD | Player HP bar | **Final** (designer) |
| HUD | Health, speech, controls and script frames; REC strip, ammo counter, shell icon, death card | **Meshy**, styled on the team's art |
| HUD | Crosshair, heart, clapper and slate icons | Saar's simple icons; small, they read fine |
| HUD | Victory card | **Flat**: item 8 |
| HUD | HP bar over each boar | **Flat**: item 6 |
| Effects | Hit splat (4 frames), dust poof (5 frames) | **Placeholder look**, drawn by script: item 9 |
| Set cut-outs | Pines (now 3D), flowers, rainbow, PG sign, shed facade, clapperboard, meat hooks, plate, burrows, windmill sails, wanted poster, butcher signs | **Meshy**, done |

## Done (in the game)

| # | What | How it was done |
|---|---|---|
| A1 | Edge pines, 120 | One 3D model, `SM_DC_PineTree` (Meshy image-to-3D from the pine picture, 15 credits), each turned and sized a little differently |
| A2 | Director's Tower | Shaft and base in scaffold paint; Taisho's film camera on top |
| A3 | Windmill | Grouped as `Windmill_Group`; second art (plaster and timber tower, red-tile cap, canvas sails, 18 credits); sail flicker fixed |
| A4 | Scaffold | Scaffold-pipe paint |
| A5 | Boar burrows | Flat burrow pictures on the ground |
| A6 | House flats, back side | Plywood paint (stencil now reads the right way round) and sandbag paint |
| B2 to B5 | Arena walls, cover, light rigs, camera crane | Padded-wall paint, road cases, spotlight model, camera-crane model |
| C1 (front) | Shed facade | Drawn facade card with the doorway cut out (sides, back and roof still to do: item 3 above) |
| C2 to C5 | Flowers, rainbow, PG sign, ribbon posts | Flower and rainbow cut-outs, heart PG sign, ribbon-post paint |
| D1 to D6 | Meat hooks and plate, director's chair, clapperboard, dolly track, light stands, props table | Cut-outs, chair model, clapperboard card, track paint, spotlight model, "PROPS" table |
| Menu | OPTIONS button and "not enough budget" card | Meshy pictures styled on the START button and PAUSED panel, crying in-game boar (18 credits) |
| Backdrop | Painted backdrop ground line | Picture moved so the hills meet the grass |

All of A to D was wired by `Scripts/LD_DressPlaceholders.py` (handoff sections 36 and 37). Every greybox piece behind the art is still there, hidden, with its collision.

## Not listed (no Meshy needed or never seen)

- Greybox pieces hidden under real art (they only keep collision): stall, notice board, well, walls, watchtower, hay bales, houses, backdrop clouds, and every piece replaced in A to D.
- The boar's box body (`ADCBoar`): only shown if the sprite is switched off.
- `DC_Set_Storm`, `DC_Set_Broken`, `DC_Exit_A/B/C`: never loaded in any beat.
- Test map pieces (training dummy, sprite gallery).
- `T_DC_Boar_Front_Placeholder` / `_Side_Placeholder` / `T_DC_BoarMeat_Placeholder`: only fallbacks in code; the real boar frames and the bacon are used.
- Hit and poof effect frames (`Art/FX`) were drawn by script, not by an artist; they are 2D effects, so a designer pass fits better than Meshy.

## If the budget is short

Items 1, 2 and 3 (the Director, his thrown prop, the shed's sides and roof): **24 to 30 credits**, covering the whole last fight and the beat 4 set from every side. Add item 7 (the Director portrait, 6) so the boss and the speech box match.
