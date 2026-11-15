# Rated PG for Pig - cuts the real doorway out of the shed facade picture so Saar's shed doors (DCShedDoors, 2.2 m wide,
# 3.2 m high, centred on the front wall) show through the facade card and can still swing open.
#   blender --background --python Scripts/Blender_ShedFacadeDoor.py
# In : Art/Meshy/Placeholders/Export/T_DC_PGShed_Facade.png      Out: Art/Meshy/Placeholders/Sized/T_DC_PGShed_Facade_Door.png
# The card is CARD_CM wide and tall, standing on the ground, centred on the front wall (LD_DressPlaceholders.ph_dress_C).
import bpy, os
import numpy as np
ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\Placeholders"
CARD_CM = 700.0
DOOR_W, DOOR_H = 225.0, 322.0           # the opening between Saar's door frames, from the ground up
img = bpy.data.images.load(os.path.join(ROOT, "Export", "T_DC_PGShed_Facade.png")); W, H = img.size
px = np.array(img.pixels[:], np.float32).reshape(H, W, 4)          # row 0 = bottom
u0, u1 = 0.5 - DOOR_W / 2 / CARD_CM, 0.5 + DOOR_W / 2 / CARD_CM
v1 = DOOR_H / CARD_CM                                             # from the bottom
px[0:int(round(v1 * H)), int(round(u0 * W)):int(round(u1 * W)), 3] = 0.0
out = bpy.data.images.new("door", W, H, alpha=True); out.alpha_mode = "STRAIGHT"
out.pixels[:] = px.ravel()
os.makedirs(os.path.join(ROOT, "Sized"), exist_ok=True)
out.filepath_raw = os.path.join(ROOT, "Sized", "T_DC_PGShed_Facade_Door.png"); out.file_format = "PNG"; out.save()
print("DOOR HOLE u", round(u0, 3), round(u1, 3), "v from bottom 0 ..", round(v1, 3))
