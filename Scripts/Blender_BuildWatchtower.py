# Director's Cut - the watchtower in the Forgive Me Father comic style: an inked plank skin built to the exact shape
# of the greybox watchtower (4 legs, deck, 3 rails, 11.7 m walk-up ramp), so the visible ramp and the walkable one match.
# Look borrowed from Taisho's woodland slide: plank-box deck on log legs with a plank ramp.
#   blender --background --python Scripts/Blender_BuildWatchtower.py
# In : Art/Meshy/SetFloor/Export/T_DC_SetStage_Planks.png   inked planks (the stage picture, reused for all wood)
# Out: Art/Meshy/Watchtower/Export/SM_DC_Watchtower.glb (+ Watchtower.blend for hand edits)
#      One mesh, one material slot (Unreal puts MI_DC_SetStage on it). Origin = tower centre on the ground
#      (Unreal 2800, 3600, 0). No collision: the hidden greybox keeps blocking and carrying the player.
# Axes: Blender (x, y) = Unreal (100 x, -100 y). The ramp comes down towards Unreal -Y = Blender +Y.
# Greybox numbers (LD_BuildDirectorsCut): legs r 0.2 at (+-1.8, +-1.8), 4.5 high; deck 4.4 x 4.4, z 4.2..4.6;
# rails 0.2 thick, z 4.6..5.6, on the back and both sides; ramp 11.662 x 2.2 x 0.6, centre (0, 7.6, 1.876), pitch 22.17.
import bpy, bmesh, math, os
import numpy as np
from mathutils import Vector

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
OUT = os.path.join(ROOT, "Art", "Meshy", "Watchtower", "Export")
WOOD_TEX = os.path.join(ROOT, "Art", "Meshy", "SetFloor", "Export", "T_DC_SetStage_Planks.png")
WOOD_M = 2.4            # metres one plank picture covers (8 planks -> 30 cm planks, like the stage)
G = 0.012               # the skin is this much bigger than the greybox, so nothing of it pokes through
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

mat = bpy.data.materials.new("M_Tower_Wood")
mat.use_nodes = True
tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
tex.image = bpy.data.images.load(WOOD_TEX)
mat.node_tree.links.new(tex.outputs["Color"], mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"])

bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")


def face(pts, uvs, out):
    n = (Vector(pts[1]) - Vector(pts[0])).cross(Vector(pts[2]) - Vector(pts[0]))
    if n.dot(Vector(out)) < 0:
        pts, uvs = pts[::-1], uvs[::-1]
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    for loop, uv in zip(f.loops, uvs):
        loop[uvl].uv = uv


def obox(c, ax_l, ax_w, ax_t, hl, hw, ht0, ht1, uoff=0.0):
    """Oriented box: centre c, unit axes (long, wide, thick), half length / width, thickness from ht0 to ht1.
    Planks run along the long axis on every face (u = along the board, v = across it)."""
    c, L, W, T = Vector(c), Vector(ax_l), Vector(ax_w), Vector(ax_t)

    def P(a, b, t):
        return tuple(c + L * a + W * b + T * t)

    def quad(corners, out, uvf):
        face([P(*k) for k in corners], [uvf(*k) for k in corners], tuple(out))

    for t, sgn in ((ht0, -1), (ht1, 1)):                               # the two broad faces
        quad([(-hl, -hw, t), (hl, -hw, t), (hl, hw, t), (-hl, hw, t)], T * sgn, lambda a, b, t: (a / WOOD_M + uoff, b / WOOD_M))
    for b, sgn in ((-hw, -1), (hw, 1)):                                # the two long edges
        quad([(-hl, b, ht0), (hl, b, ht0), (hl, b, ht1), (-hl, b, ht1)], W * sgn, lambda a, b, t: (a / WOOD_M + uoff, t / WOOD_M + 0.02))
    for a, sgn in ((-hl, -1), (hl, 1)):                                # the two ends (end grain)
        quad([(a, -hw, ht0), (a, hw, ht0), (a, hw, ht1), (a, -hw, ht1)], L * sgn, lambda a, b, t: (b / WOOD_M + uoff, t / WOOD_M + 0.02))


X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)


def box(lo, hi, long_axis, uoff=0.0):
    """Axis-aligned board; planks run along long_axis (0 = x, 1 = y, 2 = z)."""
    c = [(lo[i] + hi[i]) / 2 for i in range(3)]
    h = [(hi[i] - lo[i]) / 2 for i in range(3)]
    axes = [X, Y, Z]
    others = [i for i in range(3) if i != long_axis]
    w_i, t_i = (others[0], others[1]) if h[others[0]] >= h[others[1]] else (others[1], others[0])
    obox(c, axes[long_axis], axes[w_i], axes[t_i], h[long_axis], h[w_i], -h[t_i], h[t_i], uoff)


def log(x, y, z0, z1, r0, r1, seg=10, uoff=0.0):
    """Leg: a slightly tapered log, grain running up."""
    for k in range(seg):
        a0, a1 = 2 * math.pi * k / seg, 2 * math.pi * (k + 1) / seg
        am = (a0 + a1) / 2
        pts = [(x + r0 * math.cos(a0), y + r0 * math.sin(a0), z0), (x + r0 * math.cos(a1), y + r0 * math.sin(a1), z0),
               (x + r1 * math.cos(a1), y + r1 * math.sin(a1), z1), (x + r1 * math.cos(a0), y + r1 * math.sin(a0), z1)]
        v0, v1 = k * 0.14, (k + 1) * 0.14
        face(pts, [(z0 / WOOD_M + uoff, v0), (z0 / WOOD_M + uoff, v1), (z1 / WOOD_M + uoff, v1), (z1 / WOOD_M + uoff, v0)], (math.cos(am), math.sin(am), 0))


# ---------------------------------------------------------------- legs (greybox r 0.20, z 0..4.5)
for i, (lx, ly) in enumerate(((-1.8, -1.8), (-1.8, 1.8), (1.8, -1.8), (1.8, 1.8))):
    log(lx, ly, -0.05, 4.5, 0.25, 0.215, uoff=0.37 * i)
    box((lx - 0.34, ly - 0.34, -0.02), (lx + 0.34, ly + 0.34, 0.07), 0, uoff=0.2 * i)          # foot plate

# knee braces under the deck, between the legs (lowest point 3.0 m: well above the player's head)
for lx in (-1.8, 1.8):
    for ly in (-1.8, 1.8):
        for ax in (0, 1):
            d = [0.0, 0.0, 0.0]
            d[ax] = -1.0 if (lx if ax == 0 else ly) > 0 else 1.0                  # towards the tower's middle
            L = Vector((d[0], d[1], 1.0)).normalized()
            Wd = Vector((d[1], d[0], 0.0)).normalized()                           # horizontal, across the brace
            T = L.cross(Wd).normalized()
            c = Vector((lx + d[0] * 0.6, ly + d[1] * 0.6, 3.6))
            obox(c, L, Wd, T, 0.85, 0.07, -0.05, 0.05, uoff=0.11 * (ax + 1))

# ---------------------------------------------------------------- deck (greybox 4.4 x 4.4, z 4.2..4.6)
box((-2.2 - G, -2.2 - G, 4.2 - G), (2.2 + G, 2.2 + G, 4.6 + G), 1)                # planks run front to back (along y)
for sx in (-1, 1):                                                                # beams under the deck, on the legs
    box((sx * 1.8 - 0.14, -2.3, 4.02), (sx * 1.8 + 0.14, 2.3, 4.2), 1, uoff=0.3)
box((-2.26, 2.2, 4.12), (2.26, 2.26, 4.64), 0, uoff=0.45)                         # front fascia board (ramp side)

# ---------------------------------------------------------------- rails (greybox 0.2 thick, z 4.6..5.6; back + both sides)
RZ0, RZ1 = 4.6, 5.6 + G
box((-2.2 - G, -2.2 - G, RZ0), (2.2 + G, -2.0 + G, RZ1), 0, uoff=0.1)             # back (Unreal +Y)
for sx in (-1, 1):
    x0, x1 = (2.0 - G, 2.2 + G) if sx > 0 else (-2.2 - G, -2.0 + G)
    box((x0, -2.2 - G, RZ0), (x1, 2.2 + G, RZ1), 1, uoff=0.2 + 0.3 * sx)
box((-2.3, -2.3, RZ1), (2.3, -1.94, RZ1 + 0.07), 0, uoff=0.6)                     # cap boards
for sx in (-1, 1):
    box((sx * 2.12 - 0.18, -2.3, RZ1), (sx * 2.12 + 0.18, 2.3, RZ1 + 0.07), 1, uoff=0.7)
for cx, cy in ((-2.1, -2.1), (2.1, -2.1), (-2.1, 2.1), (2.1, 2.1)):               # corner posts
    box((cx - 0.13, cy - 0.13, 4.6), (cx + 0.13, cy + 0.13, 5.78), 2, uoff=0.15)

# ---------------------------------------------------------------- ramp (greybox 11.662 x 2.2 x 0.6, pitch 22.17, comes down towards +y)
PITCH = math.radians(22.17)
RC = (0.0, 7.60, 1.876)
RL = (0.0, math.cos(PITCH), -math.sin(PITCH))            # down the ramp
RN = (0.0, math.sin(PITCH), math.cos(PITCH))             # out of its top face
HL, HW, HT = 11.662 / 2, 1.1, 0.3
# planks lie ACROSS the ramp: one board per 0.45 m, each its own oriented box so the ink lines read as steps
NB = 26
bl = 2 * (HL + G) / NB
for k in range(NB):
    s = -HL - G + bl * (k + 0.5)
    c = Vector(RC) + Vector(RL) * s
    obox(c, X, RL, RN, HW + G, bl / 2, -HT - G, HT + G, uoff=0.173 * k)
for k in range(1, NB, 2):                                # cleats: a low batten on every second board
    s = -HL - G + bl * k
    c = Vector(RC) + Vector(RL) * s
    obox(c, X, RL, RN, HW + G, 0.035, HT + G, HT + G + 0.03, uoff=0.31 * k)
for sx in (-1, 1):                                       # side stringers, flush with the walking surface
    c = Vector(RC) + Vector(X) * (sx * (HW + G + 0.05))
    obox(c, RL, RN, X, HL + G, HT + 0.08, -0.05, 0.05, uoff=0.5 + 0.2 * sx)

bmesh.ops.triangulate(bm, faces=bm.faces)
me = bpy.data.meshes.new("SM_DC_Watchtower")
bm.to_mesh(me); bm.free()
me.materials.append(mat)
ob = bpy.data.objects.new("SM_DC_Watchtower", me)
bpy.context.scene.collection.objects.link(ob)
co = np.array([v.co[:] for v in me.vertices])
print("MESH size m", (co.max(0) - co.min(0)).round(2).tolist(), "min", co.min(0).round(2).tolist(), "tris", len(me.polygons))
bpy.ops.object.select_all(action="DESELECT")
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "SM_DC_Watchtower.glb"), export_format="GLB", use_selection=True,
                          export_apply=True, export_image_format="NONE")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "Watchtower.blend"))
print("exported")
