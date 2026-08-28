"""Audit the formal VictoryCanvas and its independently editable UMG elements."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
REQUIRED_TYPES = {
    "VictoryCanvas": unreal.CanvasPanel,
    "ArtVictoryDimmerFormal": unreal.Image,
    "ArtVictorySummaryPanelFormal": unreal.Image,
    "ArtVictoryCharacterFormal": unreal.Image,
    "ArtVictoryRoseRightFormal": unreal.Image,
    "ArtVictoryTimeShardFormal": unreal.Image,
    "ArtVictoryContinueButtonFormal": unreal.Image,
    "VictoryTitleText": unreal.TextBlock,
    "VictoryEncounterValue": unreal.TextBlock,
    "VictoryTimeShardsValue": unreal.TextBlock,
    "VictoryTraitCountValue": unreal.TextBlock,
    "VictorySelectedCardsTitle": unreal.TextBlock,
    "VictoryContinueButton": unreal.Button,
    "VictoryContinueLabel": unreal.TextBlock,
}
EXPECTED_RECTS = {
    "VictoryCanvas": (0.0, 0.0, 1920.0, 1080.0),
    "ArtVictorySummaryPanelFormal": (199.0, 341.0, 1405.0, 500.0),
    "ArtVictoryCharacterFormal": (1195.0, 192.0, 552.0, 695.0),
    "ArtVictoryRoseRightFormal": (1134.0, 276.0, 216.0, 160.0),
    "VictoryTitleText": (735.0, 206.0, 613.0, 139.0),
    "VictoryContinueButton": (784.0, 816.0, 405.0, 136.0),
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
        raise RuntimeError(f"Plan129 binding {name} is missing or has the wrong type")
    if name != "VictoryCanvas":
        if widget.get_parent() != widgets["VictoryCanvas"]:
            raise RuntimeError(f"Plan129 element {name} is not a direct VictoryCanvas child")
        if not isinstance(widget.get_editor_property("slot"), unreal.CanvasPanelSlot):
            raise RuntimeError(f"Plan129 element {name} is not freely editable through a CanvasPanelSlot")

for name, expected_rect in EXPECTED_RECTS.items():
    slot = widgets[name].get_editor_property("slot")
    if not isinstance(slot, unreal.CanvasPanelSlot):
        raise RuntimeError(f"Plan129 layout slot is missing: {name}")
    offsets = slot.get_editor_property("layout_data").offsets
    actual_rect = (offsets.left, offsets.top, offsets.right, offsets.bottom)
    if any(abs(actual - expected) > 0.1 for actual, expected in zip(actual_rect, expected_rect)):
        raise RuntimeError(f"Plan129 layout drift for {name}: {actual_rect} != {expected_rect}")

for index in range(5):
    name = f"DesignerVictoryCardSlot{index}"
    widget = widgets.get(name)
    if not isinstance(widget, unreal.Image) or widget.get_parent() != widgets["VictoryCanvas"]:
        raise RuntimeError(f"Plan129 editable card sample is missing: {name}")

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("Plan129 WBP compile audit failed")
unreal.log("[Plan129VictoryAudit] formal round-victory UI bindings and Canvas slots are valid")
