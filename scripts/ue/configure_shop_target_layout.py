import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
EMPTY_ATTACHMENT_SLOT_PATH = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/InventoryShop/"
    "T_UI_Shop_AttachmentSlot"
)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def widget_map(toolset, blueprint):
    result = toolset.call_method("GetWidgets", args=(blueprint,))
    return {str(info.widget_name): info.widget for info in result.widgets if info.widget}


def set_canvas_layout(widget, x, y, width, height, z):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not in a CanvasPanel")
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
    slot.set_editor_property("z_order", z)


blueprint = load_required(ASSET_PATH)
toolset = unreal.UMGToolSet.get_default_object()
widgets = widget_map(toolset, blueprint)

set_canvas_layout(widgets["ShopPanel"], 53.0, 164.0, 783.0, 797.0, 5)
set_canvas_layout(widgets["InventoryPanel"], 862.0, 164.0, 966.0, 797.0, 5)
set_canvas_layout(widgets["Overlay_0"], 862.0, 164.0, 966.0, 797.0, 5)
set_canvas_layout(widgets["ArtShopKeeper"], 1216.0, 342.0, 306.0, 679.0, 9)
set_canvas_layout(widgets["CloseButton"], 1478.0, 854.0, 350.0, 81.0, 20)
widgets["ArtShopTitle"].set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
# Slot descriptions now use the cursor-following custom tooltip. The large
# authored stats board behind the card slots is therefore intentionally hidden.
widgets["ArtLoadoutStats"].set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

# The target places exactly three attachment icons in one row directly below
# the weapon art. These are Overlay children, so the margins define the exact
# 93x93 boxes within the 966x797 loadout overlay.
for widget_name, left in (("ArtAttachmentSlot0", 28.0), ("ArtAttachmentSlot1", 140.0), ("ArtAttachmentSlot2", 252.0)):
    slot = widgets[widget_name].get_editor_property("slot")
    slot.set_editor_property("padding", unreal.Margin(left, 587.0, 966.0 - left - 93.0, 117.0))
    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)
    widgets[widget_name].set_brush_from_texture(load_required(EMPTY_ATTACHMENT_SLOT_PATH), False)
    widgets[widget_name].set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
widgets["ArtAttachmentSlot3"].set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan45Shop] aligned authored shop shell to the 1920x1080 target")
