"""Audit the authored Plan147 echo-storage popup and its formal UI slices."""

import unreal


SHOP_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
POPUP_PATH = "/Game/ReEcho/UI/WBP_ReEchoStoragePopup"
TEXTURE_ROOT = "/Game/ReEcho/Textures/UI/InventoryShop/EchoStorage"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def require_canvas_geometry(widget, parent, expected_z=None):
    require(widget is not None, "Missing expected widget")
    require(widget.get_parent() == parent, f"{widget.get_name()} has the wrong parent")
    slot = widget.get_editor_property("slot")
    require(
        isinstance(slot, unreal.CanvasPanelSlot),
        f"{widget.get_name()} must stay directly editable in a CanvasPanel",
    )
    if expected_z is not None:
        require(
            slot.get_editor_property("z_order") == expected_z,
            f"{widget.get_name()} has the wrong Z order",
        )
    return slot


toolset = unreal.UMGToolSet.get_default_object()
shop_blueprint = unreal.load_asset(SHOP_PATH)
popup_blueprint = unreal.load_asset(POPUP_PATH)
require(shop_blueprint is not None, f"Missing shop widget: {SHOP_PATH}")
require(popup_blueprint is not None, f"Missing popup widget: {POPUP_PATH}")

shop_infos = toolset.call_method("GetWidgets", args=(shop_blueprint,)).widgets
shop_widgets = {
    info.widget.get_name(): info.widget for info in shop_infos if info.widget
}
host = shop_widgets.get("EchoStoragePopupWidget")
require(isinstance(host, unreal.UserWidget), "Shop must contain one popup child WBP")
require(
    host.get_class() == popup_blueprint.generated_class(),
    "Shop popup host uses the wrong standalone WBP",
)
host_slot = host.get_editor_property("slot")
require(isinstance(host_slot, unreal.CanvasPanelSlot), "Popup child needs Canvas slot")
require(host_slot.get_editor_property("z_order") == 40, "Popup host has wrong Z order")
require(
    "EchoPanelScale" not in shop_widgets and "CloseConfirmWidget" not in shop_widgets,
    "Shop still embeds popup implementation details",
)

infos = toolset.call_method("GetWidgets", args=(popup_blueprint,)).widgets
widgets = {info.widget.get_name(): info.widget for info in infos if info.widget}

root = widgets.get("EchoStoragePopupRoot")
require(isinstance(root, unreal.CanvasPanel), "Standalone popup root must be a CanvasPanel")

light = unreal.load_asset(f"{TEXTURE_ROOT}/T_UI_EchoStorage_ButtonLight")
dark = unreal.load_asset(f"{TEXTURE_ROOT}/T_UI_EchoStorage_ButtonDark")
frame = unreal.load_asset(f"{TEXTURE_ROOT}/T_UI_EchoStorage_PopupFrame")
for texture in (light, dark, frame):
    require(isinstance(texture, unreal.Texture2D), "Missing formal Plan147 texture")
    require(
        texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_UI,
        f"{texture.get_name()} is not configured for UI",
    )

scale = widgets.get("EchoPanelScale")
require(isinstance(scale, unreal.ScaleBox), "Missing authored EchoPanelScale")
scale_slot = require_canvas_geometry(scale, root, 40)
scale_offsets = scale_slot.get_editor_property("layout_data").get_editor_property("offsets")
require(
    abs(scale_offsets.left) < 0.01
    and abs(scale_offsets.top) < 0.01
    and abs(scale_offsets.right - 1920.0) < 0.01
    and abs(scale_offsets.bottom - 1080.0) < 0.01,
    "EchoPanelScale no longer owns the 1920x1080 Designer surface",
)
panel = widgets.get("EchoPanel")
require(isinstance(panel, unreal.VerticalBox), "EchoPanel must preserve the C++ contract")
canvas = widgets.get("EchoPopupCanvas")
require(isinstance(canvas, unreal.CanvasPanel), "Missing editable echo popup Canvas")

frame_art = widgets.get("EchoPopupFrameArt")
require_canvas_geometry(frame_art, canvas)
require(
    frame_art.get_editor_property("brush").get_editor_property("resource_object") == frame,
    "Echo popup is not using the formal frame slice",
)

required_texts = (
    "EchoPopupTitle",
    "EchoCapacityText",
    "EchoReplayModeText",
    "EchoPendingInfoText",
)
for name in required_texts:
    widget = widgets.get(name)
    require(isinstance(widget, unreal.TextBlock), f"Missing authored text: {name}")
    require_canvas_geometry(widget, canvas)

expected_designer_text = {
    "EchoReplayModeText": "你的回响只会重复时间锚点所在关卡内的行为。",
    "EchoCapacityText": "当前时间锚点：遭遇 #1 | 角色 J_HEART | 武器 LongSword",
    "EchoPendingInfoText": "本场回响：遭遇 #3 | 角色 J_HEART | 武器 LongSword",
    "EchoStoreLabel": "替换为本场时间锚点",
    "EchoSkipLabel": "跳过本场回响",
}
for name, expected in expected_designer_text.items():
    widget = widgets.get(name)
    require(isinstance(widget, unreal.TextBlock), f"Missing authored anchor text: {name}")
    require(str(widget.get_editor_property("text")) == expected, f"{name} has stale multi-replay copy")

buttons = [
    "EchoPopupCloseButton",
    "EchoStoreButton",
    "EchoSkipButton",
]
retired_widgets = [
    "EchoCancelReplaceButton",
    "EchoReplaceInstructionText",
    "EchoSelectionText",
]
for index in range(3):
    retired_widgets.extend((f"EchoSlotText{index}", f"EchoReplace{index}", f"EchoSelect{index}"))
for name in retired_widgets:
    require(name not in widgets, f"Retired three-slot widget still exists: {name}")

close_root = widgets.get("CloseConfirmWidget")
require(isinstance(close_root, unreal.VerticalBox), "Missing authored close confirmation root")
require_canvas_geometry(close_root, root, 50)
require(
    close_root.get_editor_property("visibility") == unreal.SlateVisibility.COLLAPSED,
    "Close confirmation should not cover the primary Designer preview",
)
close_canvas = widgets.get("EchoCloseConfirmCanvas")
require(isinstance(close_canvas, unreal.CanvasPanel), "Missing close confirmation Canvas")
close_frame = widgets.get("EchoCloseConfirmFrameArt")
require_canvas_geometry(close_frame, close_canvas)
require(
    close_frame.get_editor_property("brush").get_editor_property("resource_object") == frame,
    "Close confirmation is not using the formal frame slice",
)
buttons.extend(("EchoConfirmSkipContinue", "EchoConfirmReturn"))

for name in buttons:
    button = widgets.get(name)
    require(isinstance(button, unreal.Button), f"Missing authored button: {name}")
    style = button.get_editor_property("widget_style")
    require(
        style.get_editor_property("normal").get_editor_property("resource_object") == light,
        f"{name} normal state is not using the light slice",
    )
    for state in ("hovered", "pressed", "disabled"):
        require(
            style.get_editor_property(state).get_editor_property("resource_object") == dark,
            f"{name} {state} state is not using the dark slice",
        )
    normal_padding = style.get_editor_property("normal_padding")
    pressed_padding = style.get_editor_property("pressed_padding")
    require(
        all(abs(value) < 0.01 for value in (normal_padding.left, normal_padding.top, normal_padding.right, normal_padding.bottom)),
        f"{name} has non-zero normal padding that can distort its art",
    )
    require(
        all(abs(value) < 0.01 for value in (pressed_padding.left, pressed_padding.top, pressed_padding.right, pressed_padding.bottom)),
        f"{name} has non-zero pressed padding that can distort its art",
    )

require(
    not unreal.EditorAssetLibrary.does_asset_exist(
        f"{TEXTURE_ROOT}/T_UI_EchoStorage_Reference_ReopenPopup"
    ),
    "The full SVG reference must not be imported as a runtime page texture",
)
unreal.log(
    "[Plan147EchoAudit] PASS standalone single-time-anchor modal, one-child shop host, "
    "formal slices, and no retired three-slot controls"
)
