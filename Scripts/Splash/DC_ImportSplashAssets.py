# Rated PG for Pig - imports what the splash, content notice and QUIT button need into /Game/UI/Splash.
# Run inside the editor (not in Play):
#   exec(open(r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame\Scripts\Splash\DC_ImportSplashAssets.py").read()); import_splash_assets()
# - T_UI_Button_Quit_Bar : Art/UI/RatedPG/Button_Quit_Bar.png (same texture settings as the START button)
# - T_UI_ContentNotice_Card : Art/UI/RatedPG/ContentNotice_Card.png (the drawn content notice)
# - Font_NoticeItalic    : the engine's own Roboto-Italic.ttf (same family as the HUD fonts) for the Director's line
# The Unreal Engine logo picture is not used (removed Nov 15): the splash says "Made with Unreal Engine" in text.
import os
import unreal

SPLASH_DIR = "/Game/UI/Splash"
PROJECT = r"D:\EpicGameJam\EpicGameJamGame\EpicGameJamGame"
EAL = unreal.EditorAssetLibrary


def _import(src, name):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = SPLASH_DIR; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    return unreal.load_asset(SPLASH_DIR + "/" + name)


def _ui_texture(tx):
    tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)   # "UserInterface2D"
    tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tx.set_editor_property("srgb", True)
    tx.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(tx)


def import_splash_assets():
    out = {}
    quit_tx = _import(os.path.join(PROJECT, "Art", "UI", "RatedPG", "Button_Quit_Bar.png"), "T_UI_Button_Quit_Bar")
    _ui_texture(quit_tx)
    out["quit"] = [quit_tx.blueprint_get_size_x(), quit_tx.blueprint_get_size_y()]

    card_tx = _import(os.path.join(PROJECT, "Art", "UI", "RatedPG", "ContentNotice_Card.png"), "T_UI_ContentNotice_Card")
    _ui_texture(card_tx)
    out["notice_card"] = [card_tx.blueprint_get_size_x(), card_tx.blueprint_get_size_y()]

    ttf = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir()), "Slate", "Fonts", "Roboto-Italic.ttf")
    face = _import(ttf, "FF_NoticeItalic")
    font_path = SPLASH_DIR + "/Font_NoticeItalic"
    font = unreal.load_asset(font_path) if EAL.does_asset_exist(font_path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset("Font_NoticeItalic", SPLASH_DIR, unreal.Font, unreal.FontFactory())
    cf = font.get_editor_property("composite_font")
    cf.import_text('(DefaultTypeface=(Fonts=((Name="Regular",Font=(FontFaceAsset="%s")))))' % face.get_path_name())
    font.set_editor_property("composite_font", cf)
    unreal.EditorLoadingAndSavingUtils.save_packages([face.get_outermost(), font.get_outermost()], False)
    out["italic_font"] = font.get_path_name()

    return out
