"""Remove result widgets and textures superseded by the formal Victory/Defeat canvases."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
LEGACY_WIDGETS = (
    "ArtRestartCharacter",
    "ArtResultSummaryPanel",
    "ArtSelectedCardsPanel",
    "ArtVictoryTitle",
    "ArtDefeatTitle",
)
LEGACY_ASSETS = (
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Restart_Character",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_Back",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_Continue",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_DefeatTitle",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_Restart",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_SelectedCardsPanel",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_SummaryPanel",
    "/Game/ReEcho/Textures/UI/InteractionPlaceholder/ResultsAndRestart/T_UI_Result_VictoryTitle",
)


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing restart widget: {WIDGET_PATH}")

widgets = widget_map(toolset, blueprint)
removed_widgets = []
for name in LEGACY_WIDGETS:
    widget = widgets.get(name)
    if widget is None:
        unreal.log(f"[Plan129LegacyCleanup] widget already absent: {name}")
        continue
    if not toolset.call_method("RemoveWidget", args=(blueprint, widget)):
        raise RuntimeError(f"Could not remove legacy result widget: {name}")
    removed_widgets.append(name)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("Failed to compile WBP_ReEchoRestart after legacy cleanup")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("Failed to save WBP_ReEchoRestart after legacy cleanup")

widgets = widget_map(toolset, blueprint)
survivors = [name for name in LEGACY_WIDGETS if name in widgets]
if survivors:
    raise RuntimeError(f"Legacy result widgets survived cleanup: {survivors}")

removed_assets = []
for asset_path in LEGACY_ASSETS:
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"[Plan129LegacyCleanup] asset already absent: {asset_path}")
        continue
    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
        asset_path, load_assets_to_confirm=True
    )
    if referencers:
        raise RuntimeError(f"Refusing to delete referenced legacy asset {asset_path}: {referencers}")
    if not unreal.EditorAssetLibrary.delete_asset(asset_path):
        raise RuntimeError(f"Could not delete unreferenced legacy result asset: {asset_path}")
    removed_assets.append(asset_path)

unreal.log(
    f"[Plan129LegacyCleanup] removed widgets={removed_widgets}; removed assets={removed_assets}"
)
