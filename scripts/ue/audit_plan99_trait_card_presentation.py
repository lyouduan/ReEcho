"""Audit the authored Trait Card entry/choice presentation contract."""

import unreal


ASSET_PATHS = (
    "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry",
    "/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice",
)
ENTRY_OBSOLETE_WIDGETS = {
    "ArtCardFrame",
    "ArtCardImage",
    "ArtTagPrimary",
    "ArtTagSecondary",
    "PrimaryTagText",
    "SecondaryTagText",
    "CardContent",
    "KickerText",
    "SelectHintText",
    "CanvasPanel_217",
}
CHOICE_OBSOLETE_WIDGETS = {"SubtitleText", "CurrencyText", "NeedleWidget"}


def brush_resource(widget):
    if isinstance(widget, unreal.Image):
        brush = widget.get_editor_property("brush")
        resource = brush.get_editor_property("resource_object")
        return resource.get_path_name() if resource else "<none>"
    if isinstance(widget, unreal.Button):
        style = widget.get_editor_property("widget_style")
        normal = style.get_editor_property("normal")
        resource = normal.get_editor_property("resource_object")
        return resource.get_path_name() if resource else "<none>"
    return "<not-brush-widget>"


def brush_metrics(widget):
    brush = None
    if isinstance(widget, unreal.Image):
        brush = widget.get_editor_property("brush")
    elif isinstance(widget, unreal.Button):
        style = widget.get_editor_property("widget_style")
        brush = style.get_editor_property("normal")
    if brush is None:
        return ""
    image_size = brush.get_editor_property("image_size")
    resource = brush.get_editor_property("resource_object")
    source_size = ""
    if isinstance(resource, unreal.Texture2D):
        source_size = f" source_size=({resource.blueprint_get_size_x()},{resource.blueprint_get_size_y()})"
    return f" brush_size={image_size}{source_size}"


toolset = unreal.UMGToolSet.get_default_object()
for asset_path in ASSET_PATHS:
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Missing Trait Card widget: {asset_path}")
    unreal.log(f"[Plan99TraitAudit] asset={asset_path}")
    widget_infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    widget_names = {info.widget.get_name() for info in widget_infos if info.widget}
    for index, info in enumerate(widget_infos):
        widget = info.widget
        if widget is None:
            continue
        parent = widget.get_parent()
        slot = widget.get_editor_property("slot")
        geometry = ""
        if isinstance(slot, unreal.CanvasPanelSlot):
            layout = slot.get_editor_property("layout_data")
            offsets = layout.get_editor_property("offsets")
            anchors = layout.get_editor_property("anchors")
            alignment = layout.get_editor_property("alignment")
            geometry = (
                f" offsets=({offsets.left:.1f},{offsets.top:.1f},{offsets.right:.1f},{offsets.bottom:.1f})"
                f" anchors=({anchors.minimum.x:.2f},{anchors.minimum.y:.2f})"
                f"-({anchors.maximum.x:.2f},{anchors.maximum.y:.2f})"
                f" alignment=({alignment.x:.2f},{alignment.y:.2f})"
            )
        elif isinstance(slot, (unreal.OverlaySlot, unreal.ButtonSlot, unreal.SizeBoxSlot)):
            padding = slot.get_editor_property("padding")
            horizontal = slot.get_editor_property("horizontal_alignment")
            vertical = slot.get_editor_property("vertical_alignment")
            geometry = (
                f" padding=({padding.left:.1f},{padding.top:.1f},"
                f"{padding.right:.1f},{padding.bottom:.1f})"
                f" halign={horizontal} valign={vertical}"
            )
        unreal.log(
            "[Plan99TraitAudit] "
            f"{index:03d} name={widget.get_name()} type={widget.get_class().get_name()} "
            f"parent={parent.get_name() if parent else '<root>'} "
            f"visibility={widget.get_editor_property('visibility')} "
            f"slot={slot.get_class().get_name() if slot else '<none>'} "
            f"resource={brush_resource(widget)}{brush_metrics(widget)}{geometry}"
        )
    if asset_path.endswith("TraitCardEntry"):
        survivors = sorted(ENTRY_OBSOLETE_WIDGETS & widget_names)
        if survivors:
            raise RuntimeError(f"Obsolete Trait Card entry widgets remain: {','.join(survivors)}")
        for required_name in ("SelectButton", "ArtImage", "NameText", "DescriptionText", "IconImage"):
            if required_name not in widget_names:
                raise RuntimeError(f"Trait Card entry lost required widget: {required_name}")
        widgets = {info.widget.get_name(): info.widget for info in widget_infos if info.widget}
        root_scale_box = widgets.get("CardRootScaleBox")
        root_size_box = widgets.get("CardRootSizeBox")
        select_button = widgets.get("SelectButton")
        if not isinstance(root_scale_box, unreal.ScaleBox):
            raise RuntimeError("Trait Card proportional root ScaleBox is missing")
        if not isinstance(root_size_box, unreal.SizeBox):
            raise RuntimeError("Trait Card fixed design surface SizeBox is missing")
        if (
            root_scale_box.get_parent() is not None
            or root_size_box.get_parent() is not root_scale_box
            or select_button.get_parent() is not root_size_box
        ):
            raise RuntimeError("Trait Card proportional root hierarchy is invalid")
        if root_scale_box.get_editor_property("stretch") != unreal.Stretch.SCALE_TO_FIT:
            raise RuntimeError("Trait Card root does not preserve its authored aspect ratio")
        design_surface_slot = root_size_box.get_editor_property("slot")
        if (
            not isinstance(design_surface_slot, unreal.ScaleBoxSlot)
            or design_surface_slot.get_editor_property("horizontal_alignment")
            != unreal.HorizontalAlignment.H_ALIGN_CENTER
            or design_surface_slot.get_editor_property("vertical_alignment")
            != unreal.VerticalAlignment.V_ALIGN_CENTER
        ):
            raise RuntimeError("Trait Card design surface is still stretched by its ScaleBox slot")
        if abs(root_size_box.get_editor_property("width_override") - 420.0) > 0.01 or abs(
            root_size_box.get_editor_property("height_override") - 593.0
        ) > 0.01:
            raise RuntimeError("Trait Card design surface does not match the imported 420x593 card art")
        art_resource = widgets["ArtImage"].get_editor_property("brush").get_editor_property(
            "resource_object"
        )
        if (
            not isinstance(art_resource, unreal.Texture2D)
            or art_resource.blueprint_get_size_x() != 420
            or art_resource.blueprint_get_size_y() != 593
        ):
            raise RuntimeError("Trait Card sample art no longer matches the 420x593 design surface")
        select_slot = select_button.get_editor_property("slot")
        if not isinstance(select_slot, unreal.SizeBoxSlot):
            raise RuntimeError("Trait Card SelectButton is not constrained by the root SizeBox")
        overlay_slot = widgets["Overlay_0"].get_editor_property("slot")
        if (
            not isinstance(overlay_slot, unreal.ButtonSlot)
            or overlay_slot.get_editor_property("horizontal_alignment")
            != unreal.HorizontalAlignment.H_ALIGN_FILL
            or overlay_slot.get_editor_property("vertical_alignment")
            != unreal.VerticalAlignment.V_ALIGN_FILL
        ):
            raise RuntimeError("Trait Card authored overlay does not fill the fixed card root")
        designer_canvas = widgets.get("CardDesignerCanvas")
        if not isinstance(designer_canvas, unreal.CanvasPanel):
            raise RuntimeError("Trait Card designer canvas is missing")
        for text_name in ("NameText", "DescriptionText"):
            text_widget = widgets[text_name]
            if text_widget.get_parent() is not designer_canvas or not isinstance(
                text_widget.get_editor_property("slot"), unreal.CanvasPanelSlot
            ):
                raise RuntimeError(f"Trait Card text is not freely draggable: {text_name}")
        if not str(widgets["NameText"].get_editor_property("text")).strip():
            raise RuntimeError("Trait Card designer sample name is empty")
        if not str(widgets["DescriptionText"].get_editor_property("text")).strip():
            raise RuntimeError("Trait Card designer sample description is empty")
        if brush_resource(widgets["ArtImage"]) == "<none>":
            raise RuntimeError("Trait Card designer sample art is empty")
    else:
        survivors = sorted(CHOICE_OBSOLETE_WIDGETS & widget_names)
        if survivors:
            raise RuntimeError(f"Obsolete Trait Card choice widgets remain: {','.join(survivors)}")
        for slot_index in range(3):
            slot_name = f"TraitCardSlot{slot_index}"
            sample_name = f"DesignerTraitCardSample{slot_index}"
            if slot_name not in widget_names or sample_name not in widget_names:
                raise RuntimeError(f"Trait Card designer sample is missing from slot {slot_index}")
        widgets = {info.widget.get_name(): info.widget for info in widget_infos if info.widget}
        for text_name in ("TitleText", "ConfirmButtonLabel"):
            text_widget = widgets.get(text_name)
            if not isinstance(text_widget, unreal.TextBlock) or not isinstance(
                text_widget.get_editor_property("slot"), unreal.CanvasPanelSlot
            ):
                raise RuntimeError(f"Trait Card choice text is not freely draggable: {text_name}")
