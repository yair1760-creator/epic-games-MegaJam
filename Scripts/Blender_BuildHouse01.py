# Director's Cut - builds a low-poly 3D house from taisho's house_01 drawing (Forgive Me Father style:
# simple boxy geometry, every face painted with the hand-drawn picture). Run with Blender in background:
#   blender --background --python Scripts/Blender_BuildHouse01.py
# Writes Art/Meshy/House/Export/SM_House01.glb (+ SM_House01.blend for hand edits); import the .glb into Unreal.
import bpy, bmesh, os

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
TEX = os.path.join(ROOT, r"Art\Meshy\House\Export\T_House01_Facade.png")
OUT_BLEND = os.path.join(ROOT, r"Art\Meshy\House\Export\SM_House01.blend")

# --- the drawing (2683 x 2712 px) and where its parts are, in pixels (x0, y0, x1, y1), y down
IW, IH = 2683.0, 2712.0
FRONT_WALL = (208, 1446, 2464, 2712)     # timber-framed wall with the window and the door
ROOF       = (110, 124, 2541, 1446)      # top beam + tiles + dormer
SIDE_WALL  = (208, 1446, 1555, 2712)     # left part of the wall (window, no door): reused on sides/back
PLASTER    = (421, 1572, 730, 1657)      # plain plaster for the gable triangles
BEAM       = (110, 124, 2541, 300)       # the wooden ridge beam strip

# --- sizes in metres, taken from the drawing at 5.45 m wide
S = 5.45 / IW                              # metres per pixel
WALL_W = (FRONT_WALL[2] - FRONT_WALL[0]) * S   # ~4.58
WALL_H = (FRONT_WALL[3] - FRONT_WALL[1]) * S   # ~2.57
ROOF_W = (ROOF[2] - ROOF[0]) * S               # ~4.94
ROOF_RISE = (ROOF[3] - ROOF[1]) * S            # ~2.68 (front view height of the roof)
DEPTH = 5.0
OVER = 0.3                                     # roof overhang front/back


def uv(region, u, v):
    """u, v in 0..1 inside a pixel region (v=0 bottom) -> texture UV (v up)"""
    x0, y0, x1, y1 = region
    px = x0 + (x1 - x0) * u
    py = y1 - (y1 - y0) * v
    return (px / IW, 1.0 - py / IH)


bpy.ops.wm.read_factory_settings(use_empty=True)
mesh = bpy.data.meshes.new("SM_House01")
obj = bpy.data.objects.new("SM_House01", mesh)
bpy.context.collection.objects.link(obj)
bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")


def quad(pts, region, uvs=((0, 0), (1, 0), (1, 1), (0, 1))):
    vs = [bm.verts.new(p) for p in pts]
    f = bm.faces.new(vs)
    for loop, (u, v) in zip(f.loops, uvs):
        loop[uvl].uv = uv(region, u, v)
    return f


def tri(pts, region, uvs):
    vs = [bm.verts.new(p) for p in pts]
    f = bm.faces.new(vs)
    for loop, (u, v) in zip(f.loops, uvs):
        loop[uvl].uv = uv(region, u, v)
    return f


# Blender: -Y is the front (faces the viewer in Front view), Z up, origin at the bottom centre
hx, hy = WALL_W / 2, DEPTH / 2
# walls (counter-clockwise seen from outside)
quad([(-hx, -hy, 0), (hx, -hy, 0), (hx, -hy, WALL_H), (-hx, -hy, WALL_H)], FRONT_WALL)          # front
quad([(hx, hy, 0), (-hx, hy, 0), (-hx, hy, WALL_H), (hx, hy, WALL_H)], SIDE_WALL)                # back
quad([(hx, -hy, 0), (hx, hy, 0), (hx, hy, WALL_H), (hx, -hy, WALL_H)], SIDE_WALL)                # right side
quad([(-hx, hy, 0), (-hx, -hy, 0), (-hx, -hy, WALL_H), (-hx, hy, WALL_H)], SIDE_WALL)            # left side

# gables: the triangles between the side walls and the roof
top = WALL_H + ROOF_RISE
tri([(hx, -hy, WALL_H), (hx, hy, WALL_H), (hx, 0, top)], PLASTER, ((0, 0), (1, 0), (0.5, 1)))
tri([(-hx, hy, WALL_H), (-hx, -hy, WALL_H), (-hx, 0, top)], PLASTER, ((0, 0), (1, 0), (0.5, 1)))

# roof: two slopes; the front one shows the drawn roof exactly as seen from the front
rx = ROOF_W / 2
drop = OVER * ROOF_RISE / hy                      # the slope continues past the wall by the overhang
ez = WALL_H - drop
quad([(-rx, -(hy + OVER), ez), (rx, -(hy + OVER), ez), (rx, 0, top), (-rx, 0, top)], ROOF)       # front slope
quad([(rx, hy + OVER, ez), (-rx, hy + OVER, ez), (-rx, 0, top), (rx, 0, top)], ROOF)             # back slope
# (the ridge beam is already drawn at the top of the roof texture, so no extra beam geometry)

bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
bm.to_mesh(mesh)
bm.free()

# material using the drawing (the UE import creates its own; this keeps the .blend readable)
mat = bpy.data.materials.new("M_House01")
mat.use_nodes = True
nt = mat.node_tree
tex = nt.nodes.new("ShaderNodeTexImage")
tex.image = bpy.data.images.load(TEX)
bsdf = nt.nodes.get("Principled BSDF")
nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
obj.data.materials.append(mat)

bpy.context.view_layer.objects.active = obj
obj.select_set(True)
bpy.ops.wm.save_as_mainfile(filepath=OUT_BLEND)
print("HOUSE OK", round(WALL_W, 2), round(WALL_H, 2), round(ROOF_W, 2), round(ROOF_RISE, 2), DEPTH)

# glTF too: Unreal 5.9's Interchange importer reads .glb reliably (mesh + UVs; the texture is imported separately)
OUT_GLB = os.path.join(ROOT, r"Art\Meshy\House\Export\SM_House01.glb")
bpy.ops.export_scene.gltf(filepath=OUT_GLB, export_format='GLB', use_selection=True, export_apply=True, export_image_format='NONE')
print("GLB OK", OUT_GLB)
