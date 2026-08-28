"""Remove the retired three-slot prototype from the Plan147 authored popup."""

import unreal


POPUP_PATH = "/Game/ReEcho/UI/WBP_ReEchoStoragePopup"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def set_canvas_rect(widget, x, y, width, height):
    slot = widget.get_editor_property("slot")
    require(isinstance(slot, unreal.CanvasPanelSlot), f"{widget.get_name()} needs a Canvas slot")
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


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(POPUP_PATH)
require(isinstance(blueprint, unreal.WidgetBlueprint), f"Missing popup: {POPUP_PATH}")


def widget_map():
    infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    return {info.widget.get_name(): info.widget for info in infos if info.widget}


widgets = widget_map()
retired_roots = [
    "EchoCancelReplaceButton",
    "EchoReplaceInstructionText",
    "EchoSelectionText",
]
for index in range(3):
    retired_roots.extend((f"EchoSlotText{index}", f"EchoReplace{index}", f"EchoSelect{index}"))

for name in retired_roots:
    widget = widgets.get(name)
    if widget is not None:
        require(
            toolset.call_method("RemoveWidget", args=(blueprint, widget)),
            f"Unable to remove retired widget: {name}",
        )

widgets = widget_map()
designer_text = {
    "EchoReplayModeText": "你的回响只会重复时间锚点所在关卡内的行为。",
    "EchoCapacityText": "当前时间锚点：遭遇 #1 | 角色 J_HEART | 武器 LongSword",
    "EchoPendingInfoText": "本场回响：遭遇 #3 | 角色 J_HEART | 武器 LongSword",
    "EchoStoreLabel": "替换为本场时间锚点",
    "EchoSkipLabel": "跳过本场回响",
}
for name, value in designer_text.items():
    widget = widgets.get(name)
    require(isinstance(widget, unreal.TextBlock), f"Missing authored text: {name}")
    widget.set_editor_property("text", value)

layout = {
    "EchoReplayModeText": (570.0, 365.0, 780.0, 34.0),
    "EchoCapacityText": (530.0, 420.0, 860.0, 38.0),
    "EchoPendingInfoText": (530.0, 480.0, 860.0, 38.0),
    "EchoStoreButton": (670.0, 555.0, 270.0, 75.0),
    "EchoSkipButton": (980.0, 555.0, 270.0, 75.0),
}
for name, rect in layout.items():
    widget = widgets.get(name)
    require(widget is not None, f"Missing authored widget: {name}")
    set_canvas_rect(widget, *rect)

require(
    toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)),
    "Popup failed to compile after single-time-anchor migration",
)
require(
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False),
    "Popup failed to save after single-time-anchor migration",
)
unreal.log("[Plan147SingleTimeAnchorPopup] PASS retired three-slot UI removed")
