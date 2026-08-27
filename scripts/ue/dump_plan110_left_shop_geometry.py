"""Dump the current authored geometry for the Plan110 time-shop only."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing widget blueprint: {ASSET_PATH}")

toolset = unreal.UMGToolSet.get_default_object()
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {str(info.widget_name): info.widget for info in infos if info.widget}

names = [
    "DesignerShopPresentationCanvas",
    "ArtFormalShopFrame",
    "ArtFormalShopSurface",
    "ArtFormalPartPaper",
    "ArtFormalPackPaper",
    "DesignerPartSectionTitle",
    "DesignerPackSectionTitle",
    "ArtFormalCurrencyFrame",
    "DesignerCurrencyText",
    "ShopRefreshButton",
    "DesignerRefreshLimitText",
]
for index in range(3):
    for prefix in ("DesignerPartOffer", "DesignerPackOffer"):
        names.extend(
            (
                f"{prefix}Card{index}",
                f"{prefix}Base{index}",
                f"{prefix}Icon{index}",
                f"{prefix}Description{index}",
                f"{prefix}Cost{index}",
                f"{prefix}Buy{index}",
            )
        )

for name in names:
    widget = widgets.get(name)
    if widget is None:
        unreal.log_warning(f"[Plan110LeftGeometry] MISSING {name}")
        continue
    slot = widget.get_editor_property("slot")
    if isinstance(slot, unreal.CanvasPanelSlot):
        data = slot.get_editor_property("layout_data")
        offsets = data.get_editor_property("offsets")
        unreal.log(
            "[Plan110LeftGeometry] "
            f"{name} parent={widget.get_parent().get_name() if widget.get_parent() else '<root>'} "
            f"rect=({offsets.left:.1f},{offsets.top:.1f},{offsets.right:.1f},{offsets.bottom:.1f}) "
            f"z={slot.get_editor_property('z_order')} "
            f"visibility={widget.get_editor_property('visibility')}"
        )
    else:
        unreal.log(
            "[Plan110LeftGeometry] "
            f"{name} parent={widget.get_parent().get_name() if widget.get_parent() else '<root>'} "
            f"slot={slot.get_class().get_name() if slot else '<none>'} "
            f"visibility={widget.get_editor_property('visibility')}"
        )

unreal.log("[Plan110LeftGeometry] PASS")
