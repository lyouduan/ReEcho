"""Read-only diagnostic for Plan110 loadout card-slot geometry."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


def prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as error:
        return f"<{type(error).__name__}: {error}>"


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
widgets = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widget_map = {info.widget.get_name(): info.widget for info in widgets if info.widget}

for index in range(12):
    button = widget_map.get(f"DesignerCardSlot{index}")
    art = widget_map.get(f"DesignerCardSlotArt{index}")
    if button is None:
        unreal.log_warning(f"[Plan110CardSlotInspect] missing button {index}")
        continue

    button_slot = prop(button, "slot")
    layout = prop(button_slot, "layout_data")
    offsets = prop(layout, "offsets")
    style = prop(button, "style")
    unreal.log(
        f"[Plan110CardSlotInspect] button={index} "
        f"size=({offsets.right},{offsets.bottom}) "
        f"render_transform={prop(button, 'render_transform')} "
        f"normal_padding={prop(style, 'normal_padding')} "
        f"pressed_padding={prop(style, 'pressed_padding')}"
    )

    if art is None:
        unreal.log_warning(f"[Plan110CardSlotInspect] missing art {index}")
        continue

    art_slot = prop(art, "slot")
    brush = prop(art, "brush")
    unreal.log(
        f"[Plan110CardSlotInspect] art={index} "
        f"parent={art.get_parent().get_name() if art.get_parent() else '<none>'} "
        f"slot={art_slot.get_class().get_name()} "
        f"h_align={prop(art_slot, 'horizontal_alignment')} "
        f"v_align={prop(art_slot, 'vertical_alignment')} "
        f"padding={prop(art_slot, 'padding')} "
        f"render_transform={prop(art, 'render_transform')} "
        f"brush_size={prop(brush, 'image_size')} "
        f"draw_as={prop(brush, 'draw_as')} "
        f"resource={prop(brush, 'resource_object')}"
    )

