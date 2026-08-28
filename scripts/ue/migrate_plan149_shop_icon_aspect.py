"""Wrap authored shop icons in centered ScaleBoxes so textures keep aspect ratio."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


def widget_infos(toolset, blueprint):
    return toolset.call_method("GetWidgets", args=(blueprint,)).widgets


def widget_map(toolset, blueprint):
    return {
        str(info.widget_name): info.widget
        for info in widget_infos(toolset, blueprint)
        if info.widget
    }


def mark_variable(toolset, blueprint, widget):
    info = next(
        (
            candidate
            for candidate in widget_infos(toolset, blueprint)
            if candidate.widget is widget
        ),
        None,
    )
    if info is not None and not info.get_editor_property("is_variable"):
        toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, widget, True))


def configure_scale(scale, image):
    scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)
    scale.set_editor_property("stretch_direction", unreal.StretchDirection.BOTH)
    scale.set_editor_property(
        "visibility", unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    )
    image_slot = image.get_editor_property("slot")
    if not isinstance(image_slot, unreal.ScaleBoxSlot):
        raise RuntimeError(f"{image.get_name()} is not inside a ScaleBox")
    image_slot.set_editor_property(
        "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER
    )
    image_slot.set_editor_property(
        "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER
    )


def wrap_image(toolset, blueprint, image_name, scale_name, hide_when_empty=False):
    widgets = widget_map(toolset, blueprint)
    image = widgets.get(image_name)
    if not isinstance(image, unreal.Image):
        raise RuntimeError(f"Missing shop image: {image_name}")

    scale = widgets.get(scale_name)
    if scale is None:
        wrappers = toolset.call_method(
            "WrapWidgets", args=(blueprint, [image], unreal.ScaleBox)
        )
        if len(wrappers) != 1 or wrappers[0].widget is None:
            raise RuntimeError(f"Failed to wrap {image_name}")
        renamed = toolset.call_method(
            "RenameWidget", args=(blueprint, wrappers[0].widget, scale_name)
        )
        scale = renamed.widget
    if not isinstance(scale, unreal.ScaleBox) or image.get_parent() is not scale:
        raise RuntimeError(f"{image_name} has an invalid aspect-fit wrapper")

    mark_variable(toolset, blueprint, scale)
    mark_variable(toolset, blueprint, image)
    configure_scale(scale, image)
    if hide_when_empty:
        image.set_editor_property("visibility", unreal.SlateVisibility.HIDDEN)
    parent_slot = scale.get_editor_property("slot")
    if isinstance(parent_slot, unreal.ButtonSlot):
        parent_slot.set_editor_property("padding", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        parent_slot.set_editor_property(
            "horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL
        )
        parent_slot.set_editor_property(
            "vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL
        )


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(WIDGET_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing shop widget: {WIDGET_PATH}")

    blueprint.modify()
    wrapped = []
    for index in range(3):
        wrap_image(
            toolset,
            blueprint,
            f"DesignerPartOfferIcon{index}",
            f"DesignerPartOfferIconScale{index}",
        )
        wrapped.append(f"DesignerPartOfferIcon{index}")
    for index in range(3):
        wrap_image(
            toolset,
            blueprint,
            f"DesignerAttachmentSlotArt{index}",
            f"DesignerAttachmentSlotScale{index}",
            True,
        )
        wrapped.append(f"DesignerAttachmentSlotArt{index}")
    for index in range(5):
        wrap_image(
            toolset,
            blueprint,
            f"DesignerDualAttachmentSlotArt{index}",
            f"DesignerDualAttachmentSlotScale{index}",
            True,
        )
        wrapped.append(f"DesignerDualAttachmentSlotArt{index}")
    wrap_image(
        toolset,
        blueprint,
        "DesignerWeaponInteractionArt",
        "DesignerWeaponInteractionScale",
    )
    wrapped.append("DesignerWeaponInteractionArt")

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Shop widget failed to compile after aspect migration")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Shop widget failed to save after aspect migration")
    unreal.log(
        "[Plan149ShopIconAspect] ScaleToFit+Center " + ",".join(wrapped)
    )


if __name__ == "__main__":
    main()
