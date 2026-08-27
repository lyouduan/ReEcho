"""Audit the formal DefeatCanvas and its independently editable UMG elements."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
REQUIRED_TYPES = {
    "DefeatCanvas": unreal.CanvasPanel,
    "ArtDefeatDimmerFormal": unreal.Image,
    "ArtDefeatSummaryPanelFormal": unreal.Image,
    "ArtDefeatCharacterFormal": unreal.Image,
    "ArtDefeatWitheredFlowerLeftFormal": unreal.Image,
    "ArtDefeatWitheredFlowerRightFormal": unreal.Image,
    "ArtDefeatTimeShardFormal": unreal.Image,
    "ArtDefeatRestartButtonFormal": unreal.Image,
    "ArtDefeatMainMenuButtonFormal": unreal.Image,
    "DefeatTitleText": unreal.TextBlock,
    "DefeatEncounterValue": unreal.TextBlock,
    "DefeatTimeShardsValue": unreal.TextBlock,
    "DefeatTraitCountValue": unreal.TextBlock,
    "DefeatSelectedCardsTitle": unreal.TextBlock,
    "DefeatRestartButton": unreal.Button,
    "DefeatRestartLabel": unreal.TextBlock,
    "DefeatMainMenuButton": unreal.Button,
    "DefeatMainMenuLabel": unreal.TextBlock,
}
EXPECTED_RECTS = {
    "DefeatCanvas": (0.0, 0.0, 1920.0, 1080.0),
    "ArtDefeatSummaryPanelFormal": (199.0, 341.0, 1405.0, 500.0),
    "ArtDefeatCharacterFormal": (1195.0, 192.0, 552.0, 695.0),
    "DefeatTitleText": (735.0, 206.0, 613.0, 139.0),
    "DefeatRestartButton": (502.0, 808.0, 405.0, 136.0),
    "DefeatMainMenuButton": (943.0, 808.0, 405.0, 136.0),
}


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Plan129 widget: {WIDGET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}

for name, expected_type in REQUIRED_TYPES.items():
    widget = widgets.get(name)
    if not isinstance(widget, expected_type):
        raise RuntimeError(f"Plan129 defeat binding {name} is missing or has the wrong type")
    if name != "DefeatCanvas":
        if widget.get_parent() != widgets["DefeatCanvas"]:
            raise RuntimeError(f"Plan129 defeat element {name} is not a direct DefeatCanvas child")
        if not isinstance(widget.get_editor_property("slot"), unreal.CanvasPanelSlot):
            raise RuntimeError(f"Plan129 defeat element {name} is not independently editable")

for name, expected_rect in EXPECTED_RECTS.items():
    slot = widgets[name].get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Plan129 defeat layout slot is missing: {name}")
    offsets = slot.get_editor_property("layout_data").offsets
    actual_rect = (offsets.left, offsets.top, offsets.right, offsets.bottom)
    if any(abs(actual - expected) > 0.1 for actual, expected in zip(actual_rect, expected_rect)):
        raise RuntimeError(f"Plan129 defeat layout drift for {name}: {actual_rect} != {expected_rect}")

for index in range(5):
    name = f"DesignerDefeatCardSlot{index}"
    widget = widgets.get(name)
    if not isinstance(widget, unreal.Image) or widget.get_parent() != widgets["DefeatCanvas"]:
        raise RuntimeError(f"Plan129 defeat editable card sample is missing: {name}")

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("Plan129 defeat WBP compile audit failed")
unreal.log("[Plan129DefeatAudit] formal round-defeat UI bindings and Canvas slots are valid")
