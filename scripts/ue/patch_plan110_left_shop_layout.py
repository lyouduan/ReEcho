"""Align the existing Plan110 time-shop with the approved 1920x1080 reference.

This is intentionally incremental: it edits only named left-shop widgets and
does not rebuild the presentation canvas or touch the assembly/loadout areas.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


def set_canvas_layout(widget, rect, z_order=None):
    slot = widget.get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"{widget.get_name()} is not a CanvasPanel child")
    x, y, width, height = rect
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
    if z_order is not None:
        slot.set_editor_property("z_order", z_order)


def set_font_size(text, size):
    font = text.get_editor_property("font")
    font.size = size
    text.set_editor_property("font", font)


blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing widget blueprint: {ASSET_PATH}")

toolset = unreal.UMGToolSet.get_default_object()
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {str(info.widget_name): info.widget for info in infos if info.widget}


def required(name):
    widget = widgets.get(name)
    if widget is None:
        raise RuntimeError(f"Missing authored left-shop widget: {name}")
    return widget


outer_layout = {
    "ArtFormalShopFrame": (151.0, 2.0, 701.0, 1039.0),
    "ArtFormalShopSurface": (195.0, 129.0, 610.0, 854.0),
    "ArtFormalPartPaper": (240.0, 262.0, 530.0, 320.0),
    "ArtFormalPackPaper": (240.0, 636.0, 530.0, 320.0),
    "DesignerPartSectionTitle": (241.0, 225.0, 130.0, 38.0),
    "DesignerPackSectionTitle": (241.0, 599.0, 130.0, 38.0),
    "ArtFormalCurrencyFrame": (336.0, 181.0, 260.0, 38.0),
    "DesignerCurrencyText": (366.0, 184.0, 210.0, 30.0),
    "ShopRefreshButton": (611.0, 178.0, 162.0, 41.0),
    "DesignerRefreshLimitText": (611.0, 220.0, 162.0, 24.0),
}
for name, rect in outer_layout.items():
    set_canvas_layout(required(name), rect)

# The reference has no standalone refresh-status line; button state still
# communicates availability and all refresh logic remains bound in C++.
required("DesignerRefreshLimitText").set_editor_property(
    "visibility", unreal.SlateVisibility.COLLAPSED
)

card_x = (255.0, 425.0, 594.0)
for index, x in enumerate(card_x):
    for prefix, y, font_size in (
        ("DesignerPartOffer", 276.0, 10),
        ("DesignerPackOffer", 650.0, 12),
    ):
        set_canvas_layout(required(f"{prefix}Card{index}"), (x, y, 161.0, 296.0))
        set_canvas_layout(required(f"{prefix}Base{index}"), (0.0, 0.0, 161.0, 296.0))
        icon = required(f"{prefix}Icon{index}")
        set_canvas_layout(icon, (27.0, 13.0, 108.0, 108.0))
        icon.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        description = required(f"{prefix}Description{index}")
        set_canvas_layout(description, (8.0, 184.0, 145.0, 52.0))
        description.set_editor_property("auto_wrap_text", True)
        description.set_editor_property("justification", unreal.TextJustify.LEFT)
        set_font_size(description, font_size)
        set_canvas_layout(required(f"{prefix}Cost{index}"), (104.0, 147.0, 48.0, 25.0))
        set_canvas_layout(required(f"{prefix}Buy{index}"), (12.0, 240.0, 134.0, 46.0))

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save")

unreal.log("[Plan110LeftShopPatch] aligned the time-shop only")
