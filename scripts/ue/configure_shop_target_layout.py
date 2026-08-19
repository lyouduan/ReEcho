import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
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
# These legacy Overlay children are intentionally retired. Their replacements
# live under DesignerLoadoutCanvas and must not be repositioned by this script.
for widget_name in (
    "ArtLoadoutWeapon",
    "ArtShopClock",
    "ArtAttachmentSlot0",
    "ArtAttachmentSlot1",
    "ArtAttachmentSlot2",
    "ArtAttachmentSlot3",
):
    widgets[widget_name].set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan45Shop] aligned authored shop shell to the 1920x1080 target")
