# Director's Cut - puts the drawn HUD panel frames (Scripts/Blender_BuildHudPanels.py) on the HUD widgets.
# Run inside the editor (not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\DC_ApplyHudPanelArt.py").read()); apply_hud_panel_art()
# What it does:
#   - imports Art/UI/Panels/Export/T_UI_Panel_*.png into /Game/DirectorsCut/UI/HUD (UI texture group, no streaming)
#   - in WBP_HealthBar, WBP_QuestTracker and WBP_Subtitles (built by Scripts/DC_BuildHUDWidgets.py) the three nested
#     flat-colour borders <Name>_InkFrame / _GoldTrim / _Body keep their place and children; the outer one now draws the
#     frame picture as a 9-slice box and the two inner ones go transparent. Layout, texts and fonts are not touched.
#   - the controls card (UDCControlsHintWidget) loads T_UI_Panel_Controls itself, in code.
# Note: re-running DC_BuildHUDWidgets.py rebuilds those widgets with the flat colours; run this again afterwards.
import json, os, unreal

PANEL_SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\Panels\Export"
PANEL_DEST = "/Game/DirectorsCut/UI/HUD"
# widget blueprint -> (frame name prefix, panel picture)
PANEL_TARGETS = {"WBP_HealthBar": ("HealthFrame", "Health"), "WBP_QuestTracker": ("QuestFrame", "Script"), "WBP_Subtitles": ("SubtitlePanel", "Speech")}


def _import_panel_textures():
    tasks = []
    for n in ("Health", "Speech", "Controls", "Script"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(PANEL_SRC, "T_UI_Panel_%s.png" % n)
        t.destination_path = PANEL_DEST
        t.destination_name = "T_UI_Panel_%s" % n
        t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    out = {}
    for n in ("Health", "Speech", "Controls", "Script"):
        tx = unreal.load_asset("%s/T_UI_Panel_%s" % (PANEL_DEST, n))
        tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tx.set_editor_property("never_stream", True)
        tx.set_editor_property("srgb", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tx)
        out[n] = tx
    return out


def _widgets(sub, bp):
    info = sub.get_widgets(bp)
    for prop in ("widgets", "widget_infos", "all_widgets"):
        try:
            items = info.get_editor_property(prop)
            return {i.get_editor_property("widget").get_name(): i.get_editor_property("widget") for i in items}
        except Exception:
            continue
    raise RuntimeError("UMGWidgetTreeInfo layout not recognised: %s" % [p for p in dir(info) if not p.startswith("_")])


def apply_hud_panel_art():
    sub = unreal.get_editor_subsystem(unreal.WidgetBlueprintEditorSubsystem)
    meta = json.load(open(os.path.join(PANEL_SRC, "Panels.json")))
    tex = _import_panel_textures()
    clear = unreal.LinearColor(0.0, 0.0, 0.0, 0.0)
    out = {}
    for wbp, (prefix, panel) in PANEL_TARGETS.items():
        bp = unreal.load_asset("/Game/DirectorsCut/UI/" + wbp)
        ws = _widgets(sub, bp)
        frame, trim, body = ws[prefix + "_InkFrame"], ws[prefix + "_GoldTrim"], ws[prefix + "_Body"]
        m = meta[panel]
        brush = unreal.SlateBrush()
        brush.set_editor_property("resource_object", tex[panel])
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
        brush.set_editor_property("margin", unreal.Margin(*m["margin"]))
        brush.set_editor_property("image_size", unreal.DeprecateSlateVector2D(m["logical"][0], m["logical"][1]) if hasattr(unreal, "DeprecateSlateVector2D") else unreal.Vector2D(m["logical"][0], m["logical"][1]))
        brush.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)))
        frame.set_editor_property("background", brush)
        frame.set_editor_property("brush_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        f = float(m["frame_screen_px"])
        frame.set_editor_property("padding", unreal.Margin(f, f, f, f))            # content sits inside the drawn frame band
        trim.set_editor_property("brush_color", clear)
        trim.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        body.set_editor_property("brush_color", clear)
        ok = sub.compile_widget_blueprint(bp)
        unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
        out[wbp] = {"panel": panel, "compiled": bool(ok), "frame_px": f}
    return out
