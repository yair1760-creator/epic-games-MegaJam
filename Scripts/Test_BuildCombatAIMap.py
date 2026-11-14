# Director's Cut - private test map for the sword / player HP, boar AI and camera-facing sprite systems.
# This map is ours alone: the main map (Lvl_DirectorsCut) and its sublevels are never opened or saved by this script.
#
# Run inside the editor:
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\Test_BuildCombatAIMap.py").read())
#   build_test_map()
#
# Re-runnable: every zone clears its own outliner folder (Test/<Zone>) before rebuilding.
import unreal, math, os, random

_SCRIPTS = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts"
exec(open(os.path.join(_SCRIPTS, "LD_BuildVerticalSlice.py")).read())   # shared greybox helpers + materials

LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
TEST_MAP = "/Game/DirectorsCut/TestMaps/Lvl_Test_CombatAI"
MAIN_MAP = "/Game/DirectorsCut/Maps/Lvl_DirectorsCut"

# Layout (cm). Spawn hub at the origin facing +X; one zone per system.
FLOOR = (-1500, -3500, 7500, 3500)
SPAWN = (0, 0)
ZONE_A = (1000, 1500, 6500, 3300)     # sword + player HP lane (north)
ZONE_B = (1500, -1200, 7000, 1200)    # boar arena (centre, fenced)
ZONE_F = (1000, -3300, 6500, -1500)   # sprite gallery (south)


def _world():
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    return w or unreal.EditorLevelLibrary.get_editor_world()


def trigger(label, loc, extent, tags, yaw=0):
    t = EAS.spawn_actor_from_class(unreal.TriggerBox, V(loc[0], loc[1], loc[2] + extent[2]), R(yaw=yaw))
    t.get_component_by_class(unreal.BoxComponent).set_box_extent(V(*extent), True)
    t.tags = [unreal.Name(x) for x in tags]
    return _finish(t, label)


def ensure_test_map():
    cur = _world().get_path_name().split(".")[0]
    if cur == TEST_MAP:
        return _world()
    if cur == MAIN_MAP:
        # never carry unsaved edits out of the shared map by accident
        dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        if dirty:
            raise RuntimeError("Main map has unsaved changes; save or discard them yourself before switching.")
    if unreal.EditorAssetLibrary.does_asset_exist(TEST_MAP):
        LES.load_level(TEST_MAP)
    else:
        LES.new_level(TEST_MAP)
    return _world()


def z_base():
    CURRENT_FOLDER[0] = "Test/Base"
    clear_folder("Test/Base")
    x0, y0, x1, y1 = FLOOR
    box("Floor", (x0, y0, -100), (x1, y1, 0), "Ground_Village")
    # perimeter walls
    h = 400
    box("Wall_N", (x0, y1, 0), (x1, y1 + 100, h), "Boundary")
    box("Wall_S", (x0, y0 - 100, 0), (x1, y0, h), "Boundary")
    box("Wall_E", (x1, y0, 0), (x1 + 100, y1, h), "Boundary")
    box("Wall_W", (x0 - 100, y0, 0), (x0, y1, h), "Boundary")
    # zone dividers: knee-high curbs so every zone stays visible from spawn
    box("Curb_AB", (1000, 1350, 0), (7500, 1450, 40), "Dark")
    box("Curb_BF", (1000, -1450, 0), (7500, -1350, 40), "Dark")
    # spawn pad
    cyl("SpawnPad", (SPAWN[0], SPAWN[1], 0), 400, 5, "Checkpoint")
    ps = EAS.spawn_actor_from_class(unreal.PlayerStart, V(SPAWN[0], SPAWN[1], 100), R(yaw=0))
    ps.tags = [unreal.Name("RED"), unreal.Name("BLUE"), unreal.Name("Player0")]
    _finish(ps, "PlayerStart_Test")
    nav = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(3000, 0, 200), R())
    nav.set_actor_scale3d(V(48, 38, 6))
    _finish(nav, "NavMeshBounds_Test")
    ws = _world().get_world_settings()
    ws.set_editor_property("kill_z", -1000.0)
    gm = unreal.load_class(None, "/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode.BP_ShooterGameMode_C")
    if gm:
        ws.set_editor_property("default_game_mode", gm)
    label_text("TEST MAP - not the main map", (-1400, 0, 450), 90, yaw=0, color=(255, 255, 255))


def z_light():
    CURRENT_FOLDER[0] = "Test/Light"
    clear_folder("Test/Light")
    _finish(EAS.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0), R()), "SkyAtmosphere")
    sl = EAS.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500), R())
    c = sl.get_component_by_class(unreal.SkyLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("real_time_capture", True)
    _finish(sl, "SkyLight")
    d = EAS.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 2000), R(pitch=-50, yaw=40))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_mobility(unreal.ComponentMobility.MOVABLE)
    dc.set_intensity(10)
    dc.set_editor_property("atmosphere_sun_light", True)
    _finish(d, "Sun")
    f = EAS.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0), R())
    f.get_component_by_class(unreal.ExponentialHeightFogComponent).set_fog_density(0.005)
    _finish(f, "Fog")


def z_combat():
    """Zone A: sword + player HP. Dummies at fixed distances, range ticks, a damage pad."""
    CURRENT_FOLDER[0] = "Test/A_Combat"
    clear_folder("Test/A_Combat")
    x0, y0, x1, y1 = ZONE_A
    yc = (y0 + y1) / 2
    box("A_Floor", (x0, y0, 0), (x1, y1, 2), "Ground_Meadow")
    # range ticks every 1 m from the firing line at x=1500, labelled every 2 m
    box("A_FiringLine", (1490, y0 + 100, 2), (1510, y1 - 100, 5), "Breadcrumb")
    for m in range(1, 9):
        x = 1500 + m * 100
        box("A_Tick_%dm" % m, (x - 4, yc - 250, 2), (x + 4, yc + 250, 4), "White")
        if m % 2 == 0:
            label_text("%d m" % m, (x, yc - 320, 20), 40, yaw=180)
    # training dummies (C++ ADCTrainingDummy: DC Health + floating HP; falls back to a plain post before it is compiled)
    dummy_cls = getattr(unreal, "DCTrainingDummy", None)
    for i, x in enumerate([1700, 2000, 2500]):
        if dummy_cls:
            d = EAS.spawn_actor_from_class(dummy_cls, V(x, yc + 500, 0), R(yaw=180))
            d.root_component.set_material(0, mat("POI"))
            _finish(d, "A_Dummy_%d" % i)
        else:
            cyl("A_Dummy_%d" % i, (x, yc + 500, 0), 70, 180, "POI")
        target_point("A_DummySpot_%d" % i, (x, yc + 500, 50), 180, ("TestDummy",))
    # damage pad to test player HP / hit feedback / death + respawn
    box("A_DamagePad", (4500, yc - 300, 2), (5100, yc + 300, 6), "Landmark")
    target_point("A_DamagePadSpot", (4800, yc, 50), 0, ("TestDamagePad",))
    vol_cls = getattr(unreal, "DCDamageVolume", None)
    if vol_cls:
        v = EAS.spawn_actor_from_class(vol_cls, V(4800, yc, 100), R())
        v.get_component_by_class(unreal.BoxComponent).set_box_extent(V(300, 300, 100), True)
        v.set_editor_property("damage_per_tick", 50.0)
        v.set_editor_property("tick_interval", 0.5)
        _finish(v, "A_DamagePadVolume")
    label_text("A  SWORD + PLAYER HP", (x0 + 200, yc, 350), 80, yaw=180, color=(255, 200, 60))
    label_text("dummies", (2000, yc + 500, 260), 40, yaw=180)
    label_text("damage pad", (4800, yc, 120), 40, yaw=180, color=(255, 80, 80))


def z_boar():
    """Zone B: fenced boar arena with 6 spawn points (same tag as the main map), plus a single-boar pen."""
    CURRENT_FOLDER[0] = "Test/B_Boar"
    clear_folder("Test/B_Boar")
    x0, y0, x1, y1 = ZONE_B
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    box("B_Floor", (x0, y0, 0), (x1, y1, 2), "Ground_Forest")
    # waist-high fence; the player can see and swing over it, a gate on the spawn side
    fh = 110
    box("B_Fence_N", (x0, y1 - 30, 0), (x1, y1, fh), "Trunk")
    box("B_Fence_S", (x0, y0, 0), (x1, y0 + 30, fh), "Trunk")
    box("B_Fence_E", (x1 - 30, y0, 0), (x1, y1, fh), "Trunk")
    box("B_Fence_W1", (x0, y0, 0), (x0 + 30, -250, fh), "Trunk")
    box("B_Fence_W2", (x0, 250, 0), (x0 + 30, y1, fh), "Trunk")
    # a few obstacles so the boar AI has to path around things
    cbox("B_Rock_0", (cx - 800, cy + 400, 60), (250, 200, 120), "Dark", yaw=20)
    cbox("B_Rock_1", (cx + 600, cy - 450, 60), (300, 180, 120), "Dark", yaw=-35)
    cyl("B_Stump", (cx + 1500, cy + 300, 0), 120, 80, "Trunk")
    # 6 spawn points around the edge facing the centre (tag matches the main map's BoarSpawn points)
    pts = [(x0 + 300, y1 - 300), (cx, y1 - 300), (x1 - 300, y1 - 300),
           (x0 + 300, y0 + 300), (cx, y0 + 300), (x1 - 300, y0 + 300)]
    for i, (x, y) in enumerate(pts):
        target_point("BoarSpawn_%d" % i, (x, y, 50), math.degrees(math.atan2(cy - y, cx - x)), ("BoarSpawn",))
        cyl("B_SpawnMark_%d" % i, (x, y, 2), 120, 3, "Breadcrumb")
    label_text("B  BOAR AI + SPAWNER", (x0 - 100, cy, 350), 80, yaw=180, color=(120, 255, 120))
    # wander area: inside the fence, so wandering boars never walk out through the gate
    area = trigger("B_WanderArea", (cx, cy, 0), ((x1 - x0) / 2 - 150, (y1 - y0) / 2 - 150, 150), ("BoarWanderArea",))
    # boar spawner (system 7): uses the 6 BoarSpawn points above, keeps 4 boars alive inside the fence
    spawner_cls = getattr(unreal, "DCBoarSpawner", None)
    if spawner_cls:
        s = EAS.spawn_actor_from_class(spawner_cls, V(cx, cy, 300), R())
        s.set_editor_property("wander_area", area)
        s.set_editor_property("max_alive", 4)
        _finish(s, "B_BoarSpawner")
    else:
        # before the spawner existed (system 4): one hand-placed boar
        boar_cls = unreal.load_class(None, "/Game/DirectorsCut/AI/BP_DC_Boar.BP_DC_Boar_C")
        if boar_cls:
            b = EAS.spawn_actor_from_class(boar_cls, V(cx - 1200, cy, 60), R(yaw=0))
            b.set_editor_property("wander_area", area)
            _finish(b, "B_TestBoar")


def z_sprites():
    """Zone F: sprite gallery. Slots on a ring so you can walk around and check the sprite always faces you."""
    CURRENT_FOLDER[0] = "Test/F_Sprites"
    clear_folder("Test/F_Sprites")
    x0, y0, x1, y1 = ZONE_F
    yc = (y0 + y1) / 2
    box("F_Floor", (x0, y0, 0), (x1, y1, 2), "Ground_FilmSet")
    # walk-around ring: a centre slot and a circle path around it
    rc = (3000, yc)
    cyl("F_Pedestal_Centre", (rc[0], rc[1], 0), 120, 40, "White")
    target_point("SpriteSlot_Centre", (rc[0], rc[1], 90), 180, ("SpriteSlot",))
    for i in range(16):
        a = math.radians(i * 22.5)
        cyl("F_RingMark_%02d" % i, (rc[0] + 600 * math.cos(a), rc[1] + 600 * math.sin(a), 2), 40, 3, "Breadcrumb")
    # a row of slots at different distances and heights (ground boar size, Director size up high)
    for i, (x, z) in enumerate([(4500, 0), (5200, 0), (5900, 250)]):
        cyl("F_Pedestal_%d" % i, (x, yc, 0), 120, 40 + z, "White")
        target_point("SpriteSlot_%d" % i, (x, yc, 90 + z), 180, ("SpriteSlot",))
    label_text("F  CAMERA-FACING SPRITES", (x0 + 200, yc, 350), 80, yaw=180, color=(200, 150, 255))
    # sprite actors (system 9): centre one spins so front / side / back switching can be watched
    spr = getattr(unreal, "DCSpriteActor", None)
    if spr:
        for label, loc, yaw, spin in [("F_Sprite_Centre", (rc[0], rc[1], 40), 0, 30.0),
                                      ("F_Sprite_0", (4500, yc, 40), 180, 0.0),
                                      ("F_Sprite_1", (5200, yc, 40), 90, 0.0),
                                      ("F_Sprite_2", (5900, yc, 290), -90, 0.0)]:
            a = EAS.spawn_actor_from_class(spr, V(*loc), R(yaw=yaw))
            a.set_editor_property("spin_speed", spin)
            _finish(a, label)


def z_house():
    """Zone H: taisho's house_01 drawing as a film-set flat (painted facade propped up by braces)."""
    CURRENT_FOLDER[0] = "Test/H_House"
    clear_folder("Test/H_House")
    mi = unreal.load_asset("/Game/DirectorsCut/Environment/House01/MI_House01_Facade")
    plane = unreal.load_asset("/Engine/BasicShapes/Plane")
    W, H = 545.0, 550.0                    # drawing is 2683 x 2712 px; door comes out ~2.7 m tall
    fx, fy = -300.0, -2400.0               # facade centre on the ground, facing +X (east)
    face = EAS.spawn_actor_from_object(plane, V(fx, fy, H / 2 + 2), unreal.Rotator(roll=90, pitch=0, yaw=-90))
    face.set_actor_scale3d(V(W / 100.0, H / 100.0, 1.0))
    face.static_mesh_component.set_material(0, mi)
    face.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    _finish(face, "H_House01_Facade")
    # plywood board behind the painting (blocks the player, shows the 'it's a film set' joke from behind)
    box("H_House01_Board", (fx - 14, fy - W * 0.41, 0), (fx - 4, fy + W * 0.41, H * 0.92), "Structure")
    # two diagonal braces down to the ground behind, plus sandbags holding them
    for side in (-1, 1):
        y = fy + side * W * 0.3
        ramp("H_Brace_%d" % (side + 1), (fx - 14, y, H * 0.62), (fx - 330, y, 0), 18, "Trunk", thick=14)
        cbox("H_Sandbag_%d" % (side + 1), (fx - 330, y, 18), (70, 50, 36), "Dark")
    label_text("H  HOUSE 01 (film-set flat)", (fx + 120, fy, H + 80), 60, yaw=0, color=(255, 220, 140))


def z_props():
    """Taisho's grass + hay bale cut-outs (camera-facing set props) dressing the boar arena and the house, plus the janitor Princess."""
    CURRENT_FOLDER[0] = "Test/P_Props"
    clear_folder("Test/P_Props")
    grass = getattr(unreal, "DCProp_Grass01", None)
    hay = getattr(unreal, "DCProp_HayBale01", None)
    princess = getattr(unreal, "DCPrincess", None)
    rng = random.Random(1114)
    if grass:
        # boar arena: tufts inside the fence, kept off the rocks / stump and the west gate path
        x0, y0, x1, y1 = ZONE_B
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        avoid = [(cx - 800, cy + 400, 260), (cx + 600, cy - 450, 280), (cx + 1500, cy + 300, 200), (x0 + 250, 0, 450)]
        n = 0
        for _ in range(200):
            if n >= 16:
                break
            x, y = rng.uniform(x0 + 200, x1 - 200), rng.uniform(y0 + 150, y1 - 150)
            if any((x - ax) ** 2 + (y - ay) ** 2 < r * r for ax, ay, r in avoid):
                continue
            _finish(EAS.spawn_actor_from_class(grass, V(x, y, 2), R(yaw=rng.uniform(0, 360))), "P_Grass_Arena_%02d" % n)
            n += 1
        # around the house flat (zone H)
        for i, (gx, gy) in enumerate([(-80, -2150), (-60, -2650), (150, -2300), (120, -2560), (-120, -1900), (260, -2050)]):
            _finish(EAS.spawn_actor_from_class(grass, V(gx, gy, 0), R()), "P_Grass_House_%d" % i)
    if hay:
        for i, (hx, hy) in enumerate([(250, -2900), (420, -1850)]):
            _finish(EAS.spawn_actor_from_class(hay, V(hx, hy, 0), R()), "P_HayBale_%d" % i)
    if princess:
        _finish(EAS.spawn_actor_from_class(princess, V(450, -2400, 0), R(yaw=180)), "P_Princess_Janitor")
    label_text("P  SET PROPS (Taisho grass + hay) + janitor Princess", (520, -2400, 420), 50, yaw=180, color=(255, 200, 230))


def z_taisho():
    """Zone T: Taisho's film-set models after the Blender clean-up (Scripts/Blender_CleanFilmSetModels.py), in a row
    behind the spawn, with his brick wall picture on a panel behind them. The +X arrow on each disc shows the
    model's own forward axis."""
    CURRENT_FOLDER[0] = "Test/T_Taisho"
    clear_folder("Test/T_Taisho")
    base = "/Game/DirectorsCut/Environment/FilmSet"
    row = [("FilmCamera", "Film camera", 900), ("BoomMic", "Boom mic", 0), ("WoodlandSlide", "Woodland slide", -900)]
    for name, title, y in row:
        sm_asset = unreal.load_asset("%s/SM_DC_%s" % (base, name))
        if not sm_asset:
            continue
        a = EAS.spawn_actor_from_object(sm_asset, V(-900, y, 0), R())
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        _finish(a, "T_" + name)
        cyl("T_Marker_%s" % name, (-900, y, 0), 130, 3, "White")
        box("T_Forward_%s" % name, (-770, y - 6, 0), (-640, y + 6, 5), "Dark")      # points along the model's +X
        label_text(title, (-900, y, 300), 34, yaw=0, color=(255, 230, 150))
    wall = unreal.load_asset("/Game/DirectorsCut/Environment/Taisho/Wall/MI_Taisho_Wall")
    if wall:
        w = EAS.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Cube"), V(-1430, 0, 200), R())
        w.set_actor_scale3d(V(0.1, 4.0, 4.0))          # 4 m x 4 m panel (the picture is square)
        w.static_mesh_component.set_material(0, wall)
        w.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        _finish(w, "T_WallTexture_Panel")
    box("T_HeightPost_180cm", (-620, 420, 0), (-600, 440, 180), "Dark")          # a player-height post for scale
    label_text("T  TAISHO FILM-SET MODELS", (-900, 0, 460), 50, yaw=0, color=(255, 230, 150))


ZONES = {"base": z_base, "light": z_light, "combat": z_combat, "boar": z_boar, "sprites": z_sprites, "house": z_house, "props": z_props, "taisho": z_taisho}


def build_test_map(which="all"):
    ensure_test_map()
    keys = list(ZONES) if which == "all" else [k.strip() for k in which.split(",")]
    for k in keys:
        ZONES[k]()
    LES.save_current_level()
    return _world().get_path_name()
