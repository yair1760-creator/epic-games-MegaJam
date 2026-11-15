# Rated PG for Pig - second batch of inked HUD art (Meshy pictures in Art/Meshy/HUD/Export), put on the existing widgets.
# Run inside the editor (not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\DC_ApplyHudArt2.py").read()); apply_hud_art2()
# Edits the widget Blueprints in place (it does NOT rebuild them, so hand edits made after DC_BuildHUDWidgets.py stay):
#   - WBP_DCHUD "RecBadge" and WBP_AmmoCounter "AmmoFrame": the drawn slate plate T_DC_UI_HUDPlate as a 9-slice box on the
#     outer border; the gold trim and body borders go transparent. Texts, the red REC dot and the inner padding stay.
#   - WBP_AmmoCounter: the bar becomes the team's HP bar art recoloured gold (T_DC_UI_AmmoBar_Empty under _Full); the
#     progress bar clips the full picture, never scales it (UDCAmmoCounterWidget maps the bullets to the inner track).
#     A small inked shell (T_DC_UI_ShellIcon) sits left of the status text.
#   - WBP_DeathScreen "Card": the clapperboard card T_DC_UI_DeathCard (its own striped arm), the old clapper strip is
#     removed, the text column moves down into the slate, and the slate icon becomes T_DC_UI_ClapperIcon.
#   - BP_ShooterProjectile_Bullet (the shotgun pellet): SM_DC_ShotgunShell in place of the foam bullet, same length and
#     place along the flight direction, no collision on the mesh (the collision sphere is unchanged).
# Re-running DC_BuildHUDWidgets.py brings back the flat borders: run this again afterwards (and DC_ApplyHudPanelArt.py).
import unreal

UI_ART = "/Game/DirectorsCut/UI/Art"
SUB = unreal.get_editor_subsystem(unreal.WidgetBlueprintEditorSubsystem)
CLEAR = unreal.LinearColor(0.0, 0.0, 0.0, 0.0)
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)


def _tex(name):
    t = unreal.load_asset("%s/%s" % (UI_ART, name))
    assert t, name
    return t


def _brush(tex, size, margin=None):
    b = unreal.SlateBrush()
    b.set_editor_property("resource_object", tex)
    b.set_editor_property("image_size", unreal.Vector2D(size[0], size[1]))
    b.set_editor_property("tint_color", unreal.SlateColor(WHITE))
    if margin:
        b.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        b.set_editor_property("margin", unreal.Margin(*margin))
    else:
        b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    return b


def _widgets(bp):
    return {i.get_editor_property("widget").get_name(): i.get_editor_property("widget")
            for i in SUB.get_widgets(bp).get_editor_property("widgets")}


def _plate(ws, prefix, tex, size, margin, frame_pad):
    """outer border draws the picture; the two inner flat borders go transparent (the body keeps its padding)"""
    frame, trim, body = ws[prefix + "_InkFrame"], ws[prefix + "_GoldTrim"], ws[prefix + "_Body"]
    frame.set_editor_property("background", _brush(tex, size, margin))
    frame.set_editor_property("brush_color", WHITE)
    frame.set_editor_property("padding", unreal.Margin(*frame_pad))
    trim.set_editor_property("brush_color", CLEAR)
    trim.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    body.set_editor_property("brush_color", CLEAR)


def _finish(bp):
    ok = SUB.compile_widget_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
    return bool(ok)


PLATE_MARGIN = (0.09, 0.17, 0.09, 0.20)
SIZED_SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\Meshy\HUD\Sized"


def _import_sized():
    """Box-brush corners came out sized from the picture's own pixels, so the plates use copies made at exactly the
    size they draw at (Scripts/Blender_SizeHudArt.py)."""
    names = ("T_DC_UI_HUDPlate_Rec", "T_DC_UI_HUDPlate_Ammo", "T_DC_UI_DeathCard_Sized")
    tasks = []
    for n in names:
        t = unreal.AssetImportTask()
        t.filename = "%s\%s.png" % (SIZED_SRC, n)
        t.destination_path = UI_ART; t.destination_name = n
        t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for n in names:
        tx = _tex(n)
        tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tx.set_editor_property("srgb", True)
        tx.set_editor_property("never_stream", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tx)


def apply_hud_art2():
    out = {}
    _import_sized()

    # --- REC badge
    bp = unreal.load_asset("/Game/DirectorsCut/UI/WBP_DCHUD")
    ws = _widgets(bp)
    _plate(ws, "RecBadge", _tex("T_DC_UI_HUDPlate_Rec"), (154, 58), PLATE_MARGIN, (6, 5, 6, 9))
    out["WBP_DCHUD"] = _finish(bp)

    # --- ammo counter
    bp = unreal.load_asset("/Game/DirectorsCut/UI/WBP_AmmoCounter")
    ws = _widgets(bp)
    _plate(ws, "AmmoFrame", _tex("T_DC_UI_HUDPlate_Ammo"), (245, 92), PLATE_MARGIN, (8, 7, 8, 12))
    ws["BarBox"].set_editor_property("height_override", 26.0)          # 640 x 112 art at 150 px wide
    ws["BarInk"].set_editor_property("brush_color", CLEAR)
    ws["BarInk"].set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    bar = ws["AmmoBar"]
    st = bar.get_editor_property("widget_style")
    st.set_editor_property("background_image", _brush(_tex("T_DC_UI_AmmoBar_Empty"), (150, 26)))
    st.set_editor_property("fill_image", _brush(_tex("T_DC_UI_AmmoBar_Full"), (150, 26)))
    bar.set_editor_property("widget_style", st)
    bar.set_editor_property("fill_color_and_opacity", WHITE)
    bar.set_editor_property("border_padding", unreal.Vector2D(0.0, 0.0))
    if "ShellIcon" not in ws:
        row = SUB.wrap_widgets(bp, [ws["StatusText"]], unreal.HorizontalBox)[0].get_editor_property("widget")
        SUB.rename_widget(bp, row, "StatusRow")
        icon = SUB.create_and_add_widget(bp, unreal.Image, "ShellIcon", row, 0).get_editor_property("widget")
        ws = _widgets(bp)
    icon = ws["ShellIcon"]
    icon.set_editor_property("brush", _brush(_tex("T_DC_UI_ShellIcon"), (44, 20)))
    s = icon.get_editor_property("slot")
    s.set_padding(unreal.Margin(0, 0, 6, 0)); s.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    ws["StatusText"].get_editor_property("slot").set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    out["WBP_AmmoCounter"] = _finish(bp)

    # --- death card
    bp = unreal.load_asset("/Game/DirectorsCut/UI/WBP_DeathScreen")
    ws = _widgets(bp)
    _plate(ws, "Card", _tex("T_DC_UI_DeathCard_Sized"), (590, 400), (0.06, 0.37, 0.06, 0.09), (30, 150, 30, 30))
    ws["Card_Body"].set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    if "ClapperTop_Box" in ws:
        SUB.remove_widget(bp, ws["ClapperTop_Box"])                  # the card has its own clapper arm now
    ws = _widgets(bp)
    ws["CardBody"].set_editor_property("padding", unreal.Margin(48, 20, 48, 26))
    ws["SlateIcon"].set_editor_property("brush", _brush(_tex("T_DC_UI_ClapperIcon"), (48, 48)))
    out["WBP_DeathScreen"] = _finish(bp)

    # --- shotgun pellet: the inked shell instead of the foam bullet
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    pbp = unreal.load_asset("/Game/Variant_Shooter/Blueprints/Pickups/Projectiles/BP_ShooterProjectile_Bullet")
    shell = unreal.load_asset("/Game/DirectorsCut/Combat/Shotgun/Shell/SM_DC_ShotgunShell")
    done = False
    for h in sds.k2_gather_subobject_data_for_blueprint(pbp):
        o = lib.get_object(lib.get_data(h))
        if isinstance(o, unreal.StaticMeshComponent) and o.get_name().startswith("Mesh"):
            o.modify()
            o.set_editor_property("static_mesh", shell)
            # the foam bullet ran from X -31.7 to 3.1 in the actor (35 cm). The shell is 100 long along its own X:
            # scale 0.35 -> 35 cm, no turn needed, centred where the foam bullet was.
            o.set_editor_property("relative_rotation", unreal.Rotator(0.0, 0.0, 0.0))
            o.set_editor_property("relative_scale3d", unreal.Vector(0.35, 0.35, 0.35))
            o.set_editor_property("relative_location", unreal.Vector(-14.3, 0.0, 0.0))
            o.set_collision_profile_name("NoCollision")
            done = True
    unreal.BlueprintEditorLibrary.compile_blueprint(pbp)
    unreal.EditorAssetLibrary.save_loaded_asset(pbp)
    out["pellet_mesh_set"] = done
    return out
