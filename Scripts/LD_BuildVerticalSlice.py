# Director's Cut - Vertical Slice greybox builder.
# Re-runnable: each stage deletes the actors in its own outliner folder (LD/<Stage>) before rebuilding.
# Run inside the editor:  exec(open(r"<this file>").read()); build("all")
# Layout reference: Docs/LevelDesign_VerticalSlice.md
import unreal, math, random

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MEL = unreal.MaterialEditingLibrary
MAT_DIR = "/Game/VerticalSlice/Materials"

MESH = {
    "cube": "/Game/LevelPrototyping/Meshes/SM_Cube",            # pivot at min corner, 100cm
    "ccube": "/Game/LevelPrototyping/Meshes/SM_ChamferCube",    # pivot centre, 100cm
    "cyl": "/Game/LevelPrototyping/Meshes/SM_Cylinder",         # pivot bottom centre, d100 h100
    "cone": "/Engine/BasicShapes/Cone",                         # pivot centre, 100cm
    "sphere": "/Engine/BasicShapes/Sphere",                     # pivot centre, d100
}
_mesh_cache = {}
def mesh(k):
    if k not in _mesh_cache:
        _mesh_cache[k] = unreal.load_asset(MESH[k])
    return _mesh_cache[k]

# ---------------------------------------------------------------- materials
FLAT = "/Game/LevelPrototyping/Materials/M_FlatCol"
GRID = "/Game/LevelPrototyping/Materials/M_PrototypeGrid"
MATS = {
    # name: (parent, {param: rgba})
    "MI_LD_Ground_Village": (GRID, {"SurfaceColor": (0.45, 0.42, 0.38, 1), "TopSurfaceColor": (0.45, 0.42, 0.38, 1)}),
    "MI_LD_Ground_Meadow":  (GRID, {"SurfaceColor": (0.30, 0.42, 0.22, 1), "TopSurfaceColor": (0.30, 0.42, 0.22, 1)}),
    "MI_LD_Ground_FilmSet": (GRID, {"SurfaceColor": (0.36, 0.30, 0.45, 1), "TopSurfaceColor": (0.36, 0.30, 0.45, 1)}),
    "MI_LD_Ground_Forest":  (GRID, {"SurfaceColor": (0.18, 0.30, 0.18, 1), "TopSurfaceColor": (0.18, 0.30, 0.18, 1)}),
    "MI_LD_Ground_Chasm":   (GRID, {"SurfaceColor": (0.32, 0.36, 0.42, 1), "TopSurfaceColor": (0.32, 0.36, 0.42, 1)}),
    "MI_LD_Ground_Stage":   (GRID, {"SurfaceColor": (0.45, 0.25, 0.25, 1), "TopSurfaceColor": (0.45, 0.25, 0.25, 1)}),
    "MI_LD_Boundary":   (FLAT, {"Base Color": (0.05, 0.05, 0.06, 1)}),
    "MI_LD_Path":       (FLAT, {"Base Color": (0.35, 0.20, 0.08, 1)}),
    "MI_LD_Breadcrumb": (FLAT, {"Base Color": (1.00, 0.80, 0.10, 1)}),
    "MI_LD_POI":        (FLAT, {"Base Color": (1.00, 0.40, 0.00, 1)}),
    "MI_LD_Checkpoint": (FLAT, {"Base Color": (0.00, 0.85, 1.00, 1)}),
    "MI_LD_Landmark":   (FLAT, {"Base Color": (0.80, 0.02, 0.02, 1)}),
    "MI_LD_Foliage":    (FLAT, {"Base Color": (0.05, 0.25, 0.06, 1)}),
    "MI_LD_Trunk":      (FLAT, {"Base Color": (0.20, 0.11, 0.05, 1)}),
    "MI_LD_Structure":  (FLAT, {"Base Color": (0.60, 0.52, 0.40, 1)}),
    "MI_LD_Roof":       (FLAT, {"Base Color": (0.45, 0.12, 0.08, 1)}),
    "MI_LD_Dark":       (FLAT, {"Base Color": (0.02, 0.02, 0.02, 1)}),
    "MI_LD_White":      (FLAT, {"Base Color": (0.90, 0.90, 0.90, 1)}),
}
_mat_cache = {}
def ensure_materials():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, (parent, params) in MATS.items():
        path = MAT_DIR + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            mi = tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, unreal.load_asset(parent))
        for p, c in params.items():
            MEL.set_material_instance_vector_parameter_value(mi, p, unreal.LinearColor(*c))
        MEL.update_material_instance(mi)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
        _mat_cache[name] = mi

def mat(k):
    k = "MI_LD_" + k
    if k not in _mat_cache:
        _mat_cache[k] = unreal.load_asset(MAT_DIR + "/" + k)
    return _mat_cache[k]

# ---------------------------------------------------------------- spawn helpers
V = unreal.Vector
R = lambda pitch=0, yaw=0, roll=0: unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)
CURRENT_FOLDER = ["LD"]

def _finish(a, label, m=None):
    a.set_actor_label(label)
    a.set_folder_path(CURRENT_FOLDER[0])
    if m is not None:
        a.static_mesh_component.set_material(0, mat(m))
    return a

def sm(label, key, loc, scale, rot=None, m=None):
    a = EAS.spawn_actor_from_object(mesh(key), V(*loc), rot or R())
    a.set_actor_scale3d(V(*scale))
    return _finish(a, label, m)

def box(label, mn, mx, m):
    """Axis-aligned box from min/max corners (cm)."""
    size = [mx[i] - mn[i] for i in range(3)]
    return sm(label, "cube", mn, [s / 100.0 for s in size], m=m)

def cbox(label, center, size, m, yaw=0, pitch=0, roll=0):
    """Centre-pivot chamfer box, rotatable."""
    return sm(label, "ccube", center, [s / 100.0 for s in size], R(pitch, yaw, roll), m)

def cyl(label, base, d, h, m, pitch=0, yaw=0, roll=0):
    return sm(label, "cyl", base, [d / 100.0, d / 100.0, h / 100.0], R(pitch, yaw, roll), m)

def ramp(label, p0, p1, width, m, thick=60):
    dx, dy, dz = p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]
    run = math.hypot(dx, dy)
    length = math.hypot(run, dz)
    pitch = math.degrees(math.atan2(dz, run))
    yaw = math.degrees(math.atan2(dy, dx))
    c = [(p0[i] + p1[i]) / 2.0 for i in range(3)]
    c[2] -= thick / 2.0 / max(math.cos(math.radians(pitch)), 0.1)
    return cbox(label, c, [length, width, thick], m, yaw=yaw, pitch=pitch)

def point_light(label, loc, color, intensity_cd, radius, shadows=False):
    a = EAS.spawn_actor_from_class(unreal.PointLight, V(*loc), R())
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(intensity_cd)
    c.set_light_color(unreal.LinearColor(*color))
    c.set_attenuation_radius(radius)
    c.set_cast_shadows(shadows)
    return _finish(a, label)

def spot_light(label, loc, rot, color, intensity_cd, radius, outer=30, shadows=True):
    a = EAS.spawn_actor_from_class(unreal.SpotLight, V(*loc), rot)
    c = a.get_component_by_class(unreal.SpotLightComponent)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(intensity_cd)
    c.set_light_color(unreal.LinearColor(*color))
    c.set_attenuation_radius(radius)
    c.set_outer_cone_angle(outer)
    c.set_inner_cone_angle(outer * 0.6)
    c.set_cast_shadows(shadows)
    return _finish(a, label)

def look_rot(src, dst):
    dx, dy, dz = dst[0] - src[0], dst[1] - src[1], dst[2] - src[2]
    return R(pitch=math.degrees(math.atan2(dz, math.hypot(dx, dy))), yaw=math.degrees(math.atan2(dy, dx)))

def label_text(text, loc, size=120, yaw=180, color=(255, 255, 255)):
    """Editor-only greybox annotation (hidden in game)."""
    a = EAS.spawn_actor_from_class(unreal.TextRenderActor, V(*loc), R(yaw=yaw))
    t = a.text_render
    t.set_text(text)
    t.set_world_size(size)
    t.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    t.set_text_render_color(unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    a.set_actor_hidden_in_game(True)
    return _finish(a, "LBL_" + text.split("\n")[0].replace(" ", "_")[:40])

def target_point(label, loc, yaw=0, tags=()):
    a = EAS.spawn_actor_from_class(unreal.TargetPoint, V(*loc), R(yaw=yaw))
    a.tags = [unreal.Name(t) for t in tags]
    return _finish(a, label)

def checkpoint(index, loc, yaw=0, extent=(150, 700, 250), note=""):
    """Trigger volume + respawn point + cyan banner. Tags: Checkpoint, CP_<index>."""
    cid = "CP_%02d" % index
    t = EAS.spawn_actor_from_class(unreal.TriggerBox, V(loc[0], loc[1], loc[2] + extent[2]), R(yaw=yaw))
    t.get_component_by_class(unreal.BoxComponent).set_box_extent(V(*extent), True)
    t.tags = [unreal.Name("Checkpoint"), unreal.Name(cid)]
    _finish(t, cid + "_Trigger")
    target_point(cid + "_Respawn", (loc[0], loc[1], loc[2] + 100), yaw, ("Checkpoint", cid, "Respawn"))
    # banner at the side of the path
    side = 1 if index % 2 == 0 else -1
    by = loc[1] + side * (extent[1] * 0.6)
    cyl(cid + "_BannerPole", (loc[0], by, loc[2]), 20, 400, "Checkpoint")
    cbox(cid + "_Banner", (loc[0], by + side * 60, loc[2] + 320), (10, 120, 160), "Checkpoint")
    label_text(cid + ("\n" + note if note else ""), (loc[0], by, loc[2] + 480), 60, yaw=yaw + 180, color=(0, 220, 255))

def clear_folder(folder):
    for a in EAS.get_all_level_actors():
        fp = str(a.get_folder_path())
        if fp == folder or fp.startswith(folder + "/"):
            EAS.destroy_actor(a)

# ---------------------------------------------------------------- layout data
# Critical path waypoints (x, y). Zones described in Docs/LevelDesign_VerticalSlice.md
PATH = [(-2400, 0), (500, 0), (3000, 0), (6000, 0), (8500, -1500), (10000, -1500), (11000, -1500),
        (12500, 1000), (14500, -300), (16000, 1500), (17600, 500), (18000, 500)]
PATH2 = [(20000, 500), (21500, 500)]   # after the bridge
TOWER = (29500, 500)
ARENA_C, ARENA_R, ARENA_H = (25500, 500), 2500, 600

def _seg_dist(p, a, b):
    ax, ay = a; bx, by = b; px, py = p
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    t = 0 if L2 == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))

def dist_to_path(p):
    d = 1e9
    for poly in (PATH, PATH2):
        for i in range(len(poly) - 1):
            d = min(d, _seg_dist(p, poly[i], poly[i + 1]))
    return d

# ---------------------------------------------------------------- stages
def stage_ground():
    CURRENT_FOLDER[0] = "LD/Ground"
    clear_folder("LD/Ground")
    for a in EAS.get_all_level_actors():   # template floor overlaps zone A (z-fighting)
        if a.get_actor_label() == "Floor":
            EAS.destroy_actor(a)
    box("G_A_Village", (-3000, -3000, -100), (3000, 3000, 0), "Ground_Village")
    box("G_B_Meadow", (3000, -4000, -100), (9000, 4000, 0), "Ground_Meadow")
    box("G_C_FilmSet", (9000, -2200, -100), (11000, -800, 0), "Ground_FilmSet")
    box("G_D_Forest", (11000, -3000, -100), (17000, 3500, 0), "Ground_Forest")
    box("G_E_Ledge1", (17000, -2500, -100), (18000, 3000, 0), "Ground_Chasm")
    box("G_E_ChasmFloor", (18000, -2500, -3100), (20000, 3000, -3000), "Boundary")
    box("G_E_Ledge2", (20000, -2500, -100), (21500, 3000, 0), "Ground_Chasm")
    box("G_F_Stage", (21500, -4500, -100), (32000, 5500, 0), "Ground_Stage")
    # Kill Z below the chasm
    ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings()
    ws.set_editor_property("kill_z", -1500.0)

def stage_boundary():
    CURRENT_FOLDER[0] = "LD/Boundary"
    clear_folder("LD/Boundary")
    H, PAL = 1200, 600
    w = lambda n, mn, mx: box(n, mn, mx, "Boundary")
    # Village
    w("B_A_West", (-3100, -3000, 0), (-3000, 3000, H))
    w("B_A_North", (-3000, 3000, 0), (3000, 3100, H))
    w("B_A_South", (-3000, -3100, 0), (3000, -3000, H))
    # Village palisade / meadow west face (gate gap y -450..450)
    box("B_AB_PalisadeN", (2900, 450, 0), (3000, 4000, PAL), "Structure")
    box("B_AB_PalisadeS", (2900, -4000, 0), (3000, -450, PAL), "Structure")
    # Meadow
    w("B_B_North", (3000, 4000, 0), (9000, 4100, H))
    w("B_B_South", (3000, -4100, 0), (9000, -4000, H))
    w("B_B_EastN", (9000, -800, 0), (9100, 4100, H))
    w("B_B_EastS", (9000, -4100, 0), (9100, -2200, H))
    # Film set canyon
    w("B_C_North", (9000, -800, 0), (11000, -600, H))
    w("B_C_South", (9000, -2400, 0), (11000, -2200, H))
    # Forest
    w("B_D_WestN", (11000, -800, 0), (11100, 3600, H))
    w("B_D_WestS", (11000, -3100, 0), (11100, -2200, H))
    w("B_D_North", (11000, 3500, 0), (17000, 3600, H))
    w("B_D_South", (11000, -3100, 0), (17000, -3000, H))
    w("B_D_EastN", (17000, 3000, 0), (17100, 3600, H))
    w("B_D_EastS", (17000, -3100, 0), (17100, -2500, H))
    # Chasm cliffs (run full depth)
    w("B_E_North", (17000, 3000, -3100), (21500, 3100, H))
    w("B_E_South", (17000, -2600, -3100), (21500, -2500, H))
    # Stage
    w("B_F_WestN", (21500, 3000, 0), (21600, 5600, H))
    w("B_F_WestS", (21500, -4600, 0), (21600, -2500, H))
    w("B_F_North", (21500, 5500, 0), (32000, 5600, H * 2))
    w("B_F_South", (21500, -4600, 0), (32000, -4500, H * 2))
    w("B_F_East", (32000, -4600, 0), (32100, 5600, H * 2))

def stage_path():
    """Critical path strip + lantern breadcrumbs (warm light = go)."""
    CURRENT_FOLDER[0] = "LD/Breadcrumbs"
    clear_folder("LD/Breadcrumbs")
    n = 0
    for poly in (PATH, PATH2):
        for i in range(len(poly) - 1):
            (ax, ay), (bx, by) = poly[i], poly[i + 1]
            L = math.hypot(bx - ax, by - ay)
            yaw = math.degrees(math.atan2(by - ay, bx - ax))
            cbox("Path_%02d" % n, ((ax + bx) / 2, (ay + by) / 2, 1), (L + 300, 350, 6), "Path", yaw=yaw)
            n += 1
    # lanterns every ~10m along the main path, alternating sides
    pts, step, side, k = [], 1000, 1, 0
    for poly in (PATH, PATH2):
        carry = 900 if poly is PATH else 400   # keep the spawn view clear
        for i in range(len(poly) - 1):
            (ax, ay), (bx, by) = poly[i], poly[i + 1]
            L = math.hypot(bx - ax, by - ay)
            ux, uy = (bx - ax) / L, (by - ay) / L
            t = carry
            while t < L:
                pts.append((ax + ux * t, ay + uy * t, -uy, ux))
                t += step
            carry = t - L
    for (x, y, nx, ny) in pts:
        if 18000 <= x <= 20000:      # no lanterns on the bridge (the bridge itself reads as path)
            continue
        px, py = x + nx * 280 * side, y + ny * 280 * side
        cyl("Lantern_%02d_Post" % k, (px, py, 0), 20, 260, "Trunk")
        cbox("Lantern_%02d_Lamp" % k, (px, py, 280), (45, 45, 55), "Breadcrumb")
        point_light("Lantern_%02d_Light" % k, (px, py, 290), (1.0, 0.72, 0.38), 60, 900)
        side, k = -side, k + 1

def house(n, cx, cy, w=600, d=600, h=450, yaw=0):
    box(n + "_Body", (cx - w / 2, cy - d / 2, 0), (cx + w / 2, cy + d / 2, h), "Structure")
    s = w * 0.72
    cbox(n + "_Roof", (cx, cy, h), (d + 80, s, s), "Roof", roll=45, yaw=yaw)

def stage_village():
    CURRENT_FOLDER[0] = "LD/A_Village"
    clear_folder("LD/A_Village")
    for i, (x, y) in enumerate([(-1800, 1700), (0, 1900), (1800, 1700), (-1800, -1700), (-200, -1900)]):
        house("House_%d" % i, x, y)
    # Windmill (local landmark, south-east of square)
    cyl("Windmill_Tower", (1800, -2000, 0), 500, 1300, "Structure")
    sm("Windmill_Cap", "cone", (1800, -2000, 1450), (5.5, 5.5, 3), m="Roof")
    for j in range(4):
        cbox("Windmill_Blade_%d" % j, (1530, -2000, 1150), (20, 120, 1200), "White", roll=45 + j * 90)
    # Square: well + notice board (quest POI)
    cyl("Well", (500, 800, 0), 220, 100, "Structure")
    cyl("Notice_PostL", (900, -650, 0), 20, 250, "Trunk")
    cyl("Notice_PostR", (900, -350, 0), 20, 250, "Trunk")
    cbox("POI_NoticeBoard", (900, -500, 170), (20, 360, 180), "POI")
    # Gate (threshold + frames the Tower sightline)
    cbox("Gate_TowerN", (2950, 600, 450), (300, 300, 900), "Structure")
    cbox("Gate_TowerS", (2950, -600, 450), (300, 300, 900), "Structure")
    cbox("Gate_Lintel", (2950, 0, 850), (300, 1500, 150), "Roof")
    checkpoint(0, (-2400, 0, 0), 0, note="Spawn / Village")
    checkpoint(1, (3300, 0, 0), 0, extent=(150, 500, 250), note="Meadow - Beat 1-2")
    label_text("A  VILLAGE\nQuest: hunt the boars", (900, -500, 330), 50)

def stage_meadow():
    CURRENT_FOLDER[0] = "LD/B_Meadow"
    clear_folder("LD/B_Meadow")
    for i, (x, y) in enumerate([(4500, 1200), (4700, -1300), (5600, 700), (6400, -900), (7200, 1600),
                                (7400, -2200), (6800, 2700), (5200, -2700), (7800, 300)]):
        cyl("HayBale_%d" % i, (x - 100, y, 75), 150, 200, "Breadcrumb", pitch=90)
    # Butcher stall POI (meat objective / joke)
    box("POI_ButcherStall_Counter", (3600, 1500, 0), (4000, 2300, 110), "POI")
    for j, (x, y) in enumerate([(3620, 1520), (3620, 2280), (3980, 1520), (3980, 2280)]):
        cyl("ButcherStall_Post_%d" % j, (x, y, 0), 20, 280, "Trunk")
    box("ButcherStall_Roof", (3550, 1450, 280), (4050, 2350, 300), "Roof")
    label_text("POI Butcher Stall\n'Collect 3 boar meat'", (3800, 1900, 380), 40)
    # Watchtower (vantage point / player insight over the meadow)
    for j, (dx, dy) in enumerate([(-180, -180), (-180, 180), (180, -180), (180, 180)]):
        cyl("Watchtower_Leg_%d" % j, (6000 + dx, 2600 + dy, 0), 40, 450, "Trunk")
    box("POI_Watchtower_Platform", (5780, 2380, 420), (6220, 2820, 460), "Structure")
    for j, (mn, mx) in enumerate([((5780, 2800, 460), (6220, 2820, 560)), ((6200, 2380, 460), (6220, 2820, 560)),
                                  ((5780, 2380, 460), (5800, 2820, 560))]):
        box("Watchtower_Rail_%d" % j, mn, mx, "Trunk")
    ramp("Watchtower_Ramp", (6000, 1100, 0), (6000, 2380, 440), 220, "Structure")
    label_text("POI Watchtower\n(overview)", (6000, 2600, 650), 40)
    # Boar burrows + spawn points
    for i, (x, y) in enumerate([(8200, 2800), (8000, 1000), (7600, -3300), (5400, 3200)]):
        sm("Burrow_%d" % i, "sphere", (x, y, 0), (3, 3, 1.4), m="Trunk")
        target_point("BoarSpawn_B_%d" % i, (x - 250, y, 50), 180, ("BoarSpawn", "Zone_B"))
    label_text("B  BOAR MEADOW\nBeat 1 Hunt  |  Beat 2 No meat drop", (4200, 0, 700), 70)

def stage_filmset():
    CURRENT_FOLDER[0] = "LD/C_FilmSet"
    clear_folder("LD/C_FilmSet")
    checkpoint(2, (9200, -1500, 0), 0, extent=(150, 650, 250), note="Film Set - Beat 3")
    cyl("POI_WeaponPedestal", (10000, -1500, 0), 140, 100, "POI")
    spot_light("Film_Spot_Pedestal", (10000, -1500, 1000), R(pitch=-90), (1.0, 0.95, 0.85), 4000, 1600, 20)
    # Director's chair
    box("DirChair_Seat", (10350, -1150, 60), (10450, -1050, 75), "Dark")
    box("DirChair_Back", (10440, -1150, 75), (10450, -1050, 190), "Roof")
    for j, (x, y) in enumerate([(10355, -1145), (10355, -1065), (10435, -1145), (10435, -1065)]):
        box("DirChair_Leg_%d" % j, (x, y, 0), (x + 10, y + 10, 60), "Dark")
    # Giant clapperboard on the canyon wall + stage lights
    cbox("Clapper_Board", (10000, -830, 650), (40, 700, 450), "Dark", yaw=0)
    cbox("Clapper_Stick", (10000, -830, 930), (40, 720, 90), "White", roll=-12)
    for j, (x, y) in enumerate([(9300, -2050), (10700, -2050)]):
        cyl("StageLight_Pole_%d" % j, (x, y, 0), 25, 600, "Dark")
        cbox("StageLight_Head_%d" % j, (x, y, 620), (80, 80, 80), "Dark")
    label_text("C  FILM SET\nBeat 3: Sword -> Shotgun", (10000, -1500, 500), 60)

def stage_forest():
    CURRENT_FOLDER[0] = "LD/D_Forest"
    clear_folder("LD/D_Forest")
    checkpoint(3, (11300, -1500, 0), 0, extent=(150, 650, 250), note="Forest - Beat 4 (PG)")
    # Boar pen POI (non-lethal objective: shoo boars inside), opening faces the path (+Y)
    pc, pr = (14500, -1900), 700
    for i in range(18):
        ang = i * 20
        if 60 <= ang <= 120:    # gate gap toward the path
            continue
        x, y = pc[0] + pr * math.cos(math.radians(ang)), pc[1] + pr * math.sin(math.radians(ang))
        cyl("Pen_Post_%02d" % i, (x, y, 0), 25, 130, "Trunk")
        cbox("Pen_Rail_%02d" % i, (x, y, 100), (30, 260, 20), "Trunk", yaw=ang)
    label_text("POI Boar Pen\n'Shoo them in - no killing!'", (pc[0], pc[1], 450), 50)
    for i, (x, y) in enumerate([(12200, 2600), (13800, 1600), (15800, -2400), (16400, 2900)]):
        target_point("BoarSpawn_D_%d" % i, (x, y, 50), 0, ("BoarSpawn", "Zone_D"))
    # Pine scatter: dense walls, path + pen clearing + tower-reveal gap kept clear
    rnd = random.Random(7)
    k, tries = 0, 0
    while k < 140 and tries < 5000:
        tries += 1
        x, y = rnd.uniform(11250, 16900), rnd.uniform(-2900, 3400)
        if dist_to_path((x, y)) < 650: continue
        if math.hypot(x - pc[0], y - pc[1]) < pr + 350: continue
        if x > 15700 and 500 < y < 2600 and x > 16200: continue   # reveal gap toward the Tower
        h, d = rnd.uniform(700, 1200), rnd.uniform(300, 480)
        cyl("Pine_%03d_Trunk" % k, (x, y, 0), 45, 250, "Trunk")
        sm("Pine_%03d_Crown" % k, "cone", (x, y, 200 + h / 2), (d / 100, d / 100, h / 100), m="Foliage")
        k += 1
    label_text("D  FOREST\nBeat 4: PG rating - non-lethal", (11800, -1500, 500), 60)

def stage_chasm():
    CURRENT_FOLDER[0] = "LD/E_Chasm"
    clear_folder("LD/E_Chasm")
    checkpoint(4, (17400, 500, 0), 0, extent=(150, 700, 250), note="Chasm - Beat 5")
    box("POI_Bridge_Deck", (18000, 300, -40), (20000, 700, 0), "Structure")
    for j, x in enumerate(range(18000, 20001, 400)):
        for s, y in ((0, 290), (1, 690)):
            cyl("Bridge_Post_%d_%d" % (j, s), (x, y, -40), 20, 120, "Trunk")
    for j, (x, y) in enumerate([(18600, 420), (19300, 560)]):
        box("Bridge_Crate_%d" % j, (x, y, 0), (x + 120, y + 120, 120), "Breadcrumb")
    # EXIT arch (the Director moves it) + alternates
    cbox("POI_ExitArch_L", (20700, 0, 300), (80, 80, 600), "Landmark")
    cbox("POI_ExitArch_R", (20700, 1000, 300), (80, 80, 600), "Landmark")
    cbox("POI_ExitArch_Top", (20700, 500, 640), (100, 1100, 120), "Landmark")
    label_text("EXIT", (20640, 500, 620), 110, color=(255, 255, 255))
    target_point("ExitAlt_1", (21000, -1800, 50), 0, ("ExitAlt",))
    target_point("ExitAlt_2", (21000, 2400, 50), 0, ("ExitAlt",))
    target_point("Escalation_Weather", (19000, 500, 800), 0, ("Escalation",))
    label_text("E  CHASM\nBeat 5: weather / obstacles / exit moves", (17500, 500, 600), 60)

def stage_arena():
    CURRENT_FOLDER[0] = "LD/F_Stage"
    clear_folder("LD/F_Stage")
    checkpoint(5, (21200, 500, 0), 0, extent=(150, 700, 250), note="Boss - Beat 6-7")
    ramp("Arena_Ramp", (21500, 500, 0), (23050, 500, ARENA_H), 1000, "Structure")
    cyl("POI_Arena_Floor", (ARENA_C[0], ARENA_C[1], 0), ARENA_R * 2, ARENA_H, "Structure")
    for j in range(4):
        a = math.radians(45 + j * 90)
        cbox("Arena_Cover_%d" % j, (ARENA_C[0] + 1300 * math.cos(a), ARENA_C[1] + 1300 * math.sin(a), ARENA_H + 200),
             (220, 220, 400), "Structure")
    for j in range(4):
        a = math.radians(j * 90)
        x, y = ARENA_C[0] + 2300 * math.cos(a), ARENA_C[1] + 2300 * math.sin(a)
        if j == 2:      # keep the ramp entrance clear
            continue
        cyl("Arena_LightRig_%d" % j, (x, y, ARENA_H), 40, 1200, "Dark")
        cbox("Arena_LightRig_Head_%d" % j, (x, y, ARENA_H + 1250), (120, 120, 120), "Dark")
        spot_light("Arena_Spot_%d" % j, (x, y, ARENA_H + 1250), look_rot((x, y, ARENA_H + 1250), (ARENA_C[0], ARENA_C[1], ARENA_H)),
                   (1.0, 0.9, 0.8), 3000, 4500, 28, shadows=False)
    target_point("Boss_DirectorSpawn", (ARENA_C[0] + 1500, ARENA_C[1], ARENA_H + 900), 180, ("Boss",))
    label_text("F  SOUND STAGE ARENA\nBeat 6 Breaking point | Beat 7 Boss", (22300, 500, 900), 70)

def stage_landmark():
    """Director's Tower: global landmark visible from spawn."""
    CURRENT_FOLDER[0] = "LD/Landmark"
    clear_folder("LD/Landmark")
    tx, ty = TOWER
    cyl("Tower_Shaft", (tx, ty, 0), 1200, 7000, "Dark")
    cyl("Tower_Base", (tx, ty, 0), 2000, 600, "Structure")
    cbox("Tower_CameraBody", (tx, ty, 7450), (1800, 1000, 900), "Landmark")
    cyl("Tower_CameraLens", (tx - 900, ty, 7450), 700, 700, "Dark", pitch=90)
    cyl("Tower_LensRim", (tx - 1500, ty, 7450), 820, 120, "White", pitch=90)
    for j, x in enumerate([tx - 450, tx + 450]):
        cyl("Tower_Reel_%d" % j, (x, ty - 75, 8350), 900, 150, "Dark", roll=90)
    sm("Tower_TallyLight", "sphere", (tx + 400, ty, 8000), (2.5, 2.5, 2.5), m="Landmark")
    point_light("Tower_TallyGlow", (tx + 400, ty, 8150), (1.0, 0.05, 0.02), 200000, 6000)
    spot_light("Tower_Searchlight", (tx - 1600, ty, 7450), look_rot((tx - 1600, ty, 7450), (ARENA_C[0], ARENA_C[1], ARENA_H)),
               (1.0, 0.95, 0.9), 60000, 9000, 18, shadows=False)
    label_text("LANDMARK: Director's Tower", (tx - 1200, ty, 9200), 250)

def stage_misc():
    CURRENT_FOLDER[0] = "LD/Gameplay"
    clear_folder("LD/Gameplay")
    for a in EAS.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            a.set_actor_location(V(-2400, 0, 100), False, False)
            a.set_actor_rotation(R(yaw=0), False)
    nav = EAS.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(14500, 500, 300), R())
    nav.set_actor_scale3d(V(185, 55, 10))
    _finish(nav, "NavMeshBounds_All")

STAGES = [("ground", stage_ground), ("boundary", stage_boundary), ("path", stage_path), ("village", stage_village),
          ("meadow", stage_meadow), ("filmset", stage_filmset), ("forest", stage_forest), ("chasm", stage_chasm),
          ("arena", stage_arena), ("landmark", stage_landmark), ("misc", stage_misc)]

def build(which="all"):
    ensure_materials()
    names = [n for n, _ in STAGES] if which == "all" else which.split(",")
    done = []
    with unreal.ScopedEditorTransaction("LD build " + which):
        for n, f in STAGES:
            if n in names:
                f()
                done.append(n)
    return done
