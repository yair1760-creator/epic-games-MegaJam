# Placeholder replacement art (Meshy, Nov 15)

Covers sections A to D of `Docs/Placeholder_List_For_Meshy.md`. Section E (2D sprites and UI) and B1 (the Director boss) are left for the designer.
The raw Meshy pictures are in `Source/` and the game-ready files are in `Export/`. The art style is inked comic, matching the stall, house and well.

**Wiring rule:** keep every greybox actor where it is, with the same size and collision. Only swap its material, or put a cut-out or mesh on top and hide the old shape.
Tileable textures go on the existing `SM_Cube`/`SM_Cylinder` pieces through a new MI with world-aligned or tiled UVs.
Cut-outs (with alpha) work the same way as the apple trees and grass: a masked, two-sided, camera-facing card.

| # | Placeholder | File(s) in Export | How |
|---|---|---|---|
| A1 | Edge pines (120) | `T_DC_PineTree.png` (768x1376, alpha) | Camera-facing card like the apple trees; replace crown+trunk |
| A2 | Director's Tower camera | reuse `Art/Meshy/FilmSet/SM_DC_FilmCamera.glb` scaled up; shaft/base get `T_DC_ScaffoldPipe` | No new Meshy spend |
| A3 | Windmill | `T_DC_Windmill_Wall.png` (tile) on tower + cap; `T_DC_WindmillSail.png` (alpha) on the 4 blades | Blades stay separate so they can turn |
| A4 | Scaffold | `T_DC_ScaffoldPipe.png` (tile) | On posts, beams and rails |
| A5 | Boar burrows (6) | `T_DC_Burrow.png` (alpha) | Ground decal or flat card on top of the mound |
| A6 | Backs of the house flats | `T_DC_FlatBack_Plywood.png` (tile) on boards and braces; `T_DC_Sandbags.png` (tile) on the sandbags | |
| B2 | Arena walls (28) | `T_DC_ArenaWall.png` (tile) | |
| B3 | Arena cover / low cover | `T_DC_RoadCase.png` (one case side) | Map 0-1 on each face |
| B4 / D5 | Light rigs (3) + light stands (4) | `SM_DC_StudioSpotlight.fbx` + `_base_color.png` | About 3k tris; replace the pole and head, keep a box collider |
| B5 | Camera crane | `SM_DC_CameraCrane.fbx` + `_base_color.png` | About 4k tris; replace the 4 Crane_* pieces |
| C1 | Princess shed | `T_DC_PGShed_Facade.png` (alpha) | Facade card on the front, like `T_House01_Facade`. Keep Saar's door actors and their size |
| C2 | Flowers (30) | `T_DC_Flowers.png` (4 flowers in a row; use UV offsets 0, .25, .5, .75) | Camera-facing cards |
| C3 | Rainbow (125 blocks) | `T_DC_Rainbow.png` (alpha) | One big card; hide the blocks |
| C4 | PG board | `T_DC_PGSign.png` (alpha) | Card on the board, keep the poles |
| C5 | Romance posts (10) | `T_DC_RibbonPost.png` (tile) | |
| D1 | Meat hooks (5) + plate | `T_DC_MeatHook.png`, `T_DC_EmptyPlate.png` (alpha) | Cards |
| D2 | Director's chair | `SM_DC_DirectorChair.fbx` + `_base_color.png` | About 2k tris |
| D3 | Clapperboard | `T_DC_Clapperboard.png` (alpha) | Card on the board + stick |
| D4 | Dolly track rails (20) | `T_DC_DollyRail.png` (tile) | |
| D6 | Weapon pedestal | `T_DC_PropsTable.png` | On the sides of the pedestal box |

The shed facade and PG sign were re-generated on Nov 15 (sources `C1_ShedFacade_v2`, `C4_PGSign_v2`). Those are the final versions.

Not done: A7 (reuse the existing ground pictures; no Meshy needed).
