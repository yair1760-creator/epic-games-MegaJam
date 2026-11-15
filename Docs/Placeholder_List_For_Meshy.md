# Placeholders still in the game: list for Meshy

Scanned 2026-11-15 from the open level (every visible actor that still uses the lead's flat-colour `MI_LD_*` greybox materials on a basic shape), the code (meshes and textures loaded by path) and the content folders. Which set is visible when comes from `ADCGameMode::BeatSublevels`:

| Beat | Sets streamed in |
|---|---|
| 1 Boar Hunt | `DC_Set_Hunt` |
| 2 Breakfast | `DC_Set_Hunt`, `DC_Set_NoMeat` |
| 3 Action Blockbuster | `DC_Set_Hunt`, `DC_Set_FilmCrew` |
| 4 Rated PG Romance | `DC_Set_Hunt`, `DC_Set_PG` |
| 5 Final Cut | `DC_Set_Boss` |

The persistent level (village, boundary, tower, watchtower) is visible in every beat. **`DC_Set_Storm`, `DC_Set_Broken` and `DC_Exit_A/B/C` are never streamed in, so their greybox is never seen: not listed.**

Cost guide (Meshy): a picture (nano-banana-2) = **6 credits**; a 3D model with texture for a game prop (meshy-t2 smart topology) = **15 credits**. Balance on Nov 15: about 1,155 credits. "Picture + Blender" is the method used for the stall, notice board, well, watchtower: one Meshy picture as the texture, the shape built in Blender to the greybox's exact size so collision still matches.

**Status (Nov 15):** A1 to A6, B2 to B5, C1 to C5 and D1 to D6 are done: the art is in the game, wired by `Scripts/LD_DressPlaceholders.py` (handoff section 36). Still open: A7 (no Meshy needed), B1 (the Director) and section E.

## A. Seen by every player, in every beat (do first)

| # | What | Where | Now | Suggested Meshy job | Credits |
|---|---|---|---|---|---|
| A1 | **Edge pine trees**, 120 of them (crown + trunk) | `DC/Boundary`, all along the walls | Green cones on dark trunks | One inked pine-tree picture with transparent background, used as camera-facing cut-outs (same as the apple trees and grass), or one low-poly 3D pine | 6 (picture) or 15 (3D) |
| A2 | **Director's Tower** (the giant camera on a pole behind the backdrop) | `DC/Landmark`: base, shaft, camera body, lens, rim, 2 reels, tally light | Grey/red/white boxes and cylinders | 3D model: "giant vintage film camera on a tall tower" (meshy-t2), or a picture of the camera for a cut-out | 15 |
| A3 | **Windmill** | `DC/Village`: tower, cap, 4 blades | Flat white/grey shapes | Picture + Blender (wood/plaster windmill), blades stay separate so they can turn | 6 |
| A4 | **Scaffold** behind the low east wall | `DC/Boundary`: 14 posts, 21 beams, 6 rails | Grey metal bars | Picture + Blender: one "painted metal scaffold pipe" picture as texture on the same bars | 6 |
| A5 | **Boar burrows** (where boars come from) | `DC/POIs`: 6 `Burrow` | Brown mounds | One picture of a dirt burrow hole as a ground decal or cut-out | 6 |
| A6 | **House flats, back side** (boards, braces, sandbags behind each painted house) | `DC/Village_Flats`, 4 houses | Grey board, brown braces, dark sandbags | Picture + Blender: "back of a theatre flat, raw plywood with stencilled crate marks" + sandbag picture | 12 |
| A7 | Ground outside the playable area | `DC/Ground`: `G_Backstage`, `G_TowerHill` | Flat colour | Reuse the existing ground pictures (no Meshy needed) | 0 |

## B. Beat 5: the Director fight

| # | What | Where | Now | Suggested Meshy job | Credits |
|---|---|---|---|---|---|
| B1 | **The Director himself** (the boss) | `ADCDirectorBoss` (code builds him from cubes and cylinders with greybox materials) | Box body, cylinder head | This is a character: the designer was going to draw a sprite. Fallback: Meshy picture of the Director (beret, megaphone, scarf, same face as the HUD portrait), transparent background, front view, used as a camera-facing sprite like the boars | 6 to 12 (front + hit pose) |
| B2 | **Arena walls**, 28 | `DC_Set_Boss` `Arena_Wall` | Dark red boxes | Picture + Blender: "studio sound-stage wall, padded panels, painted backdrop canvas" | 6 |
| B3 | Arena cover and low cover, 12 | `Arena_Cover`, `Arena_LowCover` | Grey boxes | Picture + Blender: film road cases / flight cases | 6 |
| B4 | Light rigs, 3 (pole + head) | `Arena_LightRig*` | Dark cylinder + box | 3D "vintage film studio spotlight on stand" (one model, placed 3 times) | 15 |
| B5 | Camera crane | `Crane_Base`, `Crane_Mast`, `Crane_Arm`, `Crane_Platform` | Grey cylinders, red platform | 3D "vintage film camera crane" | 15 |

## C. Beat 4: Rated PG Romance

| # | What | Where | Now | Suggested Meshy job | Credits |
|---|---|---|---|---|---|
| C1 | **The princess shed** (25 pieces: pink walls, lilac roof, door frame, sills, heart, stoop) | `DC_Set_PG` `Shed_*` | Pink/lilac boxes | Picture + Blender: "pink fairytale garden shed, hand-inked" on the same box layout (the doors are Saar's code, keep their size) | 6 to 12 |
| C2 | **Flowers**, 30 (stem + head) | `Flower_00..29` | Coloured boxes on green sticks | One picture of 3 to 4 cartoon flowers on a transparent background, used as cut-outs | 6 |
| C3 | **Rainbow** (125 coloured blocks) | `Rainbow` | Stacked blocks | One rainbow picture on a transparent background as a single big cut-out | 6 |
| C4 | PG board and poles | `PGBoard`, `PGBoard_Pole` | Pink board on white poles | Picture of a heart-shaped "RATED PG" sign (no real rating-board logo) | 6 |
| C5 | Romance posts, 10 | `Romance_Post` | Mint posts | Picture + Blender: pastel fence posts with ribbons | 6 |

## D. Beats 2 and 3

| # | What | Where | Now | Suggested Meshy job | Credits |
|---|---|---|---|---|---|
| D1 | Meat hooks (5) and the empty plate | `DC_Set_NoMeat` (beat 2) | Grey hooks, white plate | Picture + Blender: butcher hooks and an empty platter (logged in the backlog already) | 6 |
| D2 | Director's chair | `DC_Set_FilmCrew` (beat 3) `DirChair_*` | Boxes | 3D "folding director's chair with canvas back" | 15 |
| D3 | Clapperboard | `Clapper_Board`, `Clapper_Stick` | Black box, white stick | Picture + Blender: clapperboard (no text, or "RATED PG FOR PIG") | 6 |
| D4 | Dolly track, 20 rails | `Dolly_Rail_In/Out` | Grey bars | Picture + Blender: metal track texture on the same bars | 6 |
| D5 | Light stands, 4 | `LightStand_*` | Dark pole + box | Same model as B4 | 0 (reuse) |
| D6 | Weapon pedestal (where the shotgun appears) | `POI_WeaponPedestal` | Coloured box | Picture + Blender: prop-department table with a "PROPS" stencil | 6 |

## E. Sprites and 2D art (pictures, not models)

| # | What | Now | Suggested | Credits |
|---|---|---|---|---|
| E1 | **King Boar crown** | Code-drawn placeholder `T_DC_KingCrown_Placeholder` | Meshy picture, inked gold crown, transparent background | 6 |
| E2 | Boar side and back views | Only the designer's front frames exist; the same picture shows from every side | Designer's job; Meshy image-to-image from the front frames is possible but may not match their style | 6 to 12 per pose |
| E3 | Boar HP bar over each boar | Plain red/black strips | One small drawn bar frame picture (like the player's HP bar) | 6 |
| E4 | Ammo counter, REC strip, death slate, victory card | Flat ink/gold panels next to the four new drawn frames | One more frame picture each, or reuse the script-folder frame | 6 to 24 |
| E5 | Loading screen backdrop | Plain parchment colour behind the running boar | Optional: reuse the main menu picture | 0 |

## Not listed (no Meshy needed or not seen)

- Greybox pieces that are hidden under real art (they only keep collision): stall, notice board, well, walls, watchtower, hay bales, houses, backdrop clouds.
- `DC_Set_Storm`, `DC_Set_Broken`, `DC_Exit_A/B/C`: never loaded in any beat.
- Test map pieces (training dummy, sprite gallery).
- `T_DC_Boar_Front_Placeholder` / `_Side_Placeholder` / `T_DC_BoarMeat_Placeholder`: only fallbacks in code; the real boar frames and the bacon are used.
- Hit and poof effect frames (`Art/FX`) were drawn by script, not by an artist; they are 2D effects, so a designer pass fits better than Meshy.
- Sound: one Epic template sound is still in the content (`FirstPersonTemplateWeaponFire02`); not a Meshy item.

## If the budget is short: the eight with the most effect for the least credits

A1 pines (6), A2 tower (15), B1 Director (6 to 12), B2 arena walls (6), C1 shed (6 to 12), C3 rainbow (6), C2 flowers (6), E1 crown (6): about **57 to 69 credits**.
