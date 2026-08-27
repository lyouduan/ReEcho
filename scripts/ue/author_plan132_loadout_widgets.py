"""Author the Plan132 two-stage Loadout selection WBPs."""

import csv
from pathlib import Path

import unreal


SELECTION_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"
ENTRY_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry"
TOOLTIP_DIR = "/Game/ReEcho/UI"
TOOLTIP_NAME = "WBP_ReEchoLoadoutTooltip"
TOOLTIP_PATH = f"{TOOLTIP_DIR}/{TOOLTIP_NAME}"
TOOLTIP_PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoLoadoutTooltipWidget"
DESCRIPTION_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_DescriptionPanel"
)
ARROW_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_SelectionArrow"
)
CONFIRM_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
    "T_UI_Pause_ButtonLight"
)
SAMPLE_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/"
    "T_UI_Loadout_Character_J_HEART_Selected"
)
FORMAL_FONT_PATH = (
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W_Font"
)
CHARACTERS_CSV_PATH = (
    Path(unreal.Paths.project_content_dir()) / "Data" / "characters.csv"
)
CHARACTER_PREVIEW_ORDER = (
    ("J_HEART", "勇者"),
    ("J_SPADE", "智者"),
    ("J_CLOVER", "诗人"),
    ("J_DIAMOND", "猎手"),
)
WEAPON_PREVIEW_ORDER = (
    ("W_J_04", "镰刀"),
    ("W_J_09", "枪"),
    ("W_J_01", "剑"),
    ("W_J_08", "弓"),
)
LEGACY_SELECTION_ARROWS = (
    "CharacterSelectionArrow0",
    "CharacterSelectionArrow1",
    "CharacterSelectionArrow2",
    "SelectionArrow",
    "WeaponSelectionArrow0",
    "WeaponSelectionArrow1",
    "WeaponSelectionArrow2",
    "WeaponSelectionArrow3",
)


def load_tooltip_preview_content():
    with CHARACTERS_CSV_PATH.open("r", encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            if row.get("Id") == "J_HEART":
                title = row.get("DisplayName", "").strip()
                description = row.get("Description", "").strip()
                if title and description:
                    return title, description
    raise RuntimeError(
        f"Plan132 tooltip preview row J_HEART is missing: {CHARACTERS_CSV_PATH}"
    )


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def ensure_tooltip_blueprint():
    blueprint = (
        unreal.EditorAssetLibrary.load_asset(TOOLTIP_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(TOOLTIP_PATH)
        else None
    )
    if blueprint is None:
        parent_class = unreal.load_class(None, TOOLTIP_PARENT_CLASS_PATH)
        if parent_class is None:
            raise RuntimeError(
                f"Plan132 tooltip parent class is missing: {TOOLTIP_PARENT_CLASS_PATH}"
            )
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            TOOLTIP_NAME, TOOLTIP_DIR, unreal.WidgetBlueprint, factory
        )
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Plan132 tooltip asset has the wrong type: {TOOLTIP_PATH}")
    return blueprint


def mark_variable(toolset, blueprint, widget):
    info = next(
        (
            candidate
            for candidate in widget_infos(toolset, blueprint)
            if candidate.widget is widget
        ),
        None,
    )
    if info is None:
        raise RuntimeError(f"Unable to find WidgetInfo for {widget.get_name()}")
    if not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def add_widget(toolset, blueprint, widget_class, name, parent, variable=True):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, -1)
    )
    if info.widget is None:
        raise RuntimeError(f"Failed to add Plan132 widget: {name}")
    if variable:
        mark_variable(toolset, blueprint, info.widget)
    return info.widget


def set_canvas_layout(widget, x, y, width, height, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(x, y, width, height),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(0.0, 0.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def set_canvas_fill(widget, z_order):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a direct CanvasPanel child")
    slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(0.0, 0.0, 0.0, 0.0),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(0.0, 0.0),
                maximum=unreal.Vector2D(1.0, 1.0),
            ),
            alignment=unreal.Vector2D(0.0, 0.0),
        ),
    )
    slot.set_editor_property("auto_size", False)
    slot.set_editor_property("z_order", z_order)


def set_panel_slot_fill(widget):
    slot = widget.get_editor_property("slot")
    if slot is None:
        raise RuntimeError(f"{widget.get_name()} has no panel slot")
    if hasattr(slot, "set_editor_property"):
        for property_name, value in (
            ("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0)),
            ("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL),
            ("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL),
        ):
            try:
                slot.set_editor_property(property_name, value)
            except Exception:
                pass


def configure_text(text, value, size, color, justification):
    text.set_editor_property("text", value)
    text.set_editor_property("color_and_opacity", unreal.SlateColor(color))
    text.set_editor_property("justification", justification)
    text.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)


def configure_formal_font(text, font_asset, outline_size=0):
    font = text.get_editor_property("font")
    font.set_editor_property("font_object", font_asset)
    outline = font.get_editor_property("outline_settings")
    outline.set_editor_property("outline_size", outline_size)
    outline.set_editor_property(
        "outline_color", unreal.LinearColor(0.15, 0.08, 0.045, 1.0)
    )
    font.set_editor_property("outline_settings", outline)
    text.set_editor_property("font", font)


def configure_image(image, texture, visibility=unreal.SlateVisibility.HIT_TEST_INVISIBLE):
    brush = image.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    image.set_editor_property("brush", brush)
    image.set_editor_property("visibility", visibility)


def configure_transparent_button(button):
    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 0.0)
    )


def configure_confirm_button(button, texture):
    style = button.get_editor_property("widget_style")
    for brush_name in ("normal", "hovered", "pressed", "disabled"):
        brush = style.get_editor_property(brush_name)
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        brush.set_editor_property(
            "tint_color",
            unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
        )
        style.set_editor_property(brush_name, brush)
    button.set_editor_property("widget_style", style)
    button.set_editor_property(
        "background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )


def load_preview_texture(domain, stable_id, state):
    asset_name = f"T_UI_Loadout_{domain}_{stable_id}_{state}"
    asset_path = f"/Game/ReEcho/Textures/UI/LoadoutSelection/{asset_name}"
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Plan132 preview texture is missing: {asset_path}")
    return texture


def ensure_selection_preview_entries(
    toolset, blueprint, entry_widget_class, character_row, weapon_row
):
    expected_names = {
        *(f"CharacterEntry{index}" for index in range(4)),
        *(f"WeaponEntry{index}" for index in range(4)),
    }
    widgets = widget_map(toolset, blueprint)
    for widget in list(widgets.values()):
        if (
            widget.get_parent() in (character_row, weapon_row)
            and widget.get_name() not in expected_names
        ):
            if not toolset.call_method("RemoveWidget", args=(blueprint, widget)):
                raise RuntimeError(
                    f"Failed to remove obsolete Loadout preview child: {widget.get_name()}"
                )

    for domain, row, ordered_entries, width in (
        ("Character", character_row, CHARACTER_PREVIEW_ORDER, 390.0),
        ("Weapon", weapon_row, WEAPON_PREVIEW_ORDER, 280.0),
    ):
        for index, (stable_id, label) in enumerate(ordered_entries):
            name = f"{domain}Entry{index}"
            widgets = widget_map(toolset, blueprint)
            entry = widgets.get(name)
            if entry is not None and entry.get_class() != entry_widget_class:
                if not toolset.call_method("RemoveWidget", args=(blueprint, entry)):
                    raise RuntimeError(f"Failed to replace Loadout preview entry: {name}")
                entry = None
            if entry is None:
                entry = add_widget(
                    toolset,
                    blueprint,
                    entry_widget_class,
                    name,
                    row,
                    False,
                )
            entry.set_editor_property("designer_preview_label", label)
            entry.set_editor_property(
                "designer_preview_selected_texture",
                load_preview_texture(domain, stable_id, "Selected"),
            )
            entry.set_editor_property(
                "designer_preview_unselected_texture",
                load_preview_texture(domain, stable_id, "Unselected"),
            )
            entry.set_editor_property("designer_preview_width", width)
            entry.set_editor_property("designer_preview_height", 560.0)
            entry.set_editor_property("designer_preview_selected", index == 3)
            slot = entry.get_editor_property("slot")
            if not isinstance(slot, unreal.HorizontalBoxSlot):
                raise RuntimeError(f"Plan132 preview entry has wrong slot: {name}")
            slot.set_editor_property("padding", unreal.Margin(7.0, 7.0, 7.0, 7.0))
            slot.set_editor_property(
                "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
            )
            slot.set_editor_property(
                "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
            )


def remove_legacy_selection_arrows(toolset, blueprint):
    widgets = widget_map(toolset, blueprint)
    for name in LEGACY_SELECTION_ARROWS:
        arrow = widgets.get(name)
        if arrow is not None and not toolset.call_method(
            "RemoveWidget", args=(blueprint, arrow)
        ):
            raise RuntimeError(f"Failed to remove legacy Loadout arrow: {name}")


def ensure_stage_switcher(toolset, blueprint, design_canvas, character_stage, weapon_stage):
    widgets = widget_map(toolset, blueprint)
    stage_switcher = widgets.get("StageSwitcher")
    created = stage_switcher is None
    if created:
        stage_switcher = add_widget(
            toolset, blueprint, unreal.WidgetSwitcher, "StageSwitcher", design_canvas
        )
    elif not isinstance(stage_switcher, unreal.WidgetSwitcher):
        raise RuntimeError("Plan132 StageSwitcher has the wrong type")
    if stage_switcher.get_parent() is not design_canvas:
        moved = toolset.call_method(
            "MoveWidget", args=(blueprint, stage_switcher, design_canvas, -1)
        )
        if moved.widget is None:
            raise RuntimeError("Plan132 failed to move StageSwitcher to the design canvas")
        stage_switcher = moved.widget
    set_canvas_fill(stage_switcher, 10)
    for index, stage in enumerate((character_stage, weapon_stage)):
        if stage.get_parent() is not stage_switcher:
            moved = toolset.call_method(
                "MoveWidget", args=(blueprint, stage, stage_switcher, index)
            )
            if moved.widget is None:
                raise RuntimeError(f"Plan132 failed to move stage into switcher: {stage.get_name()}")
            stage = moved.widget
        set_panel_slot_fill(stage)
        stage.set_editor_property(
            "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
        )
    if created:
        stage_switcher.set_active_widget_index(0)
    mark_variable(toolset, blueprint, stage_switcher)
    return stage_switcher


def configure_tooltip_frame(frame, frame_texture):
    background = frame.get_editor_property("background")
    background.set_editor_property("resource_object", frame_texture)
    background.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    background.set_editor_property(
        "margin", unreal.Margin(2.0 / 230.0, 2.0 / 134.0, 2.0 / 230.0, 2.0 / 134.0)
    )
    background.set_editor_property(
        "tint_color",
        unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
    )
    frame.set_editor_property("background", background)
    frame.set_editor_property(
        "brush_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )


def author_tooltip(
    toolset, blueprint, frame_texture, preview_title, preview_description
):
    widgets = widget_map(toolset, blueprint)
    root = widgets.get("TooltipRootSizeBox")
    if root is None:
        roots = [widget for widget in widgets.values() if widget.get_parent() is None]
        for old_root in roots:
            if not toolset.call_method("RemoveWidget", args=(blueprint, old_root)):
                raise RuntimeError(
                    f"Failed to remove legacy tooltip root: {old_root.get_name()}"
                )
        root = add_widget(
            toolset,
            blueprint,
            unreal.SizeBox,
            "TooltipRootSizeBox",
            None,
        )
        frame = add_widget(
            toolset, blueprint, unreal.Border, "TooltipFrame", root
        )
        surface = add_widget(
            toolset, blueprint, unreal.Border, "TooltipSurface", frame
        )
        content = add_widget(
            toolset, blueprint, unreal.VerticalBox, "TooltipContent", surface
        )
        title = add_widget(
            toolset, blueprint, unreal.TextBlock, "TitleText", content
        )
        description = add_widget(
            toolset, blueprint, unreal.TextBlock, "DescriptionText", content
        )
    widgets = widget_map(toolset, blueprint)
    required = {
        "TooltipRootSizeBox": unreal.SizeBox,
        "TooltipFrame": unreal.Border,
        "TooltipSurface": unreal.Border,
        "TooltipContent": unreal.VerticalBox,
        "TitleText": unreal.TextBlock,
        "DescriptionText": unreal.TextBlock,
    }
    for name, expected_type in required.items():
        widget = widgets.get(name)
        if not isinstance(widget, expected_type):
            raise RuntimeError(f"Plan132 Tooltip binding {name} is missing or wrong")
        mark_variable(toolset, blueprint, widget)

    root = widgets["TooltipRootSizeBox"]
    frame = widgets["TooltipFrame"]
    surface = widgets["TooltipSurface"]
    title = widgets["TitleText"]
    description = widgets["DescriptionText"]
    root.set_width_override(380.0)
    configure_tooltip_frame(frame, frame_texture)
    frame.set_editor_property("padding", unreal.Margin(3.0, 3.0, 3.0, 3.0))
    surface.set_editor_property(
        "brush_color", unreal.LinearColor(0.015, 0.015, 0.015, 0.97)
    )
    surface.set_editor_property(
        "padding", unreal.Margin(18.0, 14.0, 18.0, 14.0)
    )
    configure_text(
        title,
        preview_title,
        22,
        unreal.LinearColor(1.0, 0.96, 0.88, 1.0),
        unreal.TextJustify.CENTER,
    )
    title.set_editor_property("auto_wrap_text", False)
    title_slot = title.get_editor_property("slot")
    title_slot.set_editor_property(
        "padding", unreal.Margin(0.0, 0.0, 0.0, 10.0)
    )
    configure_text(
        description,
        preview_description,
        20,
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
        unreal.TextJustify.LEFT,
    )
    description.set_editor_property("auto_wrap_text", True)
    description.set_editor_property("wrap_text_at", 338.0)
    for widget in (frame, surface, widgets["TooltipContent"], title, description):
        set_panel_slot_fill(widget)

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Tooltip WBP failed to compile")
    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError("Plan132 Tooltip WBP has no generated class")
    default_object = unreal.get_default_object(generated_class)
    blueprint.modify()
    default_object.call_method("ApplyDesignerPreviewSettings")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Tooltip WBP failed to save")


def author_entry(toolset, blueprint, sample_texture, arrow_texture, formal_font):
    widgets = widget_map(toolset, blueprint)
    select_button = widgets.get("SelectButton")
    entry_content = widgets.get("EntryContent")
    portrait_size = widgets.get("PortraitSize")
    portrait_scale = widgets.get("PortraitScale")
    portrait_image = widgets.get("PortraitImage")
    name_text = widgets.get("NameText")
    if not isinstance(select_button, unreal.Button):
        raise RuntimeError("Plan132 Entry lost SelectButton")
    if not isinstance(entry_content, unreal.VerticalBox):
        raise RuntimeError("Plan132 Entry lost EntryContent")
    if not isinstance(portrait_size, unreal.SizeBox):
        raise RuntimeError("Plan132 Entry lost PortraitSize")
    if not isinstance(portrait_scale, unreal.ScaleBox):
        raise RuntimeError("Plan132 Entry lost PortraitScale")
    if not isinstance(portrait_image, unreal.Image):
        raise RuntimeError("Plan132 Entry lost PortraitImage")
    if not isinstance(name_text, unreal.TextBlock):
        raise RuntimeError("Plan132 Entry lost NameText")

    root_size = widgets.get("EntryRootSizeBox")
    if root_size is None:
        if select_button.get_parent() is not None:
            raise RuntimeError("Plan132 Entry SelectButton has an unexpected parent")
        wrappers = toolset.call_method(
            "WrapWidgets", args=(blueprint, [select_button], unreal.SizeBox)
        )
        if len(wrappers) != 1 or wrappers[0].widget is None:
            raise RuntimeError("Failed to wrap Plan132 Entry in SizeBox")
        renamed = toolset.call_method(
            "RenameWidget",
            args=(blueprint, wrappers[0].widget, "EntryRootSizeBox"),
        )
        root_size = renamed.widget
    if not isinstance(root_size, unreal.SizeBox):
        raise RuntimeError("Plan132 Entry root is not SizeBox")

    widgets = widget_map(toolset, blueprint)
    visual_overlay = widgets.get("EntryVisualOverlay")
    if visual_overlay is None:
        if select_button.get_parent() is not root_size:
            raise RuntimeError("Plan132 Entry SelectButton cannot be wrapped safely")
        wrappers = toolset.call_method(
            "WrapWidgets", args=(blueprint, [select_button], unreal.Overlay)
        )
        if len(wrappers) != 1 or wrappers[0].widget is None:
            raise RuntimeError("Failed to create Plan132 Entry hover visual root")
        renamed = toolset.call_method(
            "RenameWidget",
            args=(blueprint, wrappers[0].widget, "EntryVisualOverlay"),
        )
        visual_overlay = renamed.widget
    if not isinstance(visual_overlay, unreal.Overlay):
        raise RuntimeError("Plan132 EntryVisualOverlay has the wrong type")
    if select_button.get_parent() is not visual_overlay:
        raise RuntimeError("Plan132 SelectButton is outside its hover visual root")

    widgets = widget_map(toolset, blueprint)
    selection_arrow = widgets.get("EntrySelectionArrow")
    if selection_arrow is None:
        selection_arrow = add_widget(
            toolset,
            blueprint,
            unreal.Image,
            "EntrySelectionArrow",
            visual_overlay,
        )
    if not isinstance(selection_arrow, unreal.Image):
        raise RuntimeError("Plan132 EntrySelectionArrow has the wrong type")
    if selection_arrow.get_parent() is not visual_overlay:
        raise RuntimeError("Plan132 EntrySelectionArrow is outside its hover visual root")

    mark_variable(toolset, blueprint, root_size)
    mark_variable(toolset, blueprint, visual_overlay)
    mark_variable(toolset, blueprint, select_button)
    mark_variable(toolset, blueprint, portrait_size)
    mark_variable(toolset, blueprint, portrait_image)
    mark_variable(toolset, blueprint, name_text)
    mark_variable(toolset, blueprint, selection_arrow)

    root_size.set_width_override(390.0)
    root_size.set_height_override(560.0)
    portrait_size.set_width_override(390.0)
    portrait_size.set_height_override(480.0)
    portrait_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    portrait_scale.set_editor_property(
        "stretch_direction", unreal.StretchDirection.BOTH
    )
    configure_image(
        selection_arrow, arrow_texture, unreal.SlateVisibility.HIT_TEST_INVISIBLE
    )
    configure_transparent_button(select_button)
    set_panel_slot_fill(visual_overlay)
    set_panel_slot_fill(select_button)
    set_panel_slot_fill(entry_content)
    set_panel_slot_fill(portrait_size)
    set_panel_slot_fill(portrait_scale)
    set_panel_slot_fill(name_text)
    portrait_image_slot = portrait_image.get_editor_property("slot")
    if not isinstance(portrait_image_slot, unreal.ScaleBoxSlot):
        raise RuntimeError("Plan132 PortraitImage does not have a ScaleBox slot")
    portrait_image_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
    )
    portrait_image_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER
    )
    arrow_slot = selection_arrow.get_editor_property("slot")
    if not isinstance(arrow_slot, unreal.OverlaySlot):
        raise RuntimeError("Plan132 EntrySelectionArrow does not have an Overlay slot")
    arrow_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
    )
    arrow_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_BOTTOM
    )
    arrow_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    configure_image(portrait_image, sample_texture)
    configure_text(
        name_text,
        "勇者",
        42,
        unreal.LinearColor(1.0, 0.956, 0.882, 1.0),
        unreal.TextJustify.CENTER,
    )
    configure_formal_font(name_text, formal_font, 1)

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Entry WBP failed to compile")
    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError("Plan132 Entry WBP has no generated class")
    default_object = unreal.get_default_object(generated_class)
    blueprint.modify()
    default_object.call_method("ApplyDesignerPreviewSettings")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Entry WBP failed to save")


def author_selection(
    toolset,
    blueprint,
    entry_widget_class,
    description_texture,
    confirm_texture,
    formal_font,
):
    widgets = widget_map(toolset, blueprint)
    root = widgets.get("CanvasPanel_0")
    if not isinstance(root, unreal.CanvasPanel) or root.get_parent() is not None:
        raise RuntimeError("Plan132 Selection lost stable CanvasPanel_0 root")

    design_canvas = widgets.get("LoadoutDesignCanvas")
    if design_canvas is None:
        old_children = [
            widget
            for widget in widgets.values()
            if widget.get_parent() is root
        ]
        for child in old_children:
            if not toolset.call_method("RemoveWidget", args=(blueprint, child)):
                raise RuntimeError(
                    f"Failed to remove legacy Loadout root child: {child.get_name()}"
                )

        background = add_widget(
            toolset, blueprint, unreal.Border, "LoadoutBackground", root, False
        )
        set_canvas_fill(background, 0)
        background.set_editor_property(
            "brush_color", unreal.LinearColor(0.0, 0.0, 0.0, 1.0)
        )
        background.set_editor_property(
            "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
        )

        responsive_scale = add_widget(
            toolset, blueprint, unreal.ScaleBox, "ResponsiveContentScale", root, False
        )
        set_canvas_fill(responsive_scale, 10)
        responsive_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
        responsive_scale.set_editor_property(
            "stretch_direction", unreal.StretchDirection.BOTH
        )

        responsive_size = add_widget(
            toolset,
            blueprint,
            unreal.SizeBox,
            "ResponsiveContentSize",
            responsive_scale,
            False,
        )
        responsive_size.set_width_override(1920.0)
        responsive_size.set_height_override(1080.0)
        set_panel_slot_fill(responsive_size)

        design_canvas = add_widget(
            toolset,
            blueprint,
            unreal.CanvasPanel,
            "LoadoutDesignCanvas",
            responsive_size,
            False,
        )
        set_panel_slot_fill(design_canvas)

        character_stage = add_widget(
            toolset,
            blueprint,
            unreal.CanvasPanel,
            "CharacterStagePanel",
            design_canvas,
        )
        set_canvas_layout(character_stage, 0.0, 0.0, 1920.0, 1080.0, 10)
        character_stage.set_editor_property(
            "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
        )
        character_row = add_widget(
            toolset,
            blueprint,
            unreal.HorizontalBox,
            "CharacterRow",
            character_stage,
        )
        set_canvas_layout(character_row, 152.0, 278.0, 1616.0, 560.0, 10)

        weapon_stage = add_widget(
            toolset,
            blueprint,
            unreal.CanvasPanel,
            "WeaponStagePanel",
            design_canvas,
        )
        set_canvas_layout(weapon_stage, 0.0, 0.0, 1920.0, 1080.0, 20)
        weapon_stage.set_editor_property(
            "visibility", unreal.SlateVisibility.COLLAPSED
        )
        weapon_row = add_widget(
            toolset,
            blueprint,
            unreal.HorizontalBox,
            "WeaponRow",
            weapon_stage,
        )
        set_canvas_layout(weapon_row, 372.0, 278.0, 1176.0, 560.0, 10)

        title = add_widget(
            toolset, blueprint, unreal.TextBlock, "TitleText", design_canvas
        )
        set_canvas_layout(title, 650.0, 112.0, 620.0, 120.0, 50)
        configure_text(
            title,
            "选择角色",
            72,
            unreal.LinearColor(1.0, 0.956, 0.882, 1.0),
            unreal.TextJustify.CENTER,
        )

        description_panel = add_widget(
            toolset,
            blueprint,
            unreal.Image,
            "DescriptionPanel",
            design_canvas,
        )
        set_canvas_layout(description_panel, 423.0, 370.0, 420.0, 440.0, 60)
        configure_image(
            description_panel, description_texture, unreal.SlateVisibility.COLLAPSED
        )

        description_scale = add_widget(
            toolset,
            blueprint,
            unreal.ScaleBox,
            "DescriptionTextScale",
            design_canvas,
            False,
        )
        set_canvas_layout(description_scale, 443.0, 390.0, 380.0, 400.0, 70)
        description_scale.set_editor_property("stretch", unreal.Stretch.NONE)
        description_scale.set_editor_property(
            "stretch_direction", unreal.StretchDirection.DOWN_ONLY
        )
        description_text = add_widget(
            toolset,
            blueprint,
            unreal.TextBlock,
            "DescriptionText",
            description_scale,
        )
        configure_text(
            description_text,
            "角色与武器说明由运行时数据填充",
            20,
            unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
            unreal.TextJustify.LEFT,
        )
        description_text.set_editor_property("auto_wrap_text", True)
        description_text.set_editor_property("wrap_text_at", 380.0)
        description_text.set_editor_property(
            "visibility", unreal.SlateVisibility.COLLAPSED
        )
        set_panel_slot_fill(description_text)

        confirm_button = add_widget(
            toolset,
            blueprint,
            unreal.Button,
            "ConfirmButton",
            design_canvas,
        )
        set_canvas_layout(confirm_button, 800.0, 880.0, 370.0, 100.0, 90)
        configure_confirm_button(confirm_button, confirm_texture)
        confirm_button.set_editor_property(
            "visibility", unreal.SlateVisibility.COLLAPSED
        )

        confirm_label = add_widget(
            toolset,
            blueprint,
            unreal.TextBlock,
            "ConfirmButtonLabel",
            design_canvas,
        )
        set_canvas_layout(confirm_label, 800.0, 898.0, 370.0, 64.0, 100)
        configure_text(
            confirm_label,
            "确认",
            42,
            unreal.LinearColor(1.0, 0.956, 0.882, 1.0),
            unreal.TextJustify.CENTER,
        )
        confirm_label.set_editor_property(
            "visibility", unreal.SlateVisibility.COLLAPSED
        )

        status_text = add_widget(
            toolset, blueprint, unreal.TextBlock, "StatusText", design_canvas
        )
        set_canvas_layout(status_text, 0.0, 0.0, 1.0, 1.0, 0)
        status_text.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

    remove_legacy_selection_arrows(toolset, blueprint)
    widgets = widget_map(toolset, blueprint)
    ensure_stage_switcher(
        toolset,
        blueprint,
        design_canvas,
        widgets["CharacterStagePanel"],
        widgets["WeaponStagePanel"],
    )
    widgets = widget_map(toolset, blueprint)
    required = {
        "StageSwitcher": unreal.WidgetSwitcher,
        "CharacterStagePanel": unreal.CanvasPanel,
        "WeaponStagePanel": unreal.CanvasPanel,
        "CharacterRow": unreal.HorizontalBox,
        "WeaponRow": unreal.HorizontalBox,
        "TitleText": unreal.TextBlock,
        "DescriptionPanel": unreal.Image,
        "DescriptionText": unreal.TextBlock,
        "ConfirmButton": unreal.Button,
        "ConfirmButtonLabel": unreal.TextBlock,
        "StatusText": unreal.TextBlock,
    }
    widgets = widget_map(toolset, blueprint)
    for name, expected_type in required.items():
        widget = widgets.get(name)
        if not isinstance(widget, expected_type):
            raise RuntimeError(f"Plan132 Selection binding {name} is missing or wrong")
        mark_variable(toolset, blueprint, widget)

    description_scale = widgets.get("DescriptionTextScale")
    if not isinstance(description_scale, unreal.ScaleBox):
        raise RuntimeError("Plan132 Selection binding DescriptionTextScale is missing")
    mark_variable(toolset, blueprint, description_scale)

    ensure_selection_preview_entries(
        toolset,
        blueprint,
        entry_widget_class,
        widgets["CharacterRow"],
        widgets["WeaponRow"],
    )

    title = widgets["TitleText"]
    configure_text(
        title,
        "选择角色",
        72,
        unreal.LinearColor(1.0, 0.956, 0.882, 1.0),
        unreal.TextJustify.CENTER,
    )
    configure_formal_font(title, formal_font, 3)
    title.set_editor_property("shadow_offset", unreal.Vector2D(2.0, 3.0))
    title.set_editor_property(
        "shadow_color_and_opacity", unreal.LinearColor(0.0, 0.0, 0.0, 0.85)
    )
    set_canvas_layout(title, 650.0, 112.0, 620.0, 120.0, 50)

    confirm_button = widgets["ConfirmButton"]
    confirm_button.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

    confirm_label = widgets["ConfirmButtonLabel"]
    configure_formal_font(confirm_label, formal_font, 1)
    name_color = unreal.LinearColor(1.0, 0.956, 0.882, 1.0)
    confirm_label.set_editor_property("color_and_opacity", unreal.SlateColor(name_color))
    confirm_label.set_editor_property(
        "visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE
    )

    set_canvas_layout(widgets["DescriptionPanel"], 423.0, 370.0, 420.0, 440.0, 60)
    set_canvas_layout(description_scale, 443.0, 390.0, 380.0, 400.0, 70)
    description_scale.set_editor_property("stretch", unreal.Stretch.NONE)
    description_text = widgets["DescriptionText"]
    description_font = description_text.get_editor_property("font")
    description_font.size = 20
    description_text.set_editor_property("font", description_font)
    description_text.set_editor_property("wrap_text_at", 380.0)

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Selection WBP failed to compile")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Selection WBP failed to save")


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    selection = unreal.load_asset(SELECTION_PATH)
    entry = unreal.load_asset(ENTRY_PATH)
    tooltip = ensure_tooltip_blueprint()
    description_texture = unreal.load_asset(DESCRIPTION_TEXTURE_PATH)
    arrow_texture = unreal.load_asset(ARROW_TEXTURE_PATH)
    confirm_texture = unreal.load_asset(CONFIRM_TEXTURE_PATH)
    sample_texture = unreal.load_asset(SAMPLE_TEXTURE_PATH)
    formal_font = unreal.load_asset(FORMAL_FONT_PATH)
    preview_title, preview_description = load_tooltip_preview_content()
    if not isinstance(selection, unreal.WidgetBlueprint) or not isinstance(
        entry, unreal.WidgetBlueprint
    ):
        raise RuntimeError("Plan132 required WidgetBlueprint is missing")
    for texture, path in (
        (description_texture, DESCRIPTION_TEXTURE_PATH),
        (arrow_texture, ARROW_TEXTURE_PATH),
        (confirm_texture, CONFIRM_TEXTURE_PATH),
        (sample_texture, SAMPLE_TEXTURE_PATH),
    ):
        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Plan132 required texture is missing: {path}")
    if not isinstance(formal_font, unreal.Font):
        raise RuntimeError(f"Plan132 required font is missing: {FORMAL_FONT_PATH}")

    author_tooltip(
        toolset,
        tooltip,
        description_texture,
        preview_title,
        preview_description,
    )
    author_entry(toolset, entry, sample_texture, arrow_texture, formal_font)
    entry_widget_class = entry.generated_class()
    if entry_widget_class is None:
        raise RuntimeError("Plan132 Entry WBP has no generated class")
    author_selection(
        toolset,
        selection,
        entry_widget_class,
        description_texture,
        confirm_texture,
        formal_font,
    )
    unreal.log(
        "[Plan132LoadoutAuthor] tooltip=compiled entry=compiled selection=compiled"
    )


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
