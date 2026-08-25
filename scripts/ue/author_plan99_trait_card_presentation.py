"""Clean Trait Card WBPs, add designer samples, and skin the confirm button.

The two WBP assets remain the layout authority. Runtime code supplies card
content and state, but no longer supplies obsolete helper copy. Each authored
card slot contains a sample Entry instance so layout work is WYSIWYG; runtime
replaces that sample with the actual offer.
"""

import unreal


ENTRY_ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry"
CHOICE_ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice"
CONFIRM_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/"
    "T_UI_Pause_ButtonLight"
)
SAMPLE_ART_PATH = "/Game/ReEcho/Textures/UI/Cards/Art/T_UI_CardTier2"
SAMPLE_ICON_PATH = "/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_G_1_01"
OBSOLETE_ENTRY_WIDGETS = (
    "ArtCardFrame",
    "ArtCardImage",
    "ArtTagPrimary",
    "ArtTagSecondary",
    "PrimaryTagText",
    "SecondaryTagText",
    "KickerText",
    "SelectHintText",
    "CardContent",
)
OBSOLETE_CHOICE_WIDGETS = ("SubtitleText", "CurrencyText", "NeedleWidget")
SAMPLE_CARD_NAME = "元素会心"
SAMPLE_CARD_DESCRIPTION = "每当你或回响造成伤害时，距离每增加1m，本次伤害获得3%伤害增幅。"
ENTRY_DESIGNER_CANVAS_NAME = "CardDesignerCanvas"
ENTRY_ROOT_SIZE_BOX_NAME = "CardRootSizeBox"
ENTRY_ROOT_SCALE_BOX_NAME = "CardRootScaleBox"
ENTRY_DESIGN_WIDTH = 420.0
ENTRY_DESIGN_HEIGHT = 593.0
ENTRY_TEXT_LAYOUTS = {
    "NameText": (20.0, 272.0, 304.0, 58.0, 2),
    "DescriptionText": (23.0, 342.0, 298.0, 120.0, 2),
}


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def save_compiled(toolset, blueprint, asset_path):
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError(f"Failed to compile Trait Card widget: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Trait Card widget: {asset_path}")


def add_widget(toolset, blueprint, widget_class, name, parent, child_index=-1):
    info = toolset.call_method(
        "AddWidget", args=(blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        raise RuntimeError(f"Failed to add Trait Card widget: {name}")
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


def capture_canvas_layout(source_slot):
    if not isinstance(source_slot, unreal.CanvasPanelSlot):
        raise RuntimeError("Source widget does not have a CanvasPanelSlot")
    source_layout = source_slot.get_editor_property("layout_data")
    source_offsets = source_layout.get_editor_property("offsets")
    source_anchors = source_layout.get_editor_property("anchors")
    source_alignment = source_layout.get_editor_property("alignment")
    return (
        source_offsets.left,
        source_offsets.top,
        source_offsets.right,
        source_offsets.bottom,
        source_anchors.minimum.x,
        source_anchors.minimum.y,
        source_anchors.maximum.x,
        source_anchors.maximum.y,
        source_alignment.x,
        source_alignment.y,
        source_slot.get_editor_property("auto_size"),
        source_slot.get_editor_property("z_order"),
    )


def apply_canvas_layout(snapshot, target_widget, z_offset=0):
    (
        left,
        top,
        right,
        bottom,
        anchor_min_x,
        anchor_min_y,
        anchor_max_x,
        anchor_max_y,
        alignment_x,
        alignment_y,
        auto_size,
        z_order,
    ) = snapshot
    target_slot = target_widget.get_editor_property("slot")
    if not isinstance(target_slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{target_widget.get_name()} did not receive a CanvasPanelSlot")
    target_slot.set_editor_property(
        "layout_data",
        unreal.AnchorData(
            offsets=unreal.Margin(
                left,
                top,
                right,
                bottom,
            ),
            anchors=unreal.Anchors(
                minimum=unreal.Vector2D(anchor_min_x, anchor_min_y),
                maximum=unreal.Vector2D(anchor_max_x, anchor_max_y),
            ),
            alignment=unreal.Vector2D(alignment_x, alignment_y),
        ),
    )
    target_slot.set_editor_property("auto_size", auto_size)
    target_slot.set_editor_property("z_order", z_order + z_offset)


def copy_canvas_layout(source_slot, target_widget, z_offset=0):
    apply_canvas_layout(capture_canvas_layout(source_slot), target_widget, z_offset)


toolset = unreal.UMGToolSet.get_default_object()
entry_blueprint = unreal.load_asset(ENTRY_ASSET_PATH)
choice_blueprint = unreal.load_asset(CHOICE_ASSET_PATH)
confirm_texture = unreal.load_asset(CONFIRM_TEXTURE_PATH)
sample_art = unreal.load_asset(SAMPLE_ART_PATH)
sample_icon = unreal.load_asset(SAMPLE_ICON_PATH)
if entry_blueprint is None or choice_blueprint is None:
    raise RuntimeError("Required Trait Card WBP is missing")
if not isinstance(confirm_texture, unreal.Texture2D):
    raise RuntimeError(f"Required confirm texture is missing: {CONFIRM_TEXTURE_PATH}")
if not isinstance(sample_art, unreal.Texture2D) or not isinstance(sample_icon, unreal.Texture2D):
    raise RuntimeError("Required Trait Card designer sample texture is missing")

entry_widgets = widget_map(toolset, entry_blueprint)
removed = []
for widget_name in OBSOLETE_ENTRY_WIDGETS:
    widget = entry_widgets.get(widget_name)
    if widget is None:
        unreal.log(f"[Plan99TraitAuthor] already absent: {widget_name}")
        continue
    if not toolset.call_method("RemoveWidget", args=(entry_blueprint, widget)):
        raise RuntimeError(f"Failed to remove obsolete Trait Card widget: {widget_name}")
    removed.append(widget_name)

entry_widgets = widget_map(toolset, entry_blueprint)
name_text = entry_widgets.get("NameText")
description_text = entry_widgets.get("DescriptionText")
art_image = entry_widgets.get("ArtImage")
icon_image = entry_widgets.get("IconImage")
entry_overlay = entry_widgets.get("Overlay_0")
select_button = entry_widgets.get("SelectButton")
if not isinstance(name_text, unreal.TextBlock) or not isinstance(description_text, unreal.TextBlock):
    raise RuntimeError("Trait Card entry is missing production sample text widgets")
if not isinstance(art_image, unreal.Image) or not isinstance(icon_image, unreal.Image):
    raise RuntimeError("Trait Card entry is missing production sample image widgets")
if not isinstance(entry_overlay, unreal.Overlay):
    raise RuntimeError("Trait Card entry is missing its authored presentation overlay")
if not isinstance(select_button, unreal.Button):
    raise RuntimeError("Trait Card entry is missing its authored selection button")

root_size_box = entry_widgets.get(ENTRY_ROOT_SIZE_BOX_NAME)
if root_size_box is None:
    if select_button.get_parent() is not None:
        raise RuntimeError("Trait Card SelectButton has an unexpected parent before root wrapping")
    wrappers = toolset.call_method(
        "WrapWidgets", args=(entry_blueprint, [select_button], unreal.SizeBox)
    )
    if len(wrappers) != 1 or wrappers[0].widget is None:
        raise RuntimeError("Failed to wrap Trait Card entry in a fixed root SizeBox")
    rename_info = toolset.call_method(
        "RenameWidget", args=(entry_blueprint, wrappers[0].widget, ENTRY_ROOT_SIZE_BOX_NAME)
    )
    root_size_box = rename_info.widget
if not isinstance(root_size_box, unreal.SizeBox):
    raise RuntimeError("Trait Card fixed root must be a SizeBox")
root_size_box.set_width_override(ENTRY_DESIGN_WIDTH)
root_size_box.set_height_override(ENTRY_DESIGN_HEIGHT)

root_scale_box = entry_widgets.get(ENTRY_ROOT_SCALE_BOX_NAME)
if root_scale_box is None:
    if root_size_box.get_parent() is not None:
        raise RuntimeError("Trait Card fixed design surface has an unexpected parent before scale wrapping")
    wrappers = toolset.call_method(
        "WrapWidgets", args=(entry_blueprint, [root_size_box], unreal.ScaleBox)
    )
    if len(wrappers) != 1 or wrappers[0].widget is None:
        raise RuntimeError("Failed to wrap Trait Card design surface in a root ScaleBox")
    rename_info = toolset.call_method(
        "RenameWidget", args=(entry_blueprint, wrappers[0].widget, ENTRY_ROOT_SCALE_BOX_NAME)
    )
    root_scale_box = rename_info.widget
if not isinstance(root_scale_box, unreal.ScaleBox):
    raise RuntimeError("Trait Card proportional root must be a ScaleBox")
if (
    root_scale_box.get_parent() is not None
    or root_size_box.get_parent() is not root_scale_box
    or select_button.get_parent() is not root_size_box
):
    raise RuntimeError("Trait Card proportional root hierarchy is invalid")
root_scale_box.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
root_scale_box.set_editor_property("stretch_direction", unreal.StretchDirection.BOTH)

design_surface_slot = root_size_box.get_editor_property("slot")
if not isinstance(design_surface_slot, unreal.ScaleBoxSlot):
    raise RuntimeError("Trait Card fixed design surface did not receive a ScaleBoxSlot")
design_surface_slot.set_editor_property(
    "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
)
design_surface_slot.set_editor_property(
    "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER
)

select_slot = select_button.get_editor_property("slot")
if not isinstance(select_slot, unreal.SizeBoxSlot):
    raise RuntimeError("Trait Card SelectButton did not receive a SizeBoxSlot")
select_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
select_slot.set_editor_property(
    "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
)
select_slot.set_editor_property(
    "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
)

overlay_slot = entry_overlay.get_editor_property("slot")
if not isinstance(overlay_slot, unreal.ButtonSlot):
    raise RuntimeError("Trait Card Overlay_0 did not retain its ButtonSlot")
overlay_slot.set_editor_property(
    "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
)
overlay_slot.set_editor_property(
    "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
)

designer_canvas = entry_widgets.get(ENTRY_DESIGNER_CANVAS_NAME)
if designer_canvas is None:
    designer_canvas = add_widget(
        toolset,
        entry_blueprint,
        unreal.CanvasPanel,
        ENTRY_DESIGNER_CANVAS_NAME,
        entry_overlay,
        -1,
    )
    canvas_host_slot = designer_canvas.get_editor_property("slot")
    if not isinstance(canvas_host_slot, unreal.OverlaySlot):
        raise RuntimeError("CardDesignerCanvas did not receive an OverlaySlot")
    canvas_host_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    canvas_host_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
    )
    canvas_host_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
    )
if not isinstance(designer_canvas, unreal.CanvasPanel):
    raise RuntimeError("CardDesignerCanvas must be a CanvasPanel")
designer_canvas.set_editor_property(
    "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
)

for text_name, layout in ENTRY_TEXT_LAYOUTS.items():
    text_widget = entry_widgets[text_name]
    if text_widget.get_parent() is not designer_canvas:
        moved = toolset.call_method(
            "MoveWidget", args=(entry_blueprint, text_widget, designer_canvas, -1)
        )
        if moved.widget is None:
            raise RuntimeError(f"Failed to move {text_name} to CardDesignerCanvas")
        set_canvas_layout(text_widget, *layout)
    text_widget.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

legacy_icon_parent = icon_image.get_parent()
if legacy_icon_parent is not designer_canvas:
    legacy_icon_slot = icon_image.get_editor_property("slot")
    legacy_icon_layout = capture_canvas_layout(legacy_icon_slot)
    moved = toolset.call_method(
        "MoveWidget", args=(entry_blueprint, icon_image, designer_canvas, -1)
    )
    if moved.widget is None:
        raise RuntimeError("Failed to move IconImage to CardDesignerCanvas")
    apply_canvas_layout(legacy_icon_layout, icon_image)

if (
    legacy_icon_parent is not None
    and legacy_icon_parent is not designer_canvas
    and legacy_icon_parent.get_name() == "CanvasPanel_217"
    and legacy_icon_parent.get_children_count() == 0
):
    if not toolset.call_method("RemoveWidget", args=(entry_blueprint, legacy_icon_parent)):
        raise RuntimeError("Failed to remove the empty legacy card canvas")
name_text.set_editor_property("text", SAMPLE_CARD_NAME)
description_text.set_editor_property("text", SAMPLE_CARD_DESCRIPTION)
for image_widget, texture in ((art_image, sample_art), (icon_image, sample_icon)):
    brush = image_widget.get_editor_property("brush")
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    image_widget.set_editor_property("brush", brush)
    image_widget.set_editor_property("visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)

save_compiled(toolset, entry_blueprint, ENTRY_ASSET_PATH)
entry_widgets = widget_map(toolset, entry_blueprint)
for widget_name in OBSOLETE_ENTRY_WIDGETS:
    if widget_name in entry_widgets:
        raise RuntimeError(f"Obsolete Trait Card widget survived cleanup: {widget_name}")
for required_name in ("SelectButton", "ArtImage", "NameText", "DescriptionText", "IconImage"):
    if required_name not in entry_widgets:
        raise RuntimeError(f"Trait Card presentation lost required widget: {required_name}")
root_size_box = entry_widgets.get(ENTRY_ROOT_SIZE_BOX_NAME)
root_scale_box = entry_widgets.get(ENTRY_ROOT_SCALE_BOX_NAME)
select_button = entry_widgets.get("SelectButton")
if (
    not isinstance(root_scale_box, unreal.ScaleBox)
    or root_scale_box.get_parent() is not None
    or not isinstance(root_size_box, unreal.SizeBox)
    or root_size_box.get_parent() is not root_scale_box
    or select_button.get_parent() is not root_size_box
    or not isinstance(select_button.get_editor_property("slot"), unreal.SizeBoxSlot)
):
    raise RuntimeError("Trait Card proportional 420x593 design root was not retained")
if "CanvasPanel_217" in entry_widgets:
    raise RuntimeError("Empty legacy Trait Card canvas survived migration")
for text_name in ENTRY_TEXT_LAYOUTS:
    text_widget = entry_widgets[text_name]
    if text_widget.get_parent().get_name() != ENTRY_DESIGNER_CANVAS_NAME or not isinstance(
        text_widget.get_editor_property("slot"), unreal.CanvasPanelSlot
    ):
        raise RuntimeError(f"Trait Card text is not freely draggable: {text_name}")

choice_widgets = widget_map(toolset, choice_blueprint)
for widget_name in OBSOLETE_CHOICE_WIDGETS:
    widget = choice_widgets.get(widget_name)
    if widget is None:
        unreal.log(f"[Plan99TraitAuthor] already absent: {widget_name}")
        continue
    if not toolset.call_method("RemoveWidget", args=(choice_blueprint, widget)):
        raise RuntimeError(f"Failed to remove obsolete Trait Card choice widget: {widget_name}")
    removed.append(widget_name)

choice_widgets = widget_map(toolset, choice_blueprint)
entry_widget_class = unreal.load_class(
    None,
    "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry.WBP_ReEchoTraitCardEntry_C",
)
if entry_widget_class is None:
    raise RuntimeError("Compiled Trait Card entry class is missing")
for slot_index in range(3):
    slot_name = f"TraitCardSlot{slot_index}"
    sample_name = f"DesignerTraitCardSample{slot_index}"
    card_slot = choice_widgets.get(slot_name)
    if not isinstance(card_slot, unreal.SizeBox):
        raise RuntimeError(f"Trait Card choice is missing designer slot: {slot_name}")
    children = [
        info.widget
        for info in toolset.call_method("GetWidgets", args=(choice_blueprint,)).widgets
        if info.widget and info.widget.get_parent() == card_slot
    ]
    existing_sample = choice_widgets.get(sample_name)
    if existing_sample is not None:
        if existing_sample.get_parent() != card_slot:
            raise RuntimeError(f"Designer sample has the wrong parent: {sample_name}")
        continue
    if children:
        raise RuntimeError(
            f"Refusing to replace unexpected authored content in {slot_name}: "
            + ",".join(child.get_name() for child in children)
        )
    sample_info = toolset.call_method(
        "AddWidget",
        args=(choice_blueprint, entry_widget_class, sample_name, card_slot, -1),
    )
    if sample_info.widget is None:
        raise RuntimeError(f"Failed to add designer sample: {sample_name}")

choice_widgets = widget_map(toolset, choice_blueprint)
confirm_button = choice_widgets.get("ConfirmButton")
confirm_label = choice_widgets.get("ConfirmButtonLabel")
choice_root = choice_widgets.get("RootPanel")
if not isinstance(confirm_button, unreal.Button):
    raise RuntimeError("WBP_ReEchoTraitCardChoice is missing ConfirmButton")
if not isinstance(confirm_label, unreal.TextBlock):
    raise RuntimeError("WBP_ReEchoTraitCardChoice is missing ConfirmButtonLabel")
if not isinstance(choice_root, unreal.CanvasPanel):
    raise RuntimeError("WBP_ReEchoTraitCardChoice is missing RootPanel")

if confirm_label.get_parent() is not choice_root:
    confirm_button_slot = confirm_button.get_editor_property("slot")
    moved = toolset.call_method(
        "MoveWidget", args=(choice_blueprint, confirm_label, choice_root, -1)
    )
    if moved.widget is None:
        raise RuntimeError("Failed to move ConfirmButtonLabel to RootPanel")
    copy_canvas_layout(confirm_button_slot, confirm_label, z_offset=1)

style = confirm_button.get_editor_property("widget_style")
for brush_name in ("normal", "hovered", "pressed", "disabled"):
    brush = style.get_editor_property(brush_name)
    brush.set_editor_property("resource_object", confirm_texture)
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    brush.set_editor_property(
        "tint_color",
        unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0)),
    )
    style.set_editor_property(brush_name, brush)
confirm_button.set_editor_property("widget_style", style)
confirm_button.set_editor_property("background_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
confirm_label.set_editor_property("text", "确定")
confirm_label.set_editor_property("justification", unreal.TextJustify.CENTER)
confirm_label.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)

save_compiled(toolset, choice_blueprint, CHOICE_ASSET_PATH)
choice_widgets = widget_map(toolset, choice_blueprint)
for widget_name in OBSOLETE_CHOICE_WIDGETS:
    if widget_name in choice_widgets:
        raise RuntimeError(f"Obsolete Trait Card choice widget survived cleanup: {widget_name}")
for slot_index in range(3):
    sample_name = f"DesignerTraitCardSample{slot_index}"
    if sample_name not in choice_widgets:
        raise RuntimeError(f"Trait Card designer sample was not retained: {sample_name}")
for text_name in ("TitleText", "ConfirmButtonLabel"):
    text_widget = choice_widgets.get(text_name)
    if not isinstance(text_widget, unreal.TextBlock) or not isinstance(
        text_widget.get_editor_property("slot"), unreal.CanvasPanelSlot
    ):
        raise RuntimeError(f"Choice text is not freely draggable: {text_name}")
confirm_button = choice_widgets.get("ConfirmButton")
style = confirm_button.get_editor_property("widget_style")
for brush_name in ("normal", "hovered", "pressed", "disabled"):
    brush = style.get_editor_property(brush_name)
    resource = brush.get_editor_property("resource_object")
    if resource != confirm_texture:
        raise RuntimeError(f"ConfirmButton {brush_name} brush did not retain the delivered texture")

unreal.log(
    f"[Plan99TraitAuthor] removed={','.join(removed) or '<already-clean>'}; "
    f"confirm={confirm_texture.get_path_name()}"
)
