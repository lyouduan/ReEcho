"""Remove retired visual-only widgets from the authored Plan110 shop WBP.

The legacy logic contract (InventoryPanel, ShopPanel, CurrencyText,
InventoryText and OfferContainer) is intentionally preserved.  This script
only deletes presentation nodes that are collapsed on the formal path and are
not read by UReEchoInventoryShopWidget.
"""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"

RETIRED_VISUAL_WIDGETS = (
    "ArtShopKeeper",
    "ArtLoadoutPanel",
    "ArtLoadoutTitle",
    "ArtLoadoutWeapon",
    "ArtLoadoutStats",
    "ArtAttachmentSlot0",
    "ArtAttachmentSlot1",
    "ArtAttachmentSlot2",
    "ArtAttachmentSlot3",
    "ArtShopClock",
    "ArtSkillTooltip",
    "ArtShopOfferPanel",
    "ArtShopTitle",
    "CloseButtonLabel",
    "DesignerRefreshLimitText",
)


blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing shop widget: {ASSET_PATH}")

toolset = unreal.UMGToolSet.get_default_object()
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {str(info.widget_name): info.widget for info in infos if info.widget}

removed = []
for name in RETIRED_VISUAL_WIDGETS:
    widget = widgets.get(name)
    if widget is None:
        continue
    if not toolset.call_method("RemoveWidget", args=(blueprint, widget)):
        raise RuntimeError(f"Failed to remove retired shop widget: {name}")
    removed.append(name)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to compile after cleanup")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoInventoryShopScreen failed to save after cleanup")

remaining_infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
remaining_names = {str(info.widget_name) for info in remaining_infos if info.widget}
still_present = sorted(set(RETIRED_VISUAL_WIDGETS).intersection(remaining_names))
if still_present:
    raise RuntimeError(f"Retired widgets still present: {still_present}")

unreal.log(
    f"[Plan110ShopCleanup] removed {len(removed)} retired visual widgets: "
    + ", ".join(removed)
)
