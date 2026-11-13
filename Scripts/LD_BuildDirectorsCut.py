# Director's Cut - single-map builder: Lvl_DirectorsCut + one streaming sublevel per environment state.
# The player never leaves the map. The Director "rewrites the script" and the programmer swaps sublevels
# (Load/Unload Stream Level) per beat, using the BEATS table below (also in Docs/LevelDesign_DirectorsCut.md).
#
# Run inside the editor:
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\LD_BuildDirectorsCut.py").read())
#   build_all()            # or build("hunt,pg") to rebuild single pieces
#   show_beat(5)           # editor preview of a beat's sublevel set
#
# Re-runnable: every piece clears its own outliner folder (DC/<Piece>) before rebuilding.
import unreal, math, random, os

_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Scripts")
exec(open(os.path.join(_SCRIPTS, "LD_BuildVerticalSlice.py")).read())   # shared greybox helpers + materials

LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP_DIR = "/Game/DirectorsCut/Maps"
PERSISTENT = "Lvl_DirectorsCut"

# ---------------------------------------------------------------- beat -> sublevels contract
LIGHTS = ["DC_Light_Day", "DC_Light_PG", "DC_Light_Storm", "DC_Light_Broken", "DC_Light_Boss"]
SETS = ["DC_Set_Hunt", "DC_Set_NoMeat", "DC_Set_FilmCrew", "DC_Set_PG", "DC_Set_Storm",
        "DC_Exit_A", "DC_Exit_B", "DC_Exit_C", "DC_Set_Broken", "DC_Set_Boss"]
SUBLEVELS = LIGHTS + SETS
INITIAL = ["DC_Light_Day", "DC_Set_Hunt"]          # loaded + visible when the map starts (beat 1)
BEATS = {
    1: ["DC_Light_Day", "DC_Set_Hunt"],
    2: ["DC_Light_Day", "DC_Set_Hunt", "DC_Set_NoMeat"],
    3: ["DC_Light_Day", "DC_Set_Hunt", "DC_Set_FilmCrew"],
    4: ["DC_Light_PG", "DC_Set_Hunt", "DC_Set_PG"],
    5: ["DC_Light_Boss", "DC_Set_Boss"],
}

# ---------------------------------------------------------------- layout (cm, +X toward the Tower)
STAGE_C, STAGE_R = (1000, 0), 1200            # "The Set": central plinth, reused by beats 3 and 7
BOWL = (-6000, -5000, 6000, 5000)              # playable ground
TOWER = (11000, 0)                             # global landmark behind the east backdrop
ARENA_C, ARENA_H = STAGE_C, 40                 # used by the shared landmark searchlight
SPAWN = (-5200, 0)
STALL = (-2500, 2800)
WATCHTOWER = (2800, 3600)
PEN_C, PEN_R = (3000, -3200), 700

MATS.update({
    "MI_LD_Backdrop": (FLAT, {"Base Color": (0.35, 0.60, 0.95, 1)}),
    "MI_LD_Cloud":    (FLAT, {"Base Color": (0.95, 0.95, 1.00, 1)}),
    "MI_LD_PG_Pink":  (FLAT, {"Base Color": (1.00, 0.55, 0.75, 1)}),
    "MI_LD_PG_Mint":  (FLAT, {"Base Color": (0.50, 0.95, 0.75, 1)}),
    "MI_LD_PG_Lilac": (FLAT, {"Base Color": (0.70, 0.60, 1.00, 1)}),
    "MI_LD_PG_Sky":   (FLAT, {"Base Color": (0.55, 0.80, 1.00, 1)}),
    "MI_LD_Mud":      (FLAT, {"Base Color": (0.12, 0.08, 0.04, 1)}),
    "MI_LD_Metal":    (FLAT, {"Base Color": (0.30, 0.30, 0.32, 1)}),
    "MI_LD_Boss":     (FLAT, {"Base Color": (0.35, 0.02, 0.05, 1)}),
    "MI_LD_MissingTex": (GRID, {"SurfaceColor": (1.0, 0.0, 1.0, 1), "TopSurfaceColor": (1.0, 0.0, 1.0, 1)}),
})

# ---------------------------------------------------------------- extra helpers
def sign_text(text, loc, size=120, yaw=180, color=(255, 255, 255), label=None):
    """In-game visible text (signage the player reads), unlike label_text which is editor-only."""
    a = EAS.spawn_actor_from_class(unreal.TextRenderActor, V(*loc), R(yaw=yaw))
    t = a.text_render
    t.set_text(text)
    t.set_world_size(size)
    t.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    t.set_text_render_color(unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    return _finish(a, label or ("SIGN_" + text.split("\n")[0].replace(" ", "_")[:30]))

def pine(n, x, y, h=None, d=None, rnd=random):
    h = h or rnd.uniform(700, 1200)
    d = d or rnd.uniform(300, 480)
    cyl(n + "_Trunk", (x, y, 0), 45, 250, "Trunk")
    sm(n + "_Crown", "cone", (x, y, 200 + h / 2), (d / 100, d / 100, h / 100), m="Foliage")

def trigger(label, loc, extent, tags, yaw=0):
    t = EAS.spawn_actor_from_class(unreal.TriggerBox, V(loc[0], loc[1], loc[2] + extent[2]), R(yaw=yaw))
    t.get_component_by_class(unreal.BoxComponent).set_box_extent(V(*extent), True)
    t.tags = [unreal.Name(x) for x in tags]
    return _finish(t, label)

def ring(label, c, r, seg_len, size_yz, m, z=0, gaps=(), gap_half=12):
    """Wall/ring of boxes around c. gaps = angles (deg) left open."""
    n = max(8, int(2 * math.pi * r / seg_len))
    for i in range(n):
        a = 360.0 * i / n
        if any(abs(((a - g) + 180) % 360 - 180) < gap_half for g in gaps):
            continue
        x, y = c[0] + r * math.cos(math.radians(a)), c[1] + r * math.sin(math.radians(a))
        cbox("%s_%02d" % (label, i), (x, y, z + size_yz[1] / 2), (seg_len * 1.05, size_yz[0], size_yz[1]), m, yaw=a + 90)

def far_from(p, pts, d):
    return all(math.hypot(p[0] - q[0], p[1] - q[1]) >= d for q in pts)

# ---------------------------------------------------------------- map + sublevels
def _world():
    return unreal.EditorLevelLibrary.get_editor_world()

def ensure_map():
    path = MAP_DIR + "/" + PERSISTENT
    cur = _world().get_path_name().split(".")[0]
    if cur != path:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            LES.load_level(path)
        else:
            LES.new_level(path)
            LES.save_current_level()
    return _world()

def streaming(name):
    return unreal.GameplayStatics.get_streaming_level(_world(), MAP_DIR + "/" + name)

def ensure_sublevels():
    for name in SUBLEVELS:
        if streaming(name) is not None:
            continue
        path = MAP_DIR + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorLevelUtils.add_level_to_world(_world(), path, unreal.LevelStreamingDynamic)
        else:
            unreal.EditorLevelUtils.create_new_streaming_level(unreal.LevelStreamingDynamic, path, False)
    for name in SUBLEVELS:
        sl = streaming(name)
        init = name in INITIAL
        for prop in ("initially_loaded", "initially_visible"):
            try:
                sl.set_editor_property(prop, init)
            except Exception as e:
                unreal.log_warning("could not set %s on %s: %s" % (prop, name, e))
    set_current(PERSISTENT)

def set_current(name):
    if name == PERSISTENT:
        LES.set_current_level_by_name("PersistentLevel") or LES.set_current_level_by_name(PERSISTENT)
    else:
        LES.set_current_level_by_name(name)

def show_beat(n):
    """Editor preview: make only the sublevels of beat n visible."""
    on = set(BEATS[n])
    for name in SUBLEVELS:
        sl = streaming(name)
        lvl = sl.get_loaded_level() if sl else None
        if lvl:
            unreal.EditorLevelUtils.set_level_visibility(lvl, name in on, False)
    return sorted(on)

def show_all_sets(visible=True):
    for name in SUBLEVELS:
        sl = streaming(name)
        if sl and sl.get_loaded_level():
            unreal.EditorLevelUtils.set_level_visibility(sl.get_loaded_level(), visible, False)

# ================================================================ PERSISTENT LEVEL
def p_ground():
    CURRENT_FOLDER[0] = "DC/Ground"
    clear_folder("DC/Ground")
    x0, y0, x1, y1 = BOWL
    box("G_Meadow", (x0, y0, -100), (x1, y1, 0), "Ground_Meadow")
    box("G_VillageSquare", (x0, -2600, 0), (-3400, 2600, 3), "Ground_Village")
    box("G_Backstage", (x1, y0, -100), (7200, y1, 0), "Ground_Chasm")         # behind the backdrop
    box("G_TowerHill", (7200, -2500, -100), (14000, 2500, 300), "Ground_Stage")
    cyl("POI_TheSet_Plinth", (STAGE_C[0], STAGE_C[1], 0), STAGE_R * 2, 40, "Ground_FilmSet")
    # dirt paths (read the routes without UI): spawn -> set -> backdrop, plus spurs to the POIs
    for i, (a, b) in enumerate([(SPAWN, (STAGE_C[0] - STAGE_R, 0)), ((STAGE_C[0] + STAGE_R, 0), (5600, 0)),
                                ((-2000, 0), (STALL[0], STALL[1] - 600)), ((1700, 1000), (WATCHTOWER[0], 2300)),
                                ((1700, -1000), (PEN_C[0] - 500, PEN_C[1] + 500)), ((-1000, 0), (-1000, -4400))]):
        L = math.hypot(b[0] - a[0], b[1] - a[1])
        yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
        cbox("Path_%02d" % i, ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, 2), (L + 300, 350, 6), "Path", yaw=yaw)
    ws = _world().get_world_settings()
    ws.set_editor_property("kill_z", -1500.0)
    gm = unreal.load_class(None, "/Game/DirectorsCut/Blueprints/BP_DCGameMode.BP_DCGameMode_C")
    if gm:
        ws.set_editor_property("default_game_mode", gm)

def p_boundary():
    CURRENT_FOLDER[0] = "DC/Boundary"
    clear_folder("DC/Boundary")
    x0, y0, x1, y1 = BOWL
    H = 1500
    box("B_West", (x0 - 100, y0, 0), (x0, y1, H), "Boundary")
    box("B_North", (x0 - 100, y1, 0), (7300, y1 + 100, H), "Boundary")
    box("B_South", (x0 - 100, y0 - 100, 0), (7300, y0, H), "Boundary")
    box("B_BackstageEast", (7200, y0, 0), (7300, y1, 700), "Boundary")         # low: Tower stays visible
    # forest edge band inside the cliffs (dense = "you can't go there")
    rnd = random.Random(11)
    k = 0
    for _ in range(4000):
        if k >= 120:
            break
        side = rnd.choice("NSW")
        if side == "W":
            x, y = rnd.uniform(x0 + 150, x0 + 900), rnd.uniform(y0 + 150, y1 - 150)
        else:
            x = rnd.uniform(x0 + 150, x1 - 400)
            y = rnd.uniform(y1 - 900, y1 - 150) if side == "N" else rnd.uniform(y0 + 150, y0 + 900)
        if math.hypot(x - WATCHTOWER[0], y - WATCHTOWER[1]) < 900: continue
        if math.hypot(x - STALL[0], y - STALL[1]) < 900: continue
        if abs(x - 1500) < 700 and y > 0: continue          # Exit C wall spot
        if abs(x + 1000) < 700 and y < 0: continue          # Exit B wall spot
        if abs(x - 0) < 800 and y > 0: continue             # storm wind-machine spot
        if x < -3300 and abs(y) < 2700: continue            # village
        pine("Edge_Pine_%03d" % k, x, y, rnd=rnd)
        k += 1
    # backstage scaffolding (always there; hidden behind the painted backdrop until beat 6 drops it)
    for i, y in enumerate(range(-4500, 4501, 1500)):
        for j, x in enumerate((6300, 6900)):
            box("Scaffold_Post_%d_%d" % (i, j), (x - 15, y - 15, 0), (x + 15, y + 15, 2400), "Metal")
        for z in (800, 1600, 2400):
            box("Scaffold_Beam_%d_%d" % (i, z), (6285, y - 15, z - 30), (6915, y + 15, z), "Metal")
    for z in (800, 1600, 2400):
        box("Scaffold_Rail_W_%d" % z, (6285, -4515, z - 30), (6315, 4515, z), "Metal")
        box("Scaffold_Rail_E_%d" % z, (6885, -4515, z - 30), (6915, 4515, z), "Metal")

def p_village():
    CURRENT_FOLDER[0] = "DC/Village"
    clear_folder("DC/Village")
    for i, (x, y) in enumerate([(-5100, 1900), (-3900, 2100), (-5100, -1900), (-3900, -2100)]):
        house("House_%d" % i, x, y)
    cyl("Windmill_Tower", (-5300, -3900, 0), 500, 1300, "Structure")
    sm("Windmill_Cap", "cone", (-5300, -3900, 1450), (5.5, 5.5, 3), m="Roof")
    for j in range(4):
        cbox("Windmill_Blade_%d" % j, (-5030, -3900, 1150), (20, 120, 1200), "White", roll=45 + j * 90)
    cyl("Well", (-4300, 800, 0), 220, 100, "Structure")
    cyl("Notice_PostL", (-3700, -650, 0), 20, 250, "Trunk")
    cyl("Notice_PostR", (-3700, -350, 0), 20, 250, "Trunk")
    cbox("POI_NoticeBoard", (-3700, -500, 170), (20, 360, 180), "POI")
    label_text("VILLAGE (spawn)\nNotice board = quest giver", (-3700, -500, 330), 40)

def p_pois():
    CURRENT_FOLDER[0] = "DC/POIs"
    clear_folder("DC/POIs")
    sx, sy = STALL
    box("POI_ButcherStall_Counter", (sx - 400, sy - 200, 0), (sx + 400, sy + 200, 110), "POI")
    for j, (dx, dy) in enumerate([(-380, -180), (380, -180), (-380, 180), (380, 180)]):
        cyl("ButcherStall_Post_%d" % j, (sx + dx, sy + dy, 0), 20, 280, "Trunk")
    box("ButcherStall_Roof", (sx - 450, sy - 250, 280), (sx + 450, sy + 250, 300), "Roof")
    wx, wy = WATCHTOWER
    for j, (dx, dy) in enumerate([(-180, -180), (-180, 180), (180, -180), (180, 180)]):
        cyl("Watchtower_Leg_%d" % j, (wx + dx, wy + dy, 0), 40, 450, "Trunk")
    box("POI_Watchtower_Platform", (wx - 220, wy - 220, 420), (wx + 220, wy + 220, 460), "Structure")
    for j, (mn, mx) in enumerate([((wx - 220, wy + 200, 460), (wx + 220, wy + 220, 560)),
                                  ((wx + 200, wy - 220, 460), (wx + 220, wy + 220, 560)),
                                  ((wx - 220, wy - 220, 460), (wx - 200, wy + 220, 560))]):
        box("Watchtower_Rail_%d" % j, mn, mx, "Trunk")
    ramp("Watchtower_Ramp", (wx, wy - 1300, 0), (wx, wy - 220, 440), 220, "Structure")
    for i, (x, y) in enumerate([(-1500, 4100), (4300, 3900), (4600, -4200), (-2600, -4100), (300, -4300), (5200, 1800)]):
        sm("Burrow_%d" % i, "sphere", (x, y, 0), (3, 3, 1.4), m="Trunk")
    label_text("THE SET (central plinth)\nbeat 3 pedestal | beat 7 arena", (STAGE_C[0], STAGE_C[1], 600), 60)
    label_text("Butcher stall", (sx, sy, 450), 40)
    label_text("Watchtower (overview)", (wx, wy, 700), 40)

def p_landmark():
    stage_landmark()                     # shared: Director's Tower at TOWER, searchlight on The Set
    for a in EAS.get_all_level_actors():  # move it from LD/Landmark to DC/Landmark
        if str(a.get_folder_path()) == "LD/Landmark":
            a.set_folder_path("DC/Landmark")
            if a.get_actor_label().startswith("Tower_"):
                a.set_actor_location(a.get_actor_location() + V(0, 0, 300), False, False)   # sits on the hill

def p_lighting():
    CURRENT_FOLDER[0] = "DC/Sky"
    clear_folder("DC/Sky")
    sky = EAS.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0), R())
    _finish(sky, "SkyAtmosphere")
    sl = EAS.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500), R())
    c = sl.get_component_by_class(unreal.SkyLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("real_time_capture", True)
    _finish(sl, "SkyLight")

def p_gameplay():
    CURRENT_FOLDER[0] = "DC/Gameplay"
    clear_folder("DC/Gameplay")
    for a in EAS.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            EAS.destroy_actor(a)
    ps = EAS.spawn_actor_from_class(unreal.PlayerStart, V(SPAWN[0], SPAWN[1], 100), R(yaw=0))
    ps.tags = [unreal.Name("RED"), unreal.Name("BLUE")]
    _finish(ps, "PlayerStart_Village")
    nav = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(0, 0, 300), R())
    nav.set_actor_scale3d(V(62, 52, 10))
    _finish(nav, "NavMeshBounds_Bowl")
    for i, (x, y) in enumerate([(-1500, 3700), (4300, 3500), (4600, -3800), (-2600, -3700), (300, -3900), (4900, 1800)]):
        target_point("BoarSpawn_%d" % i, (x, y, 50), math.degrees(math.atan2(-y, STAGE_C[0] - x)), ("BoarSpawn",))
    spawner_cls = getattr(unreal, "DCBoarSpawner", None)
    if spawner_cls:
        s = EAS.spawn_actor_from_class(spawner_cls, V(STAGE_C[0], STAGE_C[1], 300), R())
        s.set_editor_property("auto_start", False)
        s.set_editor_property("max_alive", 5)
        _finish(s, "DC_BoarSpawner")
    target_point("Narrator_Focus_TheSet", (STAGE_C[0], STAGE_C[1], 300), 0, ("NarratorFocus",))

# ================================================================ LIGHTING SUBLEVELS
def _light_rig(folder, sun_rot, lux, sun_col, fog_density, fog_col, pp):
    CURRENT_FOLDER[0] = folder
    clear_folder(folder)
    d = EAS.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 2000), sun_rot)
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_mobility(unreal.ComponentMobility.MOVABLE)
    dc.set_intensity(lux)
    dc.set_light_color(unreal.LinearColor(*sun_col))
    try:
        dc.set_atmosphere_sun_light(True)
    except Exception:
        dc.set_editor_property("atmosphere_sun_light", True)
    _finish(d, "Sun")
    f = EAS.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0), R())
    fc = f.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_fog_density(fog_density)
    try:
        fc.set_fog_inscattering_color(unreal.LinearColor(*fog_col))
    except Exception:
        fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*fog_col))
    _finish(f, "Fog")
    p = EAS.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0), R())
    p.set_editor_property("unbound", True)
    s = p.get_editor_property("settings")
    for k, v in pp.items():
        if isinstance(v, (tuple, list)):
            v = unreal.Vector4(*v)
        s.set_editor_property("override_" + k, True)
        s.set_editor_property(k, v)
    p.set_editor_property("settings", s)
    _finish(p, "PostProcess")

def l_day():
    _light_rig("DC/Light_Day", R(pitch=-50, yaw=70), 10.0, (1.0, 0.95, 0.85), 0.01, (0.55, 0.65, 0.8), {})

def l_pg():
    _light_rig("DC/Light_PG", R(pitch=-55, yaw=60), 8.0, (1.0, 0.85, 0.95), 0.02, (1.0, 0.7, 0.85),
               {"color_saturation": (1.35, 1.35, 1.35, 1), "bloom_intensity": 1.5})

def l_storm():
    _light_rig("DC/Light_Storm", R(pitch=-35, yaw=200), 5.0, (0.65, 0.75, 0.95), 0.03, (0.45, 0.5, 0.6),
               {"color_saturation": (0.55, 0.55, 0.55, 1), "color_contrast": (1.15, 1.15, 1.15, 1)})
    target_point("Weather_Rain", (STAGE_C[0], STAGE_C[1], 2500), 0, ("Weather", "Rain"))
    target_point("Weather_Lightning", (6500, 0, 3500), 0, ("Weather", "Lightning"))

def l_broken():
    _light_rig("DC/Light_Broken", R(pitch=-70, yaw=-30), 6.0, (0.85, 0.4, 1.0), 0.03, (0.8, 0.1, 0.8),
               {"scene_fringe_intensity": 4.0, "film_grain_intensity": 0.6, "vignette_intensity": 0.8})

def l_boss():
    _light_rig("DC/Light_Boss", R(pitch=-25, yaw=20), 7.0, (1.0, 0.4, 0.25), 0.02, (0.5, 0.08, 0.06),
               {"color_contrast": (1.2, 1.2, 1.2, 1), "vignette_intensity": 0.6})

# ================================================================ SET SUBLEVELS
def s_hunt():
    """Beats 1-5: the 'normal' fantasy hunt. Painted sky backdrop hides the backstage."""
    CURRENT_FOLDER[0] = "DC/Set_Hunt"
    clear_folder("DC/Set_Hunt")
    x0, y0, x1, y1 = BOWL
    box("Backdrop_Flat", (x1, y0, 0), (x1 + 100, y1, 2600), "Backdrop")
    for i, (y, z, w) in enumerate([(-3200, 1900, 1400), (-600, 2200, 1000), (2100, 1800, 1600), (3900, 2250, 800)]):
        cbox("Backdrop_Cloud_%d" % i, (x1 - 10, y, z), (20, w, w * 0.35), "Cloud")
    cyl("Backdrop_Sun", (x1 - 20, 1000, 2200), 900, 20, "Breadcrumb", roll=90)
    for i in range(5):        # support struts (seen from backstage only)
        y = -4000 + i * 2000
        ramp("Backdrop_Strut_%d" % i, (x1 + 900, y, 0), (x1 + 100, y, 2200), 60, "Trunk", thick=40)
    rnd = random.Random(3)
    k, avoid = 0, [STAGE_C, STALL, WATCHTOWER, PEN_C, SPAWN]
    for _ in range(500):
        if k >= 10:
            break
        x, y = rnd.uniform(-3000, 5000), rnd.uniform(-4000, 4000)
        if not far_from((x, y), avoid, STAGE_R + 500) or abs(y) < 400:
            continue
        cyl("HayBale_%d" % k, (x - 100, y, 75), 150, 200, "Breadcrumb", pitch=90, yaw=rnd.uniform(0, 180))
        avoid.append((x, y))
        k += 1
    # sword rack at the village edge (beat 1 weapon)
    box("SwordRack_Base", (-3500, 500, 0), (-3400, 900, 120), "Trunk")
    for j in range(3):
        cbox("SwordRack_Sword_%d" % j, (-3450, 580 + j * 120, 170), (10, 25, 160), "White")
    target_point("WeaponPickup_Sword", (-3300, 700, 60), 0, ("WeaponPickup", "Sword"))
    label_text("SET: HUNT (beats 1-5)", (0, 0, 900), 70)

def s_nomeat():
    """Beat 2: the meat-drop joke. The stall mocks the player."""
    CURRENT_FOLDER[0] = "DC/Set_NoMeat"
    clear_folder("DC/Set_NoMeat")
    sx, sy = STALL
    cbox("SoldOut_Board", (sx, sy - 215, 190), (10, 600, 120), "Landmark")
    sign_text("SOLD OUT", (sx, sy - 225, 160), 70, yaw=-90, color=(255, 255, 255))
    for j in range(5):
        x = sx - 300 + j * 150
        cyl("MeatHook_%d" % j, (x, sy, 180), 6, 100, "Metal")
        sm("MeatHook_Tip_%d" % j, "sphere", (x, sy, 180), (0.12, 0.12, 0.12), m="Metal")
    cyl("EmptyPlate", (sx, sy - 100, 110), 90, 4, "White")
    spot_light("Spot_EmptyPlate", (sx, sy - 600, 900), look_rot((sx, sy - 600, 900), (sx, sy - 100, 110)),
               (1.0, 0.95, 0.8), 6000, 1800, 14)
    for j, dx in enumerate((-500, 500)):
        cyl("LaughSign_Pole_%d" % j, (sx + dx, sy + 300, 0), 25, 900, "Dark")
    cbox("LaughSign_Board", (sx, sy + 300, 950), (30, 1200, 300), "Dark")
    sign_text("HA HA HA", (sx, sy + 280, 880), 140, yaw=-90, color=(255, 40, 40))
    cbox("Scoreboard", (STAGE_C[0], STAGE_C[1] + STAGE_R + 200, 500), (40, 900, 400), "Dark")
    label_text("SET: NO MEAT (beat 2)", (sx, sy, 1300), 60)

def s_filmcrew():
    """Beat 3: the film crew barges in. Weapon swap pedestal on The Set."""
    CURRENT_FOLDER[0] = "DC/Set_FilmCrew"
    clear_folder("DC/Set_FilmCrew")
    cx, cy = STAGE_C
    cyl("POI_WeaponPedestal", (cx, cy, 40), 160, 110, "POI")
    target_point("WeaponPickup_Shotgun", (cx, cy, 200), 180, ("WeaponPickup", "Shotgun"))
    for j, a in enumerate((30, 150, 210, 330)):
        x, y = cx + (STAGE_R - 150) * math.cos(math.radians(a)), cy + (STAGE_R - 150) * math.sin(math.radians(a))
        cyl("LightStand_Pole_%d" % j, (x, y, 40), 20, 650, "Dark")
        cbox("LightStand_Head_%d" % j, (x, y, 720), (90, 90, 90), "Dark")
        spot_light("LightStand_Spot_%d" % j, (x, y, 720), look_rot((x, y, 720), (cx, cy, 150)),
                   (1.0, 0.95, 0.85), 5000, 2500, 18, shadows=False)
    # director's chair facing the pedestal
    dx, dy = cx + 900, cy - 700
    box("DirChair_Seat", (dx, dy, 100), (dx + 100, dy + 100, 115), "Dark")
    box("DirChair_Back", (dx + 90, dy, 115), (dx + 100, dy + 100, 240), "Roof")
    for j, (x, y) in enumerate([(dx + 5, dy + 5), (dx + 5, dy + 85), (dx + 85, dy + 5), (dx + 85, dy + 85)]):
        box("DirChair_Leg_%d" % j, (x, y, 40), (x + 10, y + 10, 100), "Dark")
    sign_text("DIRECTOR", (dx + 105, dy + 50, 190), 22, yaw=0, color=(255, 255, 255))
    # dolly track arc + camera
    for side, r in (("In", STAGE_R + 250), ("Out", STAGE_R + 400)):
        for i in range(10):
            a = math.radians(-60 + i * 12)
            cbox("Dolly_Rail_%s_%d" % (side, i), (cx + r * math.cos(a), cy + r * math.sin(a), 8), (330, 15, 15), "Metal",
                 yaw=math.degrees(a) + 90)
    ca = math.radians(-20)
    kx, ky = cx + (STAGE_R + 325) * math.cos(ca), cy + (STAGE_R + 325) * math.sin(ca)
    cyl("FilmCamera_Stand", (kx, ky, 0), 40, 160, "Metal")
    cbox("FilmCamera_Body", (kx, ky, 210), (160, 80, 100), "Dark", yaw=math.degrees(ca) + 180)
    # boom mic + clapperboard
    ramp("BoomMic_Pole", (cx - 1500, cy + 700, 150), (cx - 400, cy + 200, 450), 12, "Metal", thick=12)
    cyl("BoomMic_Head", (cx - 400, cy + 200, 400), 40, 60, "Dark")
    cbox("Clapper_Board", (cx - 400, cy - 1500, 260), (30, 600, 400), "Dark")
    cbox("Clapper_Stick", (cx - 400, cy - 1500, 500), (30, 620, 80), "White", roll=-12)
    sign_text("SCENE 3  TAKE 2", (cx - 420, cy - 1500, 230), 70, yaw=180)
    label_text("SET: FILM CREW (beat 3)", (cx, cy, 1100), 60)

def s_pg():
    """Beat 4: rated PG romance. Pastel cardboard world, romantic shed & King Boar clearing."""
    CURRENT_FOLDER[0] = "DC/Set_PG"
    clear_folder("DC/Set_PG")
    pc, pr = PEN_C, PEN_R
    # decorative pastel posts around the romance clearing (the shed + King Boar are spawned by ADCGameMode)
    gate = math.degrees(math.atan2(STAGE_C[1] - pc[1], STAGE_C[0] - pc[0]))
    for i in range(12):
        ang = i * 30
        if abs(((ang - gate) + 180) % 360 - 180) < 45:
            continue
        x, y = pc[0] + pr * math.cos(math.radians(ang)), pc[1] + pr * math.sin(math.radians(ang))
        cyl("Romance_Post_%02d" % i, (x, y, 0), 25, 130, "PG_Mint")
    # rainbow arch over the main path (bead arch, 5 bands)
    for b, m in enumerate(("Landmark", "POI", "Breadcrumb", "PG_Mint", "PG_Sky")):
        r = 1100 - b * 70
        for i in range(25):
            a = math.radians(i * 7.5)
            sm("Rainbow_%d_%02d" % (b, i), "sphere", (-1700, r * math.cos(a), r * math.sin(a)), (0.75, 0.75, 0.75), m=m)
    # cardboard flowers
    rnd = random.Random(5)
    cols = ["PG_Pink", "PG_Lilac", "PG_Sky", "Breadcrumb"]
    k = 0
    for _ in range(800):
        if k >= 30:
            break
        x, y = rnd.uniform(-3200, 5200), rnd.uniform(-4200, 4200)
        if not far_from((x, y), [STAGE_C], STAGE_R + 200) or not far_from((x, y), [PEN_C], PEN_R + 200) or abs(y) < 350:
            continue
        h = rnd.uniform(150, 320)
        cyl("Flower_%02d_Stem" % k, (x, y, 0), 15, h, "PG_Mint")
        cbox("Flower_%02d_Head" % k, (x, y, h + 40), (12, 140, 140), cols[k % 4], yaw=rnd.uniform(0, 180), roll=45)
        k += 1
    # PG romance billboard
    bx, by = 3600, 1400
    for j, dy in enumerate((-500, 500)):
        cyl("PGBoard_Pole_%d" % j, (bx, by + dy, 0), 30, 700, "White")
    cbox("PGBoard", (bx, by, 900), (40, 1400, 700), "PG_Pink")
    label_text("SET: ROMANCE / PG (beat 4)", (bx, by, 1400), 60)

def s_storm():
    """Beat 5: escalation. Wind machine, rain rig, obstacles blocking the routes."""
    CURRENT_FOLDER[0] = "DC/Set_Storm"
    clear_folder("DC/Set_Storm")
    rnd = random.Random(9)
    # fallen trees across the paths
    for i, (x, y, yaw) in enumerate([(-2600, 300, 75), (3200, -200, 100), (-1000, -2500, 10), (2100, 2600, 40)]):
        cyl("FallenTree_%d" % i, (x, y, 40), 90, 1300, "Trunk", pitch=90, yaw=yaw)
    # crate stacks and barricades around The Set (forces new routes)
    for i, a in enumerate((20, 70, 120, 200, 250, 300)):
        r = STAGE_R + 700
        x, y = STAGE_C[0] + r * math.cos(math.radians(a)), STAGE_C[1] + r * math.sin(math.radians(a))
        cbox("Barricade_%d" % i, (x, y, 125), (900, 80, 250), "Structure", yaw=a + 90 + rnd.uniform(-15, 15))
    for i in range(8):
        x, y = rnd.uniform(-3000, 5000), rnd.uniform(-4000, 4000)
        if not far_from((x, y), [STAGE_C], STAGE_R + 300):
            continue
        for j in range(rnd.randint(1, 3)):
            box("Crate_%d_%d" % (i, j), (x + j * 30, y + j * 20, j * 120), (x + 120 + j * 30, y + 120 + j * 20, 120 + j * 120), "Breadcrumb")
    for i in range(6):
        x, y = rnd.uniform(-3000, 5000), rnd.uniform(-4000, 4000)
        cyl("MudPuddle_%d" % i, (x, y, 0), rnd.uniform(500, 900), 5, "Mud")
    # wind machine on the north cliff (a film-set gag: the storm is fake)
    wx, wy = 0, 4600
    cyl("WindMachine_Housing", (wx, wy + 150, 700), 700, 300, "Metal", roll=90)
    for j in range(3):
        cbox("WindMachine_Blade_%d" % j, (wx, wy - 170, 700), (100, 20, 620), "Dark", pitch=j * 60)
    box("WindMachine_Stand", (wx - 150, wy, 0), (wx + 150, wy + 300, 400), "Metal")
    sign_text("WIND FX", (wx, wy - 200, 1150), 80, yaw=-90, color=(255, 255, 0))
    target_point("Weather_Wind", (wx, wy - 300, 700), -90, ("Weather", "Wind"))
    # rain rig: sprinkler pipes across the sky
    for i, y in enumerate((-2500, 0, 2500)):
        box("RainRig_Pipe_%d" % i, (-3000, y - 15, 2300), (5500, y + 15, 2330), "Metal")
    label_text("SET: STORM (beat 5)\nexit hops A -> B -> C", (STAGE_C[0], STAGE_C[1], 1100), 60)

def _exit(letter, loc, yaw):
    folder = "DC/Exit_" + letter
    CURRENT_FOLDER[0] = folder
    clear_folder(folder)
    x, y = loc
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    px, py = -s, c            # along the wall
    cbox("Exit_%s_PostL" % letter, (x + px * 450, y + py * 450, 300), (80, 80, 600), "Landmark", yaw=yaw)
    cbox("Exit_%s_PostR" % letter, (x - px * 450, y - py * 450, 300), (80, 80, 600), "Landmark", yaw=yaw)
    cbox("Exit_%s_Top" % letter, (x, y, 640), (100, 1000, 120), "Landmark", yaw=yaw)
    cbox("Exit_%s_Door" % letter, (x - c * 30, y - s * 30, 290), (20, 820, 580), "Dark", yaw=yaw)
    sign_text("EXIT", (x + c * 55, y + s * 55, 600), 110, yaw=yaw, color=(255, 255, 255))
    point_light("Exit_%s_Glow" % letter, (x + c * 200, y + s * 200, 400), (1.0, 0.1, 0.05), 3000, 1500)
    trigger("Exit_%s_Trigger" % letter, (x + c * 150, y + s * 150, 0), (120, 400, 250), ("Exit", "Exit_" + letter), yaw=yaw)
    label_text("EXIT " + letter, (x + c * 100, y + s * 100, 900), 60)

def s_exit_a(): _exit("A", (5880, 0), 180)          # on the painted backdrop, facing the Set
def s_exit_b(): _exit("B", (-1000, -4880), 90)      # south cliff
def s_exit_c(): _exit("C", (1500, 4880), -90)       # north cliff

def s_broken():
    """Beat 6: the production falls apart. Backdrop gone (Hunt unloaded) -> scaffolding exposed."""
    CURRENT_FOLDER[0] = "DC/Set_Broken"
    clear_folder("DC/Set_Broken")
    rnd = random.Random(13)
    # fallen backdrop panels lying on the ground in front of the scaffolding
    for i, (y, yaw) in enumerate([(-3500, 8), (-800, -5), (2200, 12)]):
        cbox("FallenBackdrop_%d" % i, (5000, y, 40), (2400, 1800, 40), "Backdrop", yaw=yaw, pitch=4)
    # missing-texture chunks floating in the air + jutting ground slabs (cover / ramps)
    for i in range(14):
        x, y = rnd.uniform(-3000, 5500), rnd.uniform(-4200, 4200)
        z = rnd.uniform(300, 1400)
        s = rnd.uniform(150, 450)
        cbox("MissingTex_Float_%02d" % i, (x, y, z), (s, s, s), "MissingTex",
             yaw=rnd.uniform(0, 90), pitch=rnd.uniform(-30, 30), roll=rnd.uniform(-30, 30))
    for i in range(8):
        x, y = rnd.uniform(-2500, 5000), rnd.uniform(-4000, 4000)
        if not far_from((x, y), [STAGE_C], STAGE_R + 200):
            continue
        cbox("Slab_%d" % i, (x, y, 60), (700, 400, 60), "MissingTex", yaw=rnd.uniform(0, 180), pitch=rnd.uniform(10, 22))
    # unrendered "void" patches on the ground (visual only, walkable)
    for i in range(6):
        x, y = rnd.uniform(-3000, 5000), rnd.uniform(-4000, 4000)
        box("VoidPatch_%d" % i, (x, y, 0), (x + rnd.uniform(500, 1200), y + rnd.uniform(500, 1200), 4), "Dark")
    # hay bales frozen mid-fall
    for i in range(5):
        x, y = rnd.uniform(-2000, 4500), rnd.uniform(-3500, 3500)
        cyl("FrozenHay_%d" % i, (x, y, rnd.uniform(400, 1000)), 150, 200, "Breadcrumb", pitch=rnd.uniform(30, 120), yaw=rnd.uniform(0, 180))
    # crashed studio light on The Set
    cyl("CrashedLight_Pole", (STAGE_C[0] - 500, STAGE_C[1] + 600, 60), 30, 900, "Dark", pitch=80, yaw=-30)
    cbox("CrashedLight_Head", (STAGE_C[0] + 300, STAGE_C[1] + 150, 80), (120, 120, 120), "Dark", yaw=20, pitch=30)
    # the Director's giant mouse cursor hovering over the world
    cx, cy, cz = 2200, -1800, 1500
    cbox("Cursor_Body", (cx, cy, cz), (60, 260, 420), "White", roll=-25)
    cbox("Cursor_Tail", (cx, cy + 140, cz - 300), (60, 90, 260), "White", roll=-25)
    sign_text("ERROR: SCENE NOT FOUND", (5600, 0, 1800), 180, yaw=180, color=(255, 0, 255))
    sign_text("LOADING...", (STAGE_C[0], STAGE_C[1], 900), 120, yaw=180, color=(255, 255, 255))
    label_text("SET: BROKEN (beat 6)", (0, 0, 1300), 70)

def s_boss():
    """Beat 7: The Set becomes the boss arena. The Director descends on a crane."""
    CURRENT_FOLDER[0] = "DC/Set_Boss"
    clear_folder("DC/Set_Boss")
    cx, cy = STAGE_C
    ring("Arena_Wall", STAGE_C, 2600, 500, (100, 260), "Boss", gaps=(0, 90, 180, 270), gap_half=10)
    for j in range(4):
        a = math.radians(45 + j * 90)
        cbox("Arena_Cover_%d" % j, (cx + 1600 * math.cos(a), cy + 1600 * math.sin(a), 240), (250, 250, 400), "Structure")
    for j in range(8):
        a = math.radians(22.5 + j * 45)
        x, y = cx + 2000 * math.cos(a), cy + 2000 * math.sin(a)
        cbox("Arena_LowCover_%d" % j, (x, y, 60), (300, 90, 120), "Structure", yaw=math.degrees(a) + 90)
    for j, a in enumerate((60, 180, 300)):
        x, y = cx + 2900 * math.cos(math.radians(a)), cy + 2900 * math.sin(math.radians(a))
        cyl("Arena_LightRig_%d" % j, (x, y, 0), 40, 1400, "Dark")
        cbox("Arena_LightRig_Head_%d" % j, (x, y, 1450), (140, 140, 140), "Dark")
        spot_light("Arena_Spot_%d" % j, (x, y, 1450), look_rot((x, y, 1450), (cx, cy, 40)),
                   (1.0, 0.25, 0.15), 6000, 5000, 30, shadows=False)
    # director's crane: boss perch east of The Set, in front of the Tower sightline
    kx, ky = cx + 3400, cy
    cyl("Crane_Base", (kx, ky, 0), 500, 200, "Metal")
    cyl("Crane_Mast", (kx, ky, 200), 120, 1200, "Metal")
    cbox("Crane_Arm", (kx - 900, ky, 1450), (2200, 120, 120), "Metal", pitch=8)
    box("Crane_Platform", (kx - 2100, ky - 200, 1250), (kx - 1700, ky + 200, 1290), "Boss")
    target_point("Boss_DirectorSpawn", (kx - 1900, ky, 1400), 180, ("Boss",))
    sign_text("FINAL CUT", (kx - 1900, ky, 1700), 120, yaw=180, color=(255, 60, 40))
    label_text("SET: BOSS ARENA (beat 7)", (cx, cy, 1300), 70)

# ================================================================ build
PIECES = [
    # name, sublevel (None = persistent), function
    ("ground", None, p_ground), ("boundary", None, p_boundary), ("village", None, p_village),
    ("pois", None, p_pois), ("landmark", None, p_landmark), ("sky", None, p_lighting), ("gameplay", None, p_gameplay),
    ("light_day", "DC_Light_Day", l_day), ("light_pg", "DC_Light_PG", l_pg), ("light_storm", "DC_Light_Storm", l_storm),
    ("light_broken", "DC_Light_Broken", l_broken), ("light_boss", "DC_Light_Boss", l_boss),
    ("hunt", "DC_Set_Hunt", s_hunt), ("nomeat", "DC_Set_NoMeat", s_nomeat), ("filmcrew", "DC_Set_FilmCrew", s_filmcrew),
    ("pg", "DC_Set_PG", s_pg), ("storm", "DC_Set_Storm", s_storm),
    ("exit_a", "DC_Exit_A", s_exit_a), ("exit_b", "DC_Exit_B", s_exit_b), ("exit_c", "DC_Exit_C", s_exit_c),
    ("broken", "DC_Set_Broken", s_broken), ("boss", "DC_Set_Boss", s_boss),
]

def build(which):
    ensure_materials()
    names = [n for n, _, _ in PIECES] if which == "all" else which.split(",")
    done = []
    show_all_sets(True)
    for n, lvl, f in PIECES:
        if n not in names:
            continue
        set_current(lvl or PERSISTENT)
        f()
        done.append(n)
    set_current(PERSISTENT)
    return done

def build_all():
    ensure_map()
    ensure_sublevels()
    done = build("all")
    LES.save_all_dirty_levels()
    return done
