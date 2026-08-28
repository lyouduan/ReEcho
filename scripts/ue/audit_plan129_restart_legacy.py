"""Audit WBP_ReEchoRestart for superseded pre-formal result widgets."""

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


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing restart widget: {WIDGET_PATH}")

widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

present = [name for name in LEGACY_WIDGETS if name in widgets]
if present:
    raise RuntimeError(f"Legacy result widgets remain in WBP_ReEchoRestart: {present}")

present_assets = [
    asset_path
    for asset_path in LEGACY_ASSETS
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
]
if present_assets:
    raise RuntimeError(f"Legacy result textures remain: {present_assets}")

for required in ("VictoryCanvas", "DefeatCanvas", "ArtRestartDialogPanel"):
    if required not in widgets:
        raise RuntimeError(f"Required current restart UI widget was removed: {required}")

unreal.log("[Plan129RestartLegacyAudit] legacy result widgets and textures are absent")
