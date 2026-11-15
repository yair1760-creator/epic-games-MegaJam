# Rated PG for Pig - sized copies of the drawn HUD plates for 9-slice ("Box") brushes.
#   blender --background --python Scripts/Blender_SizeHudArt.py
# Why: in game the Box brush's corners came out sized from the picture's own pixel size, not from the brush's image size,
# so the 614 x 234 plate's corners did not fit a 40 px REC badge and the death card's clapper band was ~270 px instead
# of ~148. These copies are made at exactly the size they should draw at on a 1080p screen.
# In : Art/Meshy/HUD/Export/T_DC_UI_HUDPlate.png, T_DC_UI_DeathCard.png
# Out: Art/Meshy/HUD/Sized/T_DC_UI_HUDPlate_Rec.png (154 x 58), T_DC_UI_HUDPlate_Ammo.png (245 x 92),
#      T_DC_UI_DeathCard_Sized.png (590 x 400)
import bpy, os

ROOT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
SRC = os.path.join(ROOT, "Art", "Meshy", "HUD", "Export")
OUT = os.path.join(ROOT, "Art", "Meshy", "HUD", "Sized")
os.makedirs(OUT, exist_ok=True)
JOBS = [("T_DC_UI_HUDPlate.png", "T_DC_UI_HUDPlate_Rec.png", 154, 58),
        ("T_DC_UI_HUDPlate.png", "T_DC_UI_HUDPlate_Ammo.png", 245, 92),
        ("T_DC_UI_DeathCard.png", "T_DC_UI_DeathCard_Sized.png", 590, 400)]
for src, dst, w, h in JOBS:
    img = bpy.data.images.load(os.path.join(SRC, src))
    img.alpha_mode = "STRAIGHT"
    img.scale(w, h)
    img.filepath_raw = os.path.join(OUT, dst)
    img.file_format = "PNG"
    img.save()
    print("SIZED", dst, img.size[0], img.size[1])
    bpy.data.images.remove(img)
