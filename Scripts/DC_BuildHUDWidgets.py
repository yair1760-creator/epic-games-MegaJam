# Director's Cut - imports UI art & fonts, then builds/restyles all HUD, Start, Pause, and Cut widgets in /Game/DirectorsCut/UI.
#
# Run inside the editor (Output Log, Python) or via UnrealEditor-Cmd:
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_BuildHUDWidgets.py").read())
import os
import unreal

PROJ_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ENGINE_FONT_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_content_dir()), "Slate", "Fonts")
ART_SRC_DIR = os.path.join(PROJ_DIR, "Art", "UI")

FOLDER = "/Game/DirectorsCut/UI"
ART_DEST = FOLDER + "/Art"
FONT_DEST = FOLDER + "/Fonts"
GAME_MODE = "/Game/DirectorsCut/Blueprints/BP_DCGameMode"
PLAYER_CTRL = "/Game/DirectorsCut/Blueprints/BP_DCPlayerController"

SUB = unreal.get_editor_subsystem(unreal.WidgetBlueprintEditorSubsystem)
EAL = unreal.EditorAssetLibrary
LOG = []

# Forgive Me Father / Cinema Slate Palette
INK = unreal.LinearColor(0.03, 0.025, 0.02, 1.0)
SLATE_BG = unreal.LinearColor(0.06, 0.05, 0.06, 0.94)
SLATE_CARD = unreal.LinearColor(0.08, 0.065, 0.075, 0.96)
BACKDROP = unreal.LinearColor(0.015, 0.01, 0.02, 0.78)
GOLD = unreal.LinearColor(0.91, 0.70, 0.19, 1.0)
CREAM = unreal.LinearColor(0.96, 0.93, 0.86, 1.0)
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
SILVER = unreal.LinearColor(0.76, 0.80, 0.85, 1.0)
RED = unreal.LinearColor(0.84, 0.14, 0.13, 1.0)
RED_BRIGHT = unreal.LinearColor(0.96, 0.24, 0.22, 1.0)
PINK = unreal.LinearColor(1.0, 0.65, 0.80, 1.0)
TRACK_BG = unreal.LinearColor(0.14, 0.08, 0.08, 1.0)

TEXTURES = {}
FONTS = {}


def _obj(path):
    return unreal.load_object(None, "%s.%s" % (path, path.rsplit("/", 1)[-1]))


def _cls(name):
    return unreal.load_class(None, "%s/%s.%s_C" % (FOLDER, name, name))


def import_ui_textures():
    names = [
        "T_DC_Director_Portrait",
        "T_DC_UI_ClapperBar",
        "T_DC_UI_HeartIcon",
        "T_DC_UI_SlateIcon",
        "T_DC_UI_Crosshair",
    ]
    tasks = []
    for name in names:
        src = os.path.join(ART_SRC_DIR, name + ".png")
        if not os.path.isfile(src):
            raise RuntimeError("Missing UI art PNG: " + src)
        t = unreal.AssetImportTask()
        t.filename = src
        t.destination_path = ART_DEST
        t.destination_name = name
        t.replace_existing = True
        t.automated = True
        tasks.append((name, t))

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, t in tasks])
    to_save = []
    for name, _ in tasks:
        tex = _obj(ART_DEST + "/" + name)
        if not tex:
            raise RuntimeError("Failed to import UI texture: " + name)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("never_stream", True)
        tex.set_editor_property("srgb", True)
        TEXTURES[name] = tex
        to_save.append(tex.get_outermost())
    unreal.EditorLoadingAndSavingUtils.save_packages(to_save, False)
    LOG.append("ART  imported %d textures into %s" % (len(TEXTURES), ART_DEST))


def import_ui_fonts():
    specs = [
        ("Font_Display",   "FF_DisplayBlack",     "Roboto-Black.ttf"),
        ("Font_Condensed", "FF_DisplayCondensed", "Roboto-BoldCondensed.ttf"),
        ("Font_Body",      "FF_BodyMedium",       "Roboto-Medium.ttf"),
        ("Font_Mono",      "FF_SlateMono",        "DroidSansMono.ttf"),
    ]
    tasks = []
    for _, face_name, ttf_name in specs:
        src = os.path.join(ENGINE_FONT_DIR, ttf_name)
        if not os.path.isfile(src):
            raise RuntimeError("Missing Engine font TTF: " + src)
        t = unreal.AssetImportTask()
        t.filename = src
        t.destination_path = FONT_DEST
        t.destination_name = face_name
        t.replace_existing = True
        t.automated = True
        tasks.append((face_name, t))

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, t in tasks])

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    fnt_factory = unreal.FontFactory()
    to_save = []

    for font_name, face_name, _ in specs:
        face_path = FONT_DEST + "/" + face_name
        face = _obj(face_path)
        if not face:
            raise RuntimeError("Failed to import FontFace: " + face_name)
        to_save.append(face.get_outermost())

        font_pkg = FONT_DEST + "/" + font_name
        font_asset = _obj(font_pkg)
        if not font_asset:
            font_asset = asset_tools.create_asset(font_name, FONT_DEST, unreal.Font, fnt_factory)
        if not font_asset:
            raise RuntimeError("Failed to create Font asset: " + font_name)

        cf = font_asset.get_editor_property("composite_font")
        cf_text = '(DefaultTypeface=(Fonts=((Name="Regular",Font=(FontFaceAsset="%s")))))' % face.get_path_name()
        cf.import_text(cf_text)
        font_asset.set_editor_property("composite_font", cf)
        FONTS[font_name] = font_asset
        to_save.append(font_asset.get_outermost())

    unreal.EditorLoadingAndSavingUtils.save_packages(to_save, False)
    LOG.append("FONT imported %d fonts into %s" % (len(FONTS), FONT_DEST))


def add(bp, cls, name, parent=None):
    info = SUB.create_and_add_widget(bp, cls, name, parent, -1)
    w = info.get_editor_property("widget")
    if not w:
        raise RuntimeError("could not add %s to %s" % (name, bp.get_name()))
    return w


def style_text(w, value, size, color=CREAM, font_key="Font_Body", outline_size=1, wrap=False, justify=None):
    w.set_editor_property("text", unreal.Text(value))
    fi = w.get_editor_property("font")
    fi.font_object = FONTS[font_key]
    fi.typeface_font_name = unreal.Name("Regular")
    fi.size = size
    outline = fi.outline_settings
    outline.outline_size = outline_size
    outline.outline_color = INK
    fi.outline_settings = outline
    w.set_editor_property("font", fi)
    w.set_editor_property("color_and_opacity", unreal.SlateColor(specified_color=color))
    w.set_editor_property("shadow_offset", unreal.Vector2D(2.0, 2.0))
    w.set_editor_property("shadow_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.9))
    if wrap:
        w.set_editor_property("auto_wrap_text", True)
    if justify is not None:
        w.set_editor_property("justification", justify)


def make_framed_panel(bp, name, parent=None, pad=(16, 12, 16, 12), bg_color=SLATE_BG, gold_thickness=2, ink_thickness=3):
    """Builds Ink outer border -> Gold trim border -> Dark cinema slate inner border."""
    outer = add(bp, unreal.Border, name + "_InkFrame", parent)
    outer.set_editor_property("brush_color", INK)
    outer.set_editor_property("padding", unreal.Margin(ink_thickness, ink_thickness, ink_thickness, ink_thickness))

    trim = add(bp, unreal.Border, name + "_GoldTrim", outer)
    trim.set_editor_property("brush_color", GOLD)
    trim.set_editor_property("padding", unreal.Margin(gold_thickness, gold_thickness, gold_thickness, gold_thickness))

    inner = add(bp, unreal.Border, name + "_Body", trim)
    inner.set_editor_property("brush_color", bg_color)
    inner.set_editor_property("padding", unreal.Margin(*pad))
    return outer, inner


def add_clapper_strip(bp, name, parent, height=14):
    sb = add(bp, unreal.SizeBox, name + "_Box", parent)
    sb.set_height_override(height)
    img = add(bp, unreal.Image, name, sb)
    brush = img.get_editor_property("brush")
    brush.resource_object = TEXTURES["T_DC_UI_ClapperBar"]
    brush.image_size = unreal.Vector2D(512, height)
    img.set_editor_property("brush", brush)
    return sb


def add_icon(bp, name, parent, tex_key, size_xy, pad=(0, 0, 0, 0), valign=unreal.VerticalAlignment.V_ALIGN_CENTER):
    img = add(bp, unreal.Image, name, parent)
    brush = img.get_editor_property("brush")
    brush.resource_object = TEXTURES[tex_key]
    brush.image_size = unreal.Vector2D(size_xy[0], size_xy[1])
    img.set_editor_property("brush", brush)
    if parent is not None and isinstance(parent, unreal.HorizontalBox):
        box_slot(img, pad, valign)
    return img


def style_button(btn, base_color, hover_color, press_color):
    style = btn.get_editor_property("widget_style")

    def _brush(col):
        b = unreal.SlateBrush()
        b.draw_as = unreal.SlateBrushDrawType.BOX
        b.tint_color = unreal.SlateColor(specified_color=col)
        outline = b.outline_settings
        outline.color = unreal.SlateColor(specified_color=INK)
        outline.width = 2.5
        b.outline_settings = outline
        return b

    style.normal = _brush(base_color)
    style.hovered = _brush(hover_color)
    style.pressed = _brush(press_color)
    style.normal_padding = unreal.Margin(22, 10, 22, 10)
    style.pressed_padding = unreal.Margin(22, 12, 22, 8)
    btn.set_editor_property("widget_style", style)


def box_slot(w, pad=(0, 0, 0, 0), valign=unreal.VerticalAlignment.V_ALIGN_CENTER, halign=None):
    s = w.get_editor_property("slot")
    s.set_padding(unreal.Margin(*pad))
    s.set_vertical_alignment(valign)
    if halign is not None:
        s.set_horizontal_alignment(halign)


def canvas_slot(w, anchor, pos):
    s = w.get_editor_property("slot")
    s.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(*anchor), maximum=unreal.Vector2D(*anchor)))
    s.set_alignment(unreal.Vector2D(*anchor))
    s.set_position(unreal.Vector2D(*pos))
    s.set_auto_size(True)


def fill_canvas_slot(w):
    s = w.get_editor_property("slot")
    s.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)))
    s.set_offsets(unreal.Margin(0, 0, 0, 0))


def add_letterbox(bp, root, bar_height=62):
    for name, ay in (("LetterboxTop", 0.0), ("LetterboxBottom", 1.0)):
        bar = add(bp, unreal.Border, name, root)
        bar.set_editor_property("brush_color", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
        s = bar.get_editor_property("slot")
        s.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, ay), maximum=unreal.Vector2D(1, ay)))
        s.set_alignment(unreal.Vector2D(0, ay))
        s.set_offsets(unreal.Margin(0, 0, 0, bar_height))


def recreate_wbp(name, parent_cls):
    pkg = "%s/%s" % (FOLDER, name)
    if EAL.does_asset_exist(pkg):
        EAL.delete_asset(pkg)
    try:
        bp = SUB.create_widget_blueprint(FOLDER, name, parent_cls)
    except Exception:
        f = unreal.WidgetBlueprintFactory()
        f.set_editor_property("parent_class", parent_cls)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.WidgetBlueprint, f)
    LOG.append("BUILD %s" % name)
    return bp


def finish(bp):
    if not bp:
        return
    ok = SUB.compile_widget_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
    LOG.append("      %s compiled=%s" % (bp.get_name(), ok))


# ---------------------------------------------------------------------------- Widgets

def build_health_bar():
    bp = recreate_wbp("WBP_HealthBar", unreal.DCHealthBarWidget)
    outer, inner = make_framed_panel(bp, "HealthFrame", pad=(10, 8, 12, 8), bg_color=SLATE_BG, gold_thickness=2, ink_thickness=3)
    row = add(bp, unreal.HorizontalBox, "Row", inner)

    add_icon(bp, "HeartIcon", row, "T_DC_UI_HeartIcon", (36, 36), pad=(0, 0, 10, 0))

    col = add(bp, unreal.VerticalBox, "BarColumn", row)
    box_slot(col)

    hdr = add(bp, unreal.TextBlock, "HealthHeader", col)
    style_text(hdr, "ACTOR VITALS", 10, GOLD, font_key="Font_Mono", outline_size=1)
    hdr.get_editor_property("slot").set_padding(unreal.Margin(0, 0, 0, 3))

    sb = add(bp, unreal.SizeBox, "BarBox", col)
    sb.set_width_override(270)
    sb.set_height_override(26)

    bar_ink = add(bp, unreal.Border, "BarInk", sb)
    bar_ink.set_editor_property("brush_color", INK)
    bar_ink.set_editor_property("padding", unreal.Margin(2, 2, 2, 2))

    stack = add(bp, unreal.Overlay, "Stack", bar_ink)
    bar = add(bp, unreal.ProgressBar, "HealthBar", stack)
    bar.set_editor_property("percent", 1.0)
    bar.set_editor_property("fill_color_and_opacity", WHITE)
    pstyle = bar.get_editor_property("widget_style")
    bg_b = unreal.SlateBrush()
    bg_b.draw_as = unreal.SlateBrushDrawType.BOX
    bg_b.tint_color = unreal.SlateColor(specified_color=TRACK_BG)
    fill_b = unreal.SlateBrush()
    fill_b.draw_as = unreal.SlateBrushDrawType.BOX
    fill_b.tint_color = unreal.SlateColor(specified_color=RED)
    pstyle.background_image = bg_b
    pstyle.fill_image = fill_b
    bar.set_editor_property("widget_style", pstyle)

    s = bar.get_editor_property("slot")
    s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    s.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    label = add(bp, unreal.TextBlock, "HealthText", stack)
    style_text(label, "100", 16, CREAM, font_key="Font_Display", outline_size=2)
    s = label.get_editor_property("slot")
    s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    s.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    finish(bp)


def build_quest_tracker():
    bp = recreate_wbp("WBP_QuestTracker", unreal.DCQuestTrackerWidget)
    outer, inner = make_framed_panel(bp, "QuestFrame", pad=(0, 0, 0, 0), bg_color=SLATE_BG, gold_thickness=2, ink_thickness=3)
    vbox = add(bp, unreal.VerticalBox, "SlateStack", inner)

    clapper = add_clapper_strip(bp, "ClapperTop", vbox, height=13)
    clapper.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    body = add(bp, unreal.Border, "SlateBody", vbox)
    body.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 0))
    body.set_editor_property("padding", unreal.Margin(14, 10, 16, 10))

    row = add(bp, unreal.HorizontalBox, "Row", body)
    add_icon(bp, "SlateIcon", row, "T_DC_UI_SlateIcon", (42, 42), pad=(0, 0, 12, 0))

    text_col = add(bp, unreal.VerticalBox, "TextCol", row)
    box_slot(text_col)

    hdr = add(bp, unreal.TextBlock, "ObjectiveTag", text_col)
    style_text(hdr, "DIRECTOR'S SCRIPT  |  ACTIVE CUE", 10, GOLD, font_key="Font_Mono", outline_size=1)
    hdr.get_editor_property("slot").set_padding(unreal.Margin(0, 0, 0, 2))

    line_row = add(bp, unreal.HorizontalBox, "LineRow", text_col)
    title = add(bp, unreal.TextBlock, "TitleText", line_row)
    style_text(title, "Boars killed:", 22, CREAM, font_key="Font_Condensed", outline_size=2)
    box_slot(title)

    count = add(bp, unreal.TextBlock, "CountText", line_row)
    style_text(count, "0/10", 26, GOLD, font_key="Font_Display", outline_size=2)
    box_slot(count, (12, 0, 0, 0))
    count.set_editor_property("render_transform_pivot", unreal.Vector2D(0.5, 0.5))

    finish(bp)


def build_subtitles():
    bp = recreate_wbp("WBP_Subtitles", unreal.DCSubtitlesWidget)
    sb = add(bp, unreal.SizeBox, "Frame")
    sb.set_width_override(940)

    outer, inner = make_framed_panel(bp, "SubtitlePanel", sb, pad=(0, 0, 0, 0), bg_color=SLATE_BG, gold_thickness=2, ink_thickness=3)
    vbox = add(bp, unreal.VerticalBox, "SubStack", inner)

    clapper = add_clapper_strip(bp, "ClapperTop", vbox, height=12)
    clapper.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    body = add(bp, unreal.Border, "SubBody", vbox)
    body.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 0))
    body.set_editor_property("padding", unreal.Margin(16, 12, 18, 14))

    row = add(bp, unreal.HorizontalBox, "Row", body)

    p_ink = add(bp, unreal.Border, "PortraitInk", row)
    p_ink.set_editor_property("brush_color", INK)
    p_ink.set_editor_property("padding", unreal.Margin(2, 2, 2, 2))
    box_slot(p_ink, (0, 0, 16, 0), unreal.VerticalAlignment.V_ALIGN_CENTER)

    portrait = add(bp, unreal.Image, "Portrait", p_ink)
    pbrush = portrait.get_editor_property("brush")
    pbrush.resource_object = TEXTURES["T_DC_Director_Portrait"]
    pbrush.image_size = unreal.Vector2D(96, 96)
    portrait.set_editor_property("brush", pbrush)

    col = add(bp, unreal.VerticalBox, "TextColumn", row)
    cs = col.get_editor_property("slot")
    cs.set_size(unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL))
    cs.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    hdr_row = add(bp, unreal.HorizontalBox, "HeaderRow", col)
    hdr_row.get_editor_property("slot").set_padding(unreal.Margin(0, 0, 0, 4))

    speaker = add(bp, unreal.TextBlock, "SpeakerText", hdr_row)
    style_text(speaker, "THE DIRECTOR", 19, GOLD, font_key="Font_Display", outline_size=2)
    box_slot(speaker)

    on_air = add(bp, unreal.TextBlock, "OnSetBadge", hdr_row)
    style_text(on_air, "   * ON SET", 12, RED_BRIGHT, font_key="Font_Mono", outline_size=1)
    box_slot(on_air)

    line = add(bp, unreal.TextBlock, "LineText", col)
    style_text(line, "And... action.", 23, CREAM, font_key="Font_Body", outline_size=1, wrap=True)

    # Set CDO PortraitTexture so NativeConstruct keeps the Director portrait visible
    SUB.compile_widget_blueprint(bp)
    cdo = unreal.get_default_object(_cls("WBP_Subtitles"))
    cdo.set_editor_property("portrait_texture", TEXTURES["T_DC_Director_Portrait"])
    finish(bp)


def build_hud():
    bp = recreate_wbp("WBP_DCHUD", unreal.DCHUDWidget)
    root = add(bp, unreal.CanvasPanel, "Root")

    # Top-left subtle viewfinder recording pill
    badge_outer, badge_inner = make_framed_panel(bp, "RecBadge", root, pad=(12, 6, 14, 6), bg_color=unreal.LinearColor(0.05, 0.04, 0.05, 0.85), gold_thickness=1, ink_thickness=2)
    canvas_slot(badge_outer, (0, 0), (36, 36))
    rec_row = add(bp, unreal.HorizontalBox, "RecRow", badge_inner)
    dot = add(bp, unreal.Border, "RecDot", rec_row)
    dot.set_editor_property("brush_color", RED_BRIGHT)
    dot.set_editor_property("padding", unreal.Margin(5, 5, 5, 5))
    box_slot(dot, (0, 0, 8, 0))
    take_txt = add(bp, unreal.TextBlock, "TakeText", rec_row)
    style_text(take_txt, "REC  |  SCENE 1: BOAR HUNT", 13, CREAM, font_key="Font_Mono", outline_size=1)
    box_slot(take_txt)

    canvas_slot(add(bp, _cls("WBP_QuestTracker"), "QuestTracker", root), (1, 0), (-36, 36))
    canvas_slot(add(bp, _cls("WBP_HealthBar"), "HealthBar", root), (0, 1), (36, -36))
    canvas_slot(add(bp, _cls("WBP_Subtitles"), "Subtitles", root), (0.5, 1), (0, -44))

    cross = add(bp, unreal.Image, "Crosshair", root)
    cross.set_editor_property("color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 0.9))
    cbrush = cross.get_editor_property("brush")
    cbrush.resource_object = TEXTURES["T_DC_UI_Crosshair"]
    cbrush.image_size = unreal.Vector2D(30, 30)
    cross.set_editor_property("brush", cbrush)
    canvas_slot(cross, (0.5, 0.5), (0, 0))

    finish(bp)


def build_cut_screen():
    bp = recreate_wbp("WBP_CutScreen", unreal.DCVictoryWidget)
    root = add(bp, unreal.CanvasPanel, "Root")

    backdrop = add(bp, unreal.Border, "Backdrop", root)
    backdrop.set_editor_property("brush_color", BACKDROP)
    fill_canvas_slot(backdrop)
    add_letterbox(bp, root, 62)

    card_outer, card_inner = make_framed_panel(bp, "Card", root, pad=(0, 0, 0, 0), bg_color=SLATE_CARD, gold_thickness=3, ink_thickness=4)
    canvas_slot(card_outer, (0.5, 0.5), (0, 0))

    card_vbox = add(bp, unreal.VerticalBox, "CardStack", card_inner)
    add_clapper_strip(bp, "ClapperTop", card_vbox, height=22).get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    body = add(bp, unreal.Border, "CardBody", card_vbox)
    body.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 0))
    body.set_editor_property("padding", unreal.Margin(52, 34, 52, 38))

    col = add(bp, unreal.VerticalBox, "Column", body)

    p_wrap = add(bp, unreal.Border, "DirectorFrame", col)
    p_wrap.set_editor_property("brush_color", GOLD)
    p_wrap.set_editor_property("padding", unreal.Margin(3, 3, 3, 3))
    ps = p_wrap.get_editor_property("slot")
    ps.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    ps.set_padding(unreal.Margin(0, 0, 0, 16))
    p_img = add(bp, unreal.Image, "DirectorPortrait", p_wrap)
    pb = p_img.get_editor_property("brush")
    pb.resource_object = TEXTURES["T_DC_Director_Portrait"]
    pb.image_size = unreal.Vector2D(104, 104)
    p_img.set_editor_property("brush", pb)

    def vtext(name, value, size, color, bottom_pad, font_key="Font_Body", outline=2):
        tb = add(bp, unreal.TextBlock, name, col)
        style_text(tb, value, size, color, font_key=font_key, outline_size=outline, justify=unreal.TextJustify.CENTER)
        s = tb.get_editor_property("slot")
        s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        s.set_padding(unreal.Margin(0, 0, 0, bottom_pad))
        return tb

    vtext("TitleText", "CUT! THAT'S A WRAP!", 52, GOLD, 8, font_key="Font_Display", outline=3)
    vtext("SubtitleText", "SCENE 1 COMPLETE  -  THE DIRECTOR HAS BEEN RECAST", 21, CREAM, 18, font_key="Font_Condensed", outline=2)
    vtext("ReviewText", "\"Terrible performance... five stars.\"  -  The Director  * * * * *", 21, PINK, 18, font_key="Font_Body", outline=1)
    vtext("StatsText", "Boars Eliminated: 0   |   Run Time: 00:00   |   Director Defeated", 15, SILVER, 28, font_key="Font_Mono", outline=1)

    row = add(bp, unreal.HorizontalBox, "ButtonsRow", col)
    row.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

    for bname, lname, label, base_c, hov_c, pre_c in (
        ("PlayAgainButton", "PlayAgainLabel", "PLAY AGAIN", unreal.LinearColor(0.16, 0.52, 0.22, 1), unreal.LinearColor(0.22, 0.68, 0.30, 1), unreal.LinearColor(0.12, 0.38, 0.16, 1)),
        ("QuitButton", "QuitLabel", "QUIT", unreal.LinearColor(0.62, 0.14, 0.14, 1), unreal.LinearColor(0.80, 0.20, 0.20, 1), unreal.LinearColor(0.45, 0.10, 0.10, 1)),
    ):
        btn = add(bp, unreal.Button, bname, row)
        style_button(btn, base_c, hov_c, pre_c)
        lbl = add(bp, unreal.TextBlock, lname, btn)
        style_text(lbl, label, 20, WHITE, font_key="Font_Display", outline_size=2)
        btn.get_editor_property("slot").set_padding(unreal.Margin(14, 0, 14, 0))

    finish(bp)


def build_start_screen():
    bp = recreate_wbp("WBP_StartScreen", unreal.DCStartScreenWidget)
    root = add(bp, unreal.CanvasPanel, "Root")

    backdrop = add(bp, unreal.Border, "Backdrop", root)
    backdrop.set_editor_property("brush_color", unreal.LinearColor(0.015, 0.01, 0.02, 0.68))
    fill_canvas_slot(backdrop)
    add_letterbox(bp, root, 62)

    card_outer, card_inner = make_framed_panel(bp, "Card", root, pad=(0, 0, 0, 0), bg_color=SLATE_CARD, gold_thickness=3, ink_thickness=4)
    canvas_slot(card_outer, (0.5, 0.5), (0, 0))

    card_vbox = add(bp, unreal.VerticalBox, "CardStack", card_inner)
    add_clapper_strip(bp, "ClapperTop", card_vbox, height=22).get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    body = add(bp, unreal.Border, "CardBody", card_vbox)
    body.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 0))
    body.set_editor_property("padding", unreal.Margin(54, 34, 54, 38))

    col = add(bp, unreal.VerticalBox, "Column", body)

    p_wrap = add(bp, unreal.Border, "DirectorFrame", col)
    p_wrap.set_editor_property("brush_color", GOLD)
    p_wrap.set_editor_property("padding", unreal.Margin(3, 3, 3, 3))
    ps = p_wrap.get_editor_property("slot")
    ps.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    ps.set_padding(unreal.Margin(0, 0, 0, 14))
    p_img = add(bp, unreal.Image, "DirectorPortrait", p_wrap)
    pb = p_img.get_editor_property("brush")
    pb.resource_object = TEXTURES["T_DC_Director_Portrait"]
    pb.image_size = unreal.Vector2D(108, 108)
    p_img.set_editor_property("brush", pb)

    def stext(name, value, size, color, bottom_pad, font_key="Font_Body", outline=2):
        tb = add(bp, unreal.TextBlock, name, col)
        style_text(tb, value, size, color, font_key=font_key, outline_size=outline, justify=unreal.TextJustify.CENTER)
        s = tb.get_editor_property("slot")
        s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        s.set_padding(unreal.Margin(0, 0, 0, bottom_pad))
        return tb

    stext("SlateHeader", "PRODUCTION SLATE  |  SCENE 1  |  TAKE 1", 13, RED_BRIGHT, 6, font_key="Font_Mono", outline=1)
    stext("TitleText", "DIRECTOR'S CUT", 58, GOLD, 8, font_key="Font_Display", outline=3)
    stext("SubtitleText", "A TOTALLY NORMAL FANTASY QUEST... UNTIL THE DIRECTOR CHANGES HIS MIND", 20, CREAM, 18, font_key="Font_Condensed", outline=2)
    stext("ControlsText", "WASD Move   |   SPACE Jump   |   LMB Attack   |   R Reload   |   ESC / P Pause", 14, SILVER, 28, font_key="Font_Mono", outline=1)

    row = add(bp, unreal.HorizontalBox, "ButtonsRow", col)
    row.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

    for bname, lname, label, base_c, hov_c, pre_c in (
        ("StartButton", "StartLabel", "ACTION! (START)", unreal.LinearColor(0.16, 0.54, 0.22, 1), unreal.LinearColor(0.22, 0.70, 0.30, 1), unreal.LinearColor(0.12, 0.40, 0.16, 1)),
        ("QuitButton", "QuitLabel", "QUIT", unreal.LinearColor(0.62, 0.14, 0.14, 1), unreal.LinearColor(0.80, 0.20, 0.20, 1), unreal.LinearColor(0.45, 0.10, 0.10, 1)),
    ):
        btn = add(bp, unreal.Button, bname, row)
        style_button(btn, base_c, hov_c, pre_c)
        lbl = add(bp, unreal.TextBlock, lname, btn)
        style_text(lbl, label, 22, WHITE, font_key="Font_Display", outline_size=2)
        btn.get_editor_property("slot").set_padding(unreal.Margin(14, 0, 14, 0))

    finish(bp)


def build_pause_menu():
    bp = recreate_wbp("WBP_PauseMenu", unreal.UDCPauseMenuWidget if hasattr(unreal, "UDCPauseMenuWidget") else unreal.DCPauseMenuWidget)
    root = add(bp, unreal.CanvasPanel, "Root")

    backdrop = add(bp, unreal.Border, "Backdrop", root)
    backdrop.set_editor_property("brush_color", BACKDROP)
    fill_canvas_slot(backdrop)
    add_letterbox(bp, root, 52)

    card_outer, card_inner = make_framed_panel(bp, "Card", root, pad=(0, 0, 0, 0), bg_color=SLATE_CARD, gold_thickness=3, ink_thickness=4)
    canvas_slot(card_outer, (0.5, 0.5), (0, 0))

    card_vbox = add(bp, unreal.VerticalBox, "CardStack", card_inner)
    add_clapper_strip(bp, "ClapperTop", card_vbox, height=20).get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    body = add(bp, unreal.Border, "CardBody", card_vbox)
    body.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 0))
    body.set_editor_property("padding", unreal.Margin(48, 32, 48, 36))

    col = add(bp, unreal.VerticalBox, "Column", body)

    def ptext(name, value, size, color, bottom_pad, font_key="Font_Body", outline=2):
        tb = add(bp, unreal.TextBlock, name, col)
        style_text(tb, value, size, color, font_key=font_key, outline_size=outline, justify=unreal.TextJustify.CENTER)
        s = tb.get_editor_property("slot")
        s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        s.set_padding(unreal.Margin(0, 0, 0, bottom_pad))
        return tb

    ptext("TitleText", "CUT!  -  ON HOLD", 48, GOLD, 10, font_key="Font_Display", outline=3)
    ptext("StatusText", "SCENE 1: BOAR HUNT   |   Boars killed: 0/10", 16, CREAM, 14, font_key="Font_Mono", outline=1)
    ptext("ControlsText", "WASD Move   |   SPACE Jump   |   LMB Attack   |   R Reload   |   ESC / P Resume", 14, SILVER, 26, font_key="Font_Mono", outline=1)

    row = add(bp, unreal.HorizontalBox, "ButtonsRow", col)
    row.get_editor_property("slot").set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

    for bname, lname, label, base_c, hov_c, pre_c in (
        ("ResumeButton", "ResumeLabel", "RESUME", unreal.LinearColor(0.16, 0.54, 0.22, 1), unreal.LinearColor(0.22, 0.70, 0.30, 1), unreal.LinearColor(0.12, 0.40, 0.16, 1)),
        ("RestartButton", "RestartLabel", "RESTART", unreal.LinearColor(0.74, 0.48, 0.12, 1), unreal.LinearColor(0.90, 0.62, 0.18, 1), unreal.LinearColor(0.54, 0.34, 0.08, 1)),
        ("QuitButton", "QuitLabel", "QUIT", unreal.LinearColor(0.62, 0.14, 0.14, 1), unreal.LinearColor(0.80, 0.20, 0.20, 1), unreal.LinearColor(0.45, 0.10, 0.10, 1)),
    ):
        btn = add(bp, unreal.Button, bname, row)
        style_button(btn, base_c, hov_c, pre_c)
        lbl = add(bp, unreal.TextBlock, lname, btn)
        style_text(lbl, label, 20, WHITE, font_key="Font_Display", outline_size=2)
        btn.get_editor_property("slot").set_padding(unreal.Margin(10, 0, 10, 0))

    finish(bp)


def assign_to_blueprints():
    gm = _obj(GAME_MODE)
    gm_gen = unreal.load_class(None, GAME_MODE + "." + GAME_MODE.rsplit("/", 1)[-1] + "_C")
    gm_cdo = unreal.get_default_object(gm_gen)
    gm_cdo.set_editor_property("hud_widget_class", _cls("WBP_DCHUD"))
    gm_cdo.set_editor_property("start_screen_widget_class", _cls("WBP_StartScreen"))
    gm_cdo.set_editor_property("victory_widget_class", _cls("WBP_CutScreen"))
    unreal.BlueprintEditorLibrary.compile_blueprint(gm)
    unreal.EditorLoadingAndSavingUtils.save_packages([gm.get_outermost()], False)
    LOG.append("SET  BP_DCGameMode -> WBP_DCHUD, WBP_StartScreen, WBP_CutScreen")

    pc = _obj(PLAYER_CTRL)
    if pc:
        pc_gen = unreal.load_class(None, PLAYER_CTRL + "." + PLAYER_CTRL.rsplit("/", 1)[-1] + "_C")
        pc_cdo = unreal.get_default_object(pc_gen)
        pc_cdo.set_editor_property("pause_menu_widget_class", _cls("WBP_PauseMenu"))
        unreal.BlueprintEditorLibrary.compile_blueprint(pc)
        unreal.EditorLoadingAndSavingUtils.save_packages([pc.get_outermost()], False)
        LOG.append("SET  BP_DCPlayerController -> WBP_PauseMenu")


def main():
    import_ui_textures()
    import_ui_fonts()

    # Delete parent HUD first so child WBPs can be recreated cleanly without stale references
    for name in ("WBP_DCHUD", "WBP_StartScreen", "WBP_PauseMenu", "WBP_CutScreen", "WBP_Subtitles", "WBP_QuestTracker", "WBP_HealthBar"):
        pkg = "%s/%s" % (FOLDER, name)
        if EAL.does_asset_exist(pkg):
            EAL.delete_asset(pkg)

    build_health_bar()
    build_quest_tracker()
    build_subtitles()
    build_hud()
    build_cut_screen()
    build_start_screen()
    build_pause_menu()
    assign_to_blueprints()

    summary = "\n".join(LOG)
    print(summary)
    return summary


if __name__ == "__main__":
    main()
