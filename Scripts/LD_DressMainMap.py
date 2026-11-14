# Director's Cut - set dressing for the playable level (Lvl_DirectorsCut + its DC_Set_Hunt sublevel).
#   - the 4 greybox village houses (cube + chamfer roof) become Taisho's painted house_01 as film-set flats
#   - Taisho's grass tufts scattered over the meadow, hay bales at farm spots around the village
#   - the hunt set's 10 greybox hay bales (yellow cylinders) become Taisho's hay bale cut-outs
#   - Taisho's apple trees around the village edges and the meadow border
#   - the Forgive Me Father style floor (SM_DC_SetGround) over the meadow, paths and village square
#   - the round wooden stage floor (SM_DC_SetStage) over the lilac disc "THE SET"
#   - the village notice board (SM_DC_NoticeBoard: Boar King wanted poster + quest "!") over the greybox board
#   - the village well (SM_DC_Well) over the greybox cylinder, and in DC_Set_Hunt the sword rack (SM_DC_SwordRack)
#     with three SM_DC_Sword over the greybox rack
#   - the butcher stall (SM_DC_ButcherStall) over the greybox stall, and in the DC_Set_NoMeat sublevel the SOLD OUT and
#     HA HA HA signs (SM_DC_SoldOutSign, SM_DC_LaughSign) over the greybox boards and text
# Run inside the editor (Lvl_DirectorsCut open):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\LD_DressMainMap.py").read())
#   dress_main_map()            # or dress_main_map("houses") / ("hunt") / ("props") / ("trees") / ("ground") / ("stage") / ("stall") / ("notice") / ("village")
# Re-runnable: clears DC/Village_Flats, DC/SetDressing_Props and DC/SetDressing_Trees (persistent level) and the Hunt_HayProp_* actors first.
# DC_Set_Hunt only gets its greybox hay bales swapped for the real ones and the sword rack; DC_Set_NoMeat only gets the two signs. No other sublevel is touched.
# Note: re-running LD_BuildDirectorsCut's village / hunt stages would bring the greybox back - run this again afterwards.
import unreal, math, os, random

_SCRIPTS = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts"
exec(open(os.path.join(_SCRIPTS, "LD_BuildVerticalSlice.py")).read())   # shared helpers: box/cbox/ramp/_finish/clear_folder/label_text

LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
UES = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
MAIN = "Lvl_DirectorsCut"

# old greybox houses: centre of the 600 x 600 body, and which way the front faces (towards the village square, y = 0)
HOUSES = [("House_0", (-5100, 1900), -90), ("House_1", (-3900, 2100), -90),
          ("House_2", (-5100, -1900), 90), ("House_3", (-3900, -2100), 90)]
FLAT_W, FLAT_H = 545.0, 550.0          # same as the test map: house_01 drawing 2683 x 2712 px, door ~2.7 m

MEADOW = (-5700, -4700, 5700, 4700)    # inside the boundary walls
SQUARE_KEEP_CLEAR = (-6000, -1500, -3400, 1500)


def _check_world():
    w = UES.get_editor_world()
    assert w and w.get_name() == MAIN, "open Lvl_DirectorsCut first (current: %s)" % (w.get_name() if w else None)
    LES.set_current_level_by_name(MAIN)   # spawn into the persistent level, never a sublevel
    return w


def _dir(yaw):
    return math.cos(math.radians(yaw)), math.sin(math.radians(yaw))


def dress_houses():
    CURRENT_FOLDER[0] = "DC/Village_Flats"
    clear_folder("DC/Village_Flats")
    # remove the greybox houses
    gone = []
    for a in EAS.get_all_level_actors():
        lab = a.get_actor_label()
        if str(a.get_folder_path()) == "DC/Village" and lab.startswith("House_") and (lab.endswith("_Body") or lab.endswith("_Roof")):
            gone.append(lab)
            EAS.destroy_actor(a)
    mi = unreal.load_asset("/Game/DirectorsCut/Environment/House01/MI_House01_Facade")
    plane = unreal.load_asset("/Engine/BasicShapes/Plane")
    rng = random.Random(7)
    for name, (cx, cy), face in HOUSES:
        dx, dy = _dir(face)
        s = rng.uniform(0.94, 1.06)                       # a little size variety so the street isn't a copy-paste
        W, H = FLAT_W * s, FLAT_H * s
        fx, fy = cx + dx * 250, cy + dy * 250            # the painting stands where the old front wall was
        px, py = -dy, dx                                   # sideways along the facade
        a = EAS.spawn_actor_from_object(plane, V(fx, fy, H / 2 + 2), unreal.Rotator(roll=90, pitch=0, yaw=face - 90))
        a.set_actor_scale3d(V(W / 100.0, H / 100.0, 1.0))
        a.static_mesh_component.set_material(0, mi)
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        _finish(a, name + "_Facade")
        # plywood board behind the painting: blocks the player like the old house and sells the film-set joke from behind
        cbox(name + "_Board", (fx - dx * 9, fy - dy * 9, H * 0.46), (10, W * 0.82, H * 0.92), "Structure", yaw=face)
        for side in (-1, 1):
            bx, by = fx + px * side * W * 0.3, fy + py * side * W * 0.3
            ramp(name + "_Brace_%d" % (side + 1), (bx - dx * 14, by - dy * 14, H * 0.62), (bx - dx * 330, by - dy * 330, 0), 18, "Trunk", thick=14)
            cbox(name + "_Sandbag_%d" % (side + 1), (bx - dx * 330, by - dy * 330, 18), (70, 50, 36), "Dark", yaw=face)
    return gone


def _blockers():
    """2D boxes (centre, half extents, yaw) of everything grass / hay must not sit on."""
    out = []
    for a in EAS.get_all_level_actors():
        fp = str(a.get_folder_path())
        lab = a.get_actor_label()
        if not (lab.startswith("Path_") or fp.startswith(("DC/POIs", "DC/Landmark", "DC/Village", "DC/Set_", "DC/Exit_"))
                or lab.startswith("POI_") or lab.startswith("BoarSpawn") or a.get_class().get_name() == "PlayerStart"):
            continue
        if lab.startswith("Path_"):
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            l, s = a.get_actor_location(), a.get_actor_scale3d()
            out.append((l.x, l.y, 50 * s.x + 120, 50 * s.y + 120, a.get_actor_rotation().yaw))
            continue
        o, e = a.get_actor_bounds(False)
        if e.x > 2500 or e.y > 2500 or (e.x < 1 and e.y < 1 and not lab.startswith(("BoarSpawn", "PlayerStart"))):
            continue                                      # skip huge ground / lights, and empty helpers
        pad = 350 if lab.startswith(("BoarSpawn", "PlayerStart")) else 150
        out.append((o.x, o.y, e.x + pad, e.y + pad, 0.0))
    out.append((1000, 0, 1350, 1350, 0.0))                # THE SET plinth (round, r 1200)
    return out


def _free(x, y, blockers, extra=0):
    x0, y0, x1, y1 = SQUARE_KEEP_CLEAR
    if x0 < x < x1 and y0 < y < y1:
        return False
    for cx, cy, ex, ey, yaw in blockers:
        c, s = math.cos(math.radians(-yaw)), math.sin(math.radians(-yaw))
        lx, ly = (x - cx) * c - (y - cy) * s, (x - cx) * s + (y - cy) * c
        if abs(lx) < ex + extra and abs(ly) < ey + extra:
            return False
    return True


def dress_props(n_grass=70):
    CURRENT_FOLDER[0] = "DC/SetDressing_Props"
    clear_folder("DC/SetDressing_Props")
    grass, hay = unreal.DCProp_Grass01, unreal.DCProp_HayBale01
    blockers = _blockers()
    rng = random.Random(1115)
    placed = []
    # hay bales: farm spots around the village (checked against the same blockers, bigger margin)
    hay_spots = [(-4700, -3450), (-5600, -3000), (-5650, 2750), (-3350, 3350), (-3250, -3250), (-1900, 3300)]
    n_hay = 0
    for x, y in hay_spots:
        if _free(x, y, blockers, extra=220):
            _finish(EAS.spawn_actor_from_class(hay, V(x, y, 0), R(yaw=rng.uniform(0, 360))), "Prop_HayBale_%d" % n_hay)
            placed.append((x, y)); n_hay += 1
    # grass: random over the meadow, spaced out, off paths / POIs / spawns / set pieces
    x0, y0, x1, y1 = MEADOW
    n = 0
    for _ in range(4000):
        if n >= n_grass:
            break
        x, y = rng.uniform(x0, x1), rng.uniform(y0, y1)
        if not _free(x, y, blockers) or any((x - px) ** 2 + (y - py) ** 2 < 380 ** 2 for px, py in placed):
            continue
        _finish(EAS.spawn_actor_from_class(grass, V(x, y, 0), R(yaw=rng.uniform(0, 360))), "Prop_Grass_%02d" % n)
        placed.append((x, y)); n += 1
    # a few tufts hugging the house flats so they sit in the ground
    for name, (cx, cy), face in HOUSES:
        dx, dy = _dir(face)
        for k, side in enumerate((-1, 1)):
            gx, gy = cx + dx * 300 + (-dy) * side * 320, cy + dy * 300 + dx * side * 320
            _finish(EAS.spawn_actor_from_class(grass, V(gx, gy, 0), R()), "Prop_Grass_%s_%d" % (name, k))
    return {"hay": n_hay, "grass": n + 8}


def dress_hunt_hay():
    """The hunt set (DC_Set_Hunt sublevel, Beats 1-2) has 10 greybox hay bales (yellow cylinders HayBale_0..9):
    swap each for Taisho's hay bale cut-out at the same spot, in the same sublevel (so it still streams with the set)."""
    LES.set_current_level_by_name("DC_Set_Hunt")
    try:
        old = [a for a in EAS.get_all_level_actors()
               if a.get_level().get_outer().get_name() == "DC_Set_Hunt" and a.get_actor_label().startswith("HayBale_")
               and a.get_class().get_name() == "StaticMeshActor"]
        for a in EAS.get_all_level_actors():
            if a.get_level().get_outer().get_name() == "DC_Set_Hunt" and a.get_actor_label().startswith("Hunt_HayProp_"):
                EAS.destroy_actor(a)
        rng = random.Random(42)
        n = 0
        for a in old:
            l = a.get_actor_location()
            o, e = a.get_actor_bounds(False)
            p = EAS.spawn_actor_from_class(unreal.DCProp_HayBale01, V(o.x, o.y, 0), R(yaw=rng.uniform(0, 360)))
            p.set_actor_label("Hunt_HayProp_%d" % n)
            p.set_folder_path("DC/Set_Hunt")
            EAS.destroy_actor(a)
            n += 1
        return n
    finally:
        LES.set_current_level_by_name(MAIN)


def dress_trees(max_trees=9):
    """Taisho's apple trees around the village edges and the meadow border (DC/SetDressing_Trees, persistent level)."""
    CURRENT_FOLDER[0] = "DC/SetDressing_Trees"
    clear_folder("DC/SetDressing_Trees")
    blockers = _blockers()
    taken = [(a.get_actor_location().x, a.get_actor_location().y) for a in EAS.get_all_level_actors()
             if a.get_actor_label().startswith(("Prop_HayBale_", "Hunt_HayProp_", "Edge_Pine_"))]
    spots = [(-5550, 3350), (-4450, 3500), (-3650, 4150), (-4100, -3350), (-3600, -4300), (-1300, 4400), (1500, 4450),
             (-1700, -4450), (2300, -4400), (5300, 2900), (5300, -2700), (-2600, 4500), (3800, 4450)]
    rng = random.Random(202)
    n = 0
    for x, y in spots:
        if n >= max_trees:
            break
        if not _free(x, y, blockers, extra=200) or any((x - tx) ** 2 + (y - ty) ** 2 < 450 ** 2 for tx, ty in taken):
            continue
        _finish(EAS.spawn_actor_from_class(unreal.DCProp_AppleTree02, V(x, y, 0), R(yaw=rng.uniform(0, 360))), "Prop_AppleTree_%d" % n)
        taken.append((x, y)); n += 1
    return n


GREYBOX_GROUND = ("G_Meadow", "G_VillageSquare", "Path_00", "Path_01", "Path_02", "Path_03", "Path_04", "Path_05")


def dress_ground():
    """The Forgive Me Father style floor (SM_DC_SetGround, Scripts/Blender_BuildSetGround.py) over the whole platform.
    Visual only: the greybox meadow / square / paths under it are made invisible but keep their collision and nav."""
    CURRENT_FOLDER[0] = "DC/SetDressing_Ground"
    clear_folder("DC/SetDressing_Ground")
    sm_asset = unreal.load_asset("/Game/DirectorsCut/Environment/SetGround/SM_DC_SetGround")
    a = EAS.spawn_actor_from_object(sm_asset, V(0, 0, 0), R())
    c = a.static_mesh_component
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("can_ever_affect_navigation", False)
    c.set_editor_property("cast_shadow", False)
    _finish(a, "Ground_SetFloor")
    hidden = []
    for x in EAS.get_all_level_actors():
        if x.get_actor_label() in GREYBOX_GROUND and x.get_class().get_name() == "StaticMeshActor":
            x.static_mesh_component.set_editor_property("visible", False)     # still collides, still on the nav mesh
            hidden.append(x.get_actor_label())
    return sorted(hidden)


def dress_stage():
    """The round wooden stage floor (SM_DC_SetStage: Meshy plank picture + Scripts/Blender_BuildSetStage.py) over the lilac disc
    "THE SET" (POI_TheSet_Plinth). Visual only: the disc goes invisible but keeps its collision and nav."""
    CURRENT_FOLDER[0] = "DC/SetDressing_Stage"
    clear_folder("DC/SetDressing_Stage")
    plinth = next((x for x in EAS.get_all_level_actors() if x.get_actor_label() == "POI_TheSet_Plinth"), None)
    assert plinth, "POI_TheSet_Plinth not found"
    o, e = plinth.get_actor_bounds(False)
    sm_asset = unreal.load_asset("/Game/DirectorsCut/Environment/SetFloor/SM_DC_SetStage")   # inked planks (Blender_BuildSetStage.py)
    b = sm_asset.get_bounding_box()
    a = EAS.spawn_actor_from_object(sm_asset, V(o.x, o.y, o.z - e.z), R())
    # fit the model to the disc exactly (it is built to the same 24 m x 40 cm, so the scale is ~1)
    a.set_actor_scale3d(V(2 * e.x / (b.max.x - b.min.x), 2 * e.y / (b.max.y - b.min.y), 2 * e.z / (b.max.z - b.min.z)))
    c = a.static_mesh_component
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("can_ever_affect_navigation", False)
    _finish(a, "Stage_SetFloor")
    plinth.static_mesh_component.set_editor_property("visible", False)        # still collides, still on the nav mesh
    return {"at": [round(o.x), round(o.y), round(o.z - e.z)], "size_cm": [round(2 * e.x), round(2 * e.y), round(2 * e.z)]}


STALL = (-2500, 2800)                  # the butcher stall (same as LD_BuildDirectorsCut)
STALL_DIR = "/Game/DirectorsCut/Environment/ButcherStall"


def _visual_only(a):
    c = a.static_mesh_component
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("can_ever_affect_navigation", False)


def _hide(a):
    """greybox piece goes invisible; it keeps its collision and its place on the nav mesh"""
    if a.get_class().get_name() == "TextRenderActor":
        a.text_render.set_editor_property("visible", False)
    else:
        a.static_mesh_component.set_editor_property("visible", False)


def dress_stall():
    """The butcher stall and its two joke signs (Meshy pictures + Scripts/Blender_BuildButcherStall.py).
    Persistent level: SM_DC_ButcherStall over the greybox counter / posts / roof, and its name board SM_DC_ButcherSign
    ("BUTCHER") above the awning (DC/SetDressing_Stall).
    DC_Set_NoMeat sublevel (beat 2): SM_DC_SoldOutSign and SM_DC_LaughSign over the greybox boards, poles and text,
    spawned into that sublevel so they still stream in with the joke. Visual only: the greybox goes invisible."""
    sx, sy = STALL
    CURRENT_FOLDER[0] = "DC/SetDressing_Stall"
    clear_folder("DC/SetDressing_Stall")
    a = EAS.spawn_actor_from_object(unreal.load_asset(STALL_DIR + "/SM_DC_ButcherStall"), V(sx, sy, 0), R())
    _visual_only(a)
    _finish(a, "Stall_ButcherStall")
    n = EAS.spawn_actor_from_object(unreal.load_asset(STALL_DIR + "/SM_DC_ButcherSign"), V(sx, sy, 0), R())   # name board, same origin
    _visual_only(n)
    _finish(n, "Stall_ButcherSign")
    hidden = []
    grey_main = ["POI_ButcherStall_Counter", "ButcherStall_Roof"] + ["ButcherStall_Post_%d" % j for j in range(4)]
    grey_nomeat = ["SoldOut_Board", "SIGN_SOLD_OUT", "LaughSign_Board", "LaughSign_Pole_0", "LaughSign_Pole_1", "SIGN_HA_HA_HA"]
    for x in EAS.get_all_level_actors():
        lvl = x.get_level().get_outer().get_name()
        if (lvl == MAIN and x.get_actor_label() in grey_main) or (lvl == "DC_Set_NoMeat" and x.get_actor_label() in grey_nomeat):
            _hide(x)
            hidden.append(x.get_actor_label())
    LES.set_current_level_by_name("DC_Set_NoMeat")
    try:
        for x in EAS.get_all_level_actors():
            if x.get_level().get_outer().get_name() == "DC_Set_NoMeat" and x.get_actor_label().startswith("NoMeat_Sign_"):
                EAS.destroy_actor(x)
        for label, asset, loc in (("NoMeat_Sign_SoldOut", "SM_DC_SoldOutSign", (sx, sy - 225, 180)),     # hangs over the counter front
                                  ("NoMeat_Sign_Laugh", "SM_DC_LaughSign", (sx, sy + 300, 0))):          # towers behind the stall
            s = EAS.spawn_actor_from_object(unreal.load_asset(STALL_DIR + "/" + asset), V(*loc), R())
            _visual_only(s)
            s.set_actor_label(label)
            s.set_folder_path("DC/Set_NoMeat")
    finally:
        LES.set_current_level_by_name(MAIN)
    return {"hidden_greybox": sorted(hidden)}


def dress_notice():
    """The village notice board (Meshy poster + Scripts/Blender_BuildNoticeBoard.py): an inked plank board with the
    Boar King wanted poster over the greybox board, and a floating quest-giver "!" (one camera-facing sprite) (POI_NoticeBoard + its two posts).
    Persistent level, DC/SetDressing_Notice. Visual only: the greybox goes invisible but keeps its collision."""
    CURRENT_FOLDER[0] = "DC/SetDressing_Notice"
    clear_folder("DC/SetDressing_Notice")
    board = next((x for x in EAS.get_all_level_actors() if x.get_actor_label() == "POI_NoticeBoard"), None)
    assert board, "POI_NoticeBoard not found"
    o, e = board.get_actor_bounds(False)
    a = EAS.spawn_actor_from_object(unreal.load_asset("/Game/DirectorsCut/Environment/NoticeBoard/SM_DC_NoticeBoard"), V(o.x, o.y, 0), R())
    _visual_only(a)
    _finish(a, "Notice_WantedBoard")
    # the quest-giver "!": ONE camera-facing sprite, so it reads as a single mark from every side and from above
    tex = unreal.load_asset("/Game/DirectorsCut/Environment/NoticeBoard/T_DC_QuestMark")
    q = EAS.spawn_actor_from_class(unreal.DCSpriteActor, V(o.x, o.y, 305), R(yaw=180))
    spr = q.get_editor_property("sprite")
    for prop in ("front_texture", "side_texture", "back_texture"):
        spr.set_editor_property(prop, tex)
    for prop in ("front_size", "side_size"):
        spr.set_editor_property(prop, unreal.Vector2D(57.5, 115.0))
    spr.set_editor_property("cast_shadow", False)
    spr.set_editor_property("pivot_at_bottom", True)
    _finish(q, "Notice_QuestMark")
    hidden = []
    for x in EAS.get_all_level_actors():
        if x.get_level().get_outer().get_name() == MAIN and x.get_actor_label() in ("POI_NoticeBoard", "Notice_PostL", "Notice_PostR"):
            _hide(x)
            hidden.append(x.get_actor_label())
    return {"at": [round(o.x), round(o.y), 0], "hidden_greybox": sorted(hidden)}


VILLAGE_DIR = "/Game/DirectorsCut/Environment/Village"
RACK_SWORD_SCALE = 1.6                 # the game sword is 95 cm; on the rack it is shown bigger so it reads from the spawn
RACK_SWORD_LEAN = 10.0                 # degrees the swords lean back onto the rail


def dress_village(sword_roll=None, sword_flip=True):
    """The village well and the sword rack (Meshy stone picture + Scripts/Blender_BuildVillageProps.py).
    Persistent level: SM_DC_Well over the greybox cylinder "Well" (DC/SetDressing_Village).
    DC_Set_Hunt sublevel: SM_DC_SwordRack over the greybox rack base, and three of the game's SM_DC_Sword standing in it
    instead of the three white sticks, spawned into that sublevel so they still stream with the hunt set.
    Visual only: the greybox goes invisible but keeps its collision."""
    CURRENT_FOLDER[0] = "DC/SetDressing_Village"
    clear_folder("DC/SetDressing_Village")
    acts = EAS.get_all_level_actors()
    well = next((x for x in acts if x.get_actor_label() == "Well" and x.get_level().get_outer().get_name() == MAIN), None)
    assert well, "greybox Well not found"
    o, e = well.get_actor_bounds(False)
    a = EAS.spawn_actor_from_object(unreal.load_asset(VILLAGE_DIR + "/SM_DC_Well"), V(o.x, o.y, 0), R())
    _visual_only(a)
    _finish(a, "Village_Well")
    _hide(well)
    hidden = ["Well"]

    hunt = [x for x in acts if x.get_level().get_outer().get_name() == "DC_Set_Hunt"]
    base = next((x for x in hunt if x.get_actor_label() == "SwordRack_Base"), None)
    assert base, "greybox SwordRack_Base not found in DC_Set_Hunt"
    bo, be = base.get_actor_bounds(False)
    sticks = sorted([x for x in hunt if x.get_actor_label().startswith("SwordRack_Sword_")], key=lambda x: x.get_actor_label())
    for x in [base] + sticks:
        _hide(x)
        hidden.append(x.get_actor_label())
    LES.set_current_level_by_name("DC_Set_Hunt")
    try:
        for x in hunt:
            if x.get_actor_label().startswith("Hunt_Rack"):
                EAS.destroy_actor(x)
        r = EAS.spawn_actor_from_object(unreal.load_asset(VILLAGE_DIR + "/SM_DC_SwordRack"), V(bo.x, bo.y, 0), R())
        _visual_only(r)
        r.set_actor_label("Hunt_Rack")
        r.set_folder_path("DC/Set_Hunt")
        sword = unreal.load_asset("/Game/DirectorsCut/Combat/Sword/SM_DC_Sword")
        # the weapon's own material turns black on a static prop out in the level: use the inked prop material
        # with the sword's colour picture instead (MI_DC_Sword_Prop). sword_flip: blade down, hilt up.
        sword_mi = unreal.load_asset(VILLAGE_DIR + "/MI_DC_Sword_Prop")
        half = 47.5 * RACK_SWORD_SCALE                       # the sword model is 95 cm long, centred on its origin
        lean = math.radians(RACK_SWORD_LEAN)
        roll = (RACK_SWORD_LEAN if sword_roll is None else sword_roll) + (180.0 if sword_flip else 0.0)
        for j, st in enumerate(sticks):
            so, _ = st.get_actor_bounds(False)
            # tip on the base board (42 cm up, 5 cm behind the rack centre), leaning back onto the rail
            loc = V(bo.x - 5 - half * math.sin(lean), so.y, 40 + half * math.cos(lean))
            sw = EAS.spawn_actor_from_object(sword, loc, R(roll=roll, pitch=0, yaw=90))
            sw.set_actor_scale3d(V(RACK_SWORD_SCALE, RACK_SWORD_SCALE, RACK_SWORD_SCALE))
            _visual_only(sw)
            if sword_mi:
                sw.static_mesh_component.set_material(0, sword_mi)
            sw.set_actor_label("Hunt_RackSword_%d" % j)
            sw.set_folder_path("DC/Set_Hunt")
    finally:
        LES.set_current_level_by_name(MAIN)
    return {"well_at": [round(o.x), round(o.y)], "rack_at": [round(bo.x), round(bo.y)], "swords": len(sticks), "hidden_greybox": sorted(hidden)}


def dress_main_map(which="all"):
    _check_world()
    # no ensure_materials(): it re-saves the lead's shared MI_LD_* materials; the ones used here already exist
    out = {}
    with unreal.ScopedEditorTransaction("Dress main map " + which):
        if which in ("all", "houses"):
            out["removed"] = dress_houses()
        if which in ("all", "hunt"):
            out["hunt_hay"] = dress_hunt_hay()
        if which in ("all", "props"):
            out.update(dress_props())
        if which in ("all", "ground"):
            out["ground_hidden_greybox"] = dress_ground()
        if which in ("all", "stage"):
            out["stage"] = dress_stage()
        if which in ("all", "stall"):
            out["stall"] = dress_stall()
        if which in ("all", "notice"):
            out["notice"] = dress_notice()
        if which in ("all", "village"):
            out["village"] = dress_village()
        if which in ("all", "trees"):
            out["apple_trees"] = dress_trees()
    w = UES.get_editor_world()
    unreal.SystemLibrary.execute_console_command(w, "RebuildNavigation")   # hay bales block: refresh the static nav mesh
    return out
