# Director's Cut - brick skins for the level's four boundary walls, so Taisho's brick wall picture tiles evenly on
# them instead of being stretched once over a 100 m greybox cube.
#   blender --background --python Scripts/Blender_BuildBoundaryWalls.py
# Out: Art/Meshy/WallTexture/Export/
#      SM_DC_BoundaryWall_100x15.glb   west wall            (B_West)
#      SM_DC_BoundaryWall_134x15.glb   north and south wall (B_North, B_South)
#      SM_DC_BoundaryWall_100x7.glb    low backstage wall   (B_BackstageEast)
# Each is a plain box, long side along Blender X, origin at the bottom centre, 4 cm bigger than the greybox wall all
# round (so it never z-fights with it). UVs are in wall metres / TILE_M on every face, so the picture repeats at the
# same brick size on all walls. No material or texture is exported: Unreal puts MI_Taisho_Wall on it
# (Scripts/LD_DressMainMap.py, dress_walls).
import bpy, bmesh, os

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
OUT = os.path.join(ROOT, "Art", "Meshy", "WallTexture", "Export")
TILE_M = 5.0            # metres one brick picture covers (28 brick rows -> 18 cm rows: oversized, reads from across the set)
THICK = 1.0             # greybox wall thickness
GROW = 0.04             # skin is this much bigger than the greybox on each side
WALLS = [("SM_DC_BoundaryWall_100x15", 100.0, 15.0), ("SM_DC_BoundaryWall_134x15", 134.0, 15.0), ("SM_DC_BoundaryWall_100x7", 100.0, 7.0)]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

for name, length, height in WALLS:
    hx, hy = length / 2.0 + GROW, THICK / 2.0 + GROW
    z0, z1 = 0.0, height + GROW
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")

    def quad(pts, uvs):
        f = bm.faces.new([bm.verts.new(p) for p in pts])
        for loop, t in zip(f.loops, uvs):
            loop[uv].uv = (t[0] / TILE_M, t[1] / TILE_M)

    # long faces (+Y and -Y): u runs along the wall, v up
    quad([(hx, hy, z0), (-hx, hy, z0), (-hx, hy, z1), (hx, hy, z1)], [(0, z0), (2 * hx, z0), (2 * hx, z1), (0, z1)])
    quad([(-hx, -hy, z0), (hx, -hy, z0), (hx, -hy, z1), (-hx, -hy, z1)], [(0, z0), (2 * hx, z0), (2 * hx, z1), (0, z1)])
    # end faces
    quad([(hx, -hy, z0), (hx, hy, z0), (hx, hy, z1), (hx, -hy, z1)], [(0, z0), (2 * hy, z0), (2 * hy, z1), (0, z1)])
    quad([(-hx, hy, z0), (-hx, -hy, z0), (-hx, -hy, z1), (-hx, hy, z1)], [(0, z0), (2 * hy, z0), (2 * hy, z1), (0, z1)])
    # top
    quad([(-hx, -hy, z1), (hx, -hy, z1), (hx, hy, z1), (-hx, hy, z1)], [(0, 0), (2 * hx, 0), (2 * hx, 2 * hy), (0, 2 * hy)])
    bm.normal_update()
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    mat = bpy.data.materials.new("M_BoundaryWall")
    me.materials.append(mat)
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + ".glb"), export_format="GLB", use_selection=True,
                              export_image_format="NONE", export_yup=True)
    print("exported", name, "faces", len(me.polygons))
