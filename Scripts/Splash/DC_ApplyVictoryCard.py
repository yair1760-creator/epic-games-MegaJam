# Rated PG for Pig - puts the painted "CUT! THAT'S A WRAP!" card (Art/UI/RatedPG/Victory_Card.png, from
# Scripts/Splash/Blender_VictoryCard.py) on the victory screen WBP_CutScreen. Run inside the editor, not in Play:
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\Splash\DC_ApplyVictoryCard.py").read()); apply_victory_card()
# - Card_InkFrame draws the card at a fixed size; the gold trim and dark body go transparent
# - the code-drawn clapper strip and the title text are hidden (both are painted on the card)
# - the column moves down into the empty paper; the texts turn to dark ink so they read on cream
import unreal

SRC = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Art\UI\RatedPG\Victory_Card.png"
DEST, NAME = "/Game/DirectorsCut/UI/Menu", "T_UI_Victory_Card"
SUB = unreal.get_editor_subsystem(unreal.WidgetBlueprintEditorSubsystem)
CLEAR = unreal.LinearColor(0, 0, 0, 0)
WHITE = unreal.LinearColor(1, 1, 1, 1)
CARD = (1300.0, 709.0)                      # the 1351 x 737 picture, a little smaller on a 1920 x 1080 screen
PAD = (150.0, 286.0, 150.0, 50.0)          # inside the paper, below the painted title


def _import():
    t = unreal.AssetImportTask()
    t.filename = SRC; t.destination_path = DEST; t.destination_name = NAME
    t.automated = True; t.replace_existing = True; t.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tx = unreal.load_asset(DEST + "/" + NAME)
    ref = unreal.load_asset("/Game/DirectorsCut/UI/Menu/T_UI_Button_Start")
    for p in ("compression_settings", "mip_gen_settings", "lod_group", "srgb", "never_stream", "filter"):
        tx.set_editor_property(p, ref.get_editor_property(p))
    unreal.EditorAssetLibrary.save_loaded_asset(tx)
    return tx


def _ink(tb, rgb):
    tb.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)))
    f = tb.get_editor_property("font")
    o = f.get_editor_property("outline_settings")
    o.set_editor_property("outline_size", 0)
    f.set_editor_property("outline_settings", o)
    tb.set_editor_property("font", f)
    tb.set_editor_property("shadow_color_and_opacity", CLEAR)


def apply_victory_card():
    tx = _import()
    bp = unreal.load_asset("/Game/DirectorsCut/UI/WBP_CutScreen")
    ws = {i.get_editor_property("widget").get_name(): i.get_editor_property("widget")
          for i in SUB.get_widgets(bp).get_editor_property("widgets")}

    b = unreal.SlateBrush()
    b.set_editor_property("resource_object", tx)
    b.set_editor_property("image_size", unreal.Vector2D(*CARD))
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    b.set_editor_property("tint_color", unreal.SlateColor(WHITE))
    frame = ws["Card_InkFrame"]
    frame.set_editor_property("background", b)
    frame.set_editor_property("brush_color", WHITE)
    frame.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    slot = frame.get_editor_property("slot")
    slot.set_auto_size(False)
    slot.set_size(unreal.Vector2D(*CARD))
    for n in ("Card_GoldTrim", "Card_Body"):
        ws[n].set_editor_property("brush_color", CLEAR)
        ws[n].set_editor_property("padding", unreal.Margin(0, 0, 0, 0))
    ws["ClapperTop_Box"].set_visibility(unreal.SlateVisibility.COLLAPSED)
    ws["TitleText"].set_visibility(unreal.SlateVisibility.COLLAPSED)
    ws["CardBody"].set_editor_property("padding", unreal.Margin(*PAD))

    _ink(ws["SubtitleText"], (0.06, 0.04, 0.03))
    _ink(ws["ReviewText"], (0.55, 0.08, 0.05))
    _ink(ws["StatsText"], (0.09, 0.07, 0.05))
    ws["DirectorFrame"].set_editor_property("brush_color", unreal.LinearColor(0.03, 0.02, 0.02, 1.0))

    ok = SUB.compile_widget_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
    return {"compiled": bool(ok), "tex": [tx.blueprint_get_size_x(), tx.blueprint_get_size_y()]}
