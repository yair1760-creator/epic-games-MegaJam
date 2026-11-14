# Director's Cut - creates the plain, functional HUD widget blueprints in /Game/DirectorsCut/UI.
# Logic lives in C++ (Source/EpicGameJamGame/DirectorsCut/UI); these WBPs are only layout, for Designer 1 to restyle.
#
# Run inside the editor (Output Log, Python):
#   exec(open(unreal.Paths.project_dir() + "Scripts/DC_BuildHUDWidgets.py").read())
# Create-only: a widget blueprint that already exists is left untouched, so restyled widgets are never overwritten.
# Delete a WBP first if you really want it rebuilt from scratch.
import unreal

FOLDER = "/Game/DirectorsCut/UI"
GAME_MODE = "/Game/DirectorsCut/Blueprints/BP_DCGameMode"
SUB = unreal.get_editor_subsystem(unreal.WidgetBlueprintEditorSubsystem)
LOG = []

WHITE = unreal.LinearColor(1, 1, 1, 1)
YELLOW = unreal.LinearColor(1.0, 0.85, 0.2, 1)
PANEL = unreal.LinearColor(0, 0, 0, 0.55)
RED = unreal.LinearColor(0.85, 0.1, 0.08, 1)


def _obj(path):
    return unreal.load_object(None, "%s.%s" % (path, path.rsplit("/", 1)[-1]))


def _cls(name):
    return unreal.load_class(None, "%s/%s.%s_C" % (FOLDER, name, name))


def add(bp, cls, name, parent=None):
    info = SUB.create_and_add_widget(bp, cls, name, parent, -1)
    w = info.get_editor_property("widget")
    if not w:
        raise RuntimeError("could not add %s to %s" % (name, bp.get_name()))
    return w


def text(w, value, size, color=WHITE, wrap=False):
    w.set_editor_property("text", unreal.Text(value))
    font = w.get_editor_property("font")
    font.size = size
    w.set_editor_property("font", font)
    w.set_editor_property("color_and_opacity", unreal.SlateColor(specified_color=color))
    w.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
    w.set_editor_property("shadow_color_and_opacity", unreal.LinearColor(0, 0, 0, 0.8))
    if wrap:
        w.set_editor_property("auto_wrap_text", True)


def panel(w, pad):
    w.set_editor_property("brush_color", PANEL)
    w.set_editor_property("padding", unreal.Margin(*pad))


def box_slot(w, pad=(0, 0, 0, 0), valign=unreal.VerticalAlignment.V_ALIGN_CENTER):
    s = w.get_editor_property("slot")
    s.set_padding(unreal.Margin(*pad))
    s.set_vertical_alignment(valign)


def canvas_slot(w, anchor, pos):
    s = w.get_editor_property("slot")
    s.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(*anchor), maximum=unreal.Vector2D(*anchor)))
    s.set_alignment(unreal.Vector2D(*anchor))
    s.set_position(unreal.Vector2D(*pos))
    s.set_auto_size(True)


def new_wbp(name, parent_cls):
    if _obj("%s/%s" % (FOLDER, name)):
        LOG.append("KEEP %s (already exists)" % name)
        return None
    try:
        bp = SUB.create_widget_blueprint(FOLDER, name, parent_cls)
    except Exception:
        f = unreal.WidgetBlueprintFactory()
        f.set_editor_property("parent_class", parent_cls)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.WidgetBlueprint, f)
    LOG.append("NEW  %s" % name)
    return bp


def finish(bp):
    if not bp:
        return
    ok = SUB.compile_widget_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([bp.get_outermost()], False)
    LOG.append("     %s compiled=%s" % (bp.get_name(), ok))


def build_quest_tracker():
    bp = new_wbp("WBP_QuestTracker", unreal.DCQuestTrackerWidget)
    if bp:
        frame = add(bp, unreal.Border, "Frame")
        panel(frame, (16, 8, 16, 8))
        row = add(bp, unreal.HorizontalBox, "Row", frame)
        title = add(bp, unreal.TextBlock, "TitleText", row)
        text(title, "Boars killed:", 22)
        box_slot(title)
        count = add(bp, unreal.TextBlock, "CountText", row)
        text(count, "0/10", 26, YELLOW)
        box_slot(count, (10, 0, 0, 0))
        count.set_editor_property("render_transform_pivot", unreal.Vector2D(0.5, 0.5))
    finish(bp)


def build_health_bar():
    bp = new_wbp("WBP_HealthBar", unreal.DCHealthBarWidget)
    if bp:
        size = add(bp, unreal.SizeBox, "Frame")
        size.set_width_override(320)
        size.set_height_override(30)
        stack = add(bp, unreal.Overlay, "Stack", size)
        bar = add(bp, unreal.ProgressBar, "HealthBar", stack)
        bar.set_editor_property("percent", 1.0)
        bar.set_editor_property("fill_color_and_opacity", RED)
        s = bar.get_editor_property("slot")
        s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        s.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
        label = add(bp, unreal.TextBlock, "HealthText", stack)
        text(label, "100", 16)
        s = label.get_editor_property("slot")
        s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        s.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    finish(bp)


def build_subtitles():
    bp = new_wbp("WBP_Subtitles", unreal.DCSubtitlesWidget)
    if bp:
        size = add(bp, unreal.SizeBox, "Frame")
        size.set_width_override(900)
        bg = add(bp, unreal.Border, "Background", size)
        panel(bg, (20, 14, 20, 14))
        row = add(bp, unreal.HorizontalBox, "Row", bg)
        portrait = add(bp, unreal.Image, "Portrait", row)
        brush = portrait.get_editor_property("brush")
        brush.image_size = unreal.Vector2D(96, 96)
        portrait.set_editor_property("brush", brush)
        box_slot(portrait, (0, 0, 16, 0), unreal.VerticalAlignment.V_ALIGN_TOP)
        column = add(bp, unreal.VerticalBox, "TextColumn", row)
        s = column.get_editor_property("slot")
        s.set_size(unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL))
        speaker = add(bp, unreal.TextBlock, "SpeakerText", column)
        text(speaker, "THE DIRECTOR", 18, YELLOW)
        speaker.get_editor_property("slot").set_padding(unreal.Margin(0, 0, 0, 4))
        line = add(bp, unreal.TextBlock, "LineText", column)
        text(line, "And... action.", 24, WHITE, wrap=True)
    finish(bp)


def build_hud():
    bp = new_wbp("WBP_DCHUD", unreal.DCHUDWidget)
    if bp:
        root = add(bp, unreal.CanvasPanel, "Root")
        canvas_slot(add(bp, _cls("WBP_QuestTracker"), "QuestTracker", root), (1, 0), (-40, 40))
        canvas_slot(add(bp, _cls("WBP_HealthBar"), "HealthBar", root), (0, 1), (40, -40))
        canvas_slot(add(bp, _cls("WBP_Subtitles"), "Subtitles", root), (0.5, 1), (0, -80))
        cross = add(bp, unreal.Image, "Crosshair", root)
        cross.set_editor_property("color_and_opacity", unreal.LinearColor(1, 1, 1, 0.8))
        brush = cross.get_editor_property("brush")
        brush.image_size = unreal.Vector2D(6, 6)
        cross.set_editor_property("brush", brush)
        canvas_slot(cross, (0.5, 0.5), (0, 0))
    finish(bp)


def build_cut_screen():
    bp = new_wbp("WBP_CutScreen", unreal.DCVictoryWidget)
    if bp:
        root = add(bp, unreal.CanvasPanel, "Root")
        backdrop = add(bp, unreal.Border, "Backdrop", root)
        backdrop.set_editor_property("brush_color", unreal.LinearColor(0.02, 0.01, 0.03, 0.88))
        bs = backdrop.get_editor_property("slot")
        bs.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)))
        bs.set_offsets(unreal.Margin(0, 0, 0, 0))

        for name, ay in (("LetterboxTop", 0.0), ("LetterboxBottom", 1.0)):
            bar = add(bp, unreal.Border, name, root)
            bar.set_editor_property("brush_color", unreal.LinearColor(0, 0, 0, 1))
            s = bar.get_editor_property("slot")
            s.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, ay), maximum=unreal.Vector2D(1, ay)))
            s.set_alignment(unreal.Vector2D(0, ay))
            s.set_offsets(unreal.Margin(0, 0, 0, 68))

        card = add(bp, unreal.Border, "Card", root)
        card.set_editor_property("brush_color", unreal.LinearColor(0.07, 0.06, 0.09, 0.96))
        card.set_editor_property("padding", unreal.Margin(56, 42, 56, 42))
        canvas_slot(card, (0.5, 0.5), (0, 0))

        col = add(bp, unreal.VerticalBox, "Column", card)

        def vtext(name, value, size, color, bottom_pad):
            tb = add(bp, unreal.TextBlock, name, col)
            text(tb, value, size, color)
            tb.set_editor_property("justification", unreal.TextJustify.CENTER)
            s = tb.get_editor_property("slot")
            s.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
            s.set_padding(unreal.Margin(0, 0, 0, bottom_pad))
            return tb

        vtext("TitleText", "CUT! THAT'S A WRAP!", 52, YELLOW, 10)
        vtext("SubtitleText", "SCENE 1 COMPLETE  -  THE DIRECTOR HAS BEEN RECAST", 20, WHITE, 20)
        vtext("ReviewText", "\"Terrible performance... five stars.\"  -  The Director  * * * * *", 22, unreal.LinearColor(1.0, 0.65, 0.8, 1), 22)
        vtext("StatsText", "Boars Eliminated: 0   |   Run Time: 00:00   |   Director Defeated", 18, unreal.LinearColor(0.8, 0.85, 0.9, 1), 30)

        row = add(bp, unreal.HorizontalBox, "ButtonsRow", col)
        rs = row.get_editor_property("slot")
        rs.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

        for bname, lname, label, bg in (
            ("PlayAgainButton", "PlayAgainLabel", "  PLAY AGAIN  ", unreal.LinearColor(0.18, 0.58, 0.25, 1)),
            ("QuitButton", "QuitLabel", "  QUIT  ", unreal.LinearColor(0.65, 0.14, 0.14, 1)),
        ):
            btn = add(bp, unreal.Button, bname, row)
            btn.set_editor_property("background_color", bg)
            lbl = add(bp, unreal.TextBlock, lname, btn)
            text(lbl, label, 20, WHITE)
            btn.get_editor_property("slot").set_padding(unreal.Margin(14, 0, 14, 0))
    finish(bp)


def assign_to_game_mode():
    gm = _obj(GAME_MODE)
    gen = unreal.load_class(None, GAME_MODE + "." + GAME_MODE.rsplit("/", 1)[-1] + "_C")
    cdo = unreal.get_default_object(gen)
    changed = False
    if cdo.get_editor_property("hud_widget_class"):
        LOG.append("KEEP BP_DCGameMode HUD widget class")
    else:
        cdo.set_editor_property("hud_widget_class", _cls("WBP_DCHUD"))
        LOG.append("SET  BP_DCGameMode HUD widget class = WBP_DCHUD")
        changed = True
    if cdo.get_editor_property("victory_widget_class"):
        LOG.append("KEEP BP_DCGameMode Victory widget class")
    else:
        cdo.set_editor_property("victory_widget_class", _cls("WBP_CutScreen"))
        LOG.append("SET  BP_DCGameMode Victory widget class = WBP_CutScreen")
        changed = True
    if changed:
        unreal.BlueprintEditorLibrary.compile_blueprint(gm)
        unreal.EditorLoadingAndSavingUtils.save_packages([gm.get_outermost()], False)


build_quest_tracker()
build_health_bar()
build_subtitles()
build_hud()
build_cut_screen()
assign_to_game_mode()
print("\n".join(LOG))
"\n".join(LOG)
