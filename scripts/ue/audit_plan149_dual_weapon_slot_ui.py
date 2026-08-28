"""Audit the authored five-slot dual-weapon layout."""

import unreal


WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"
CORE_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan149/"
    "T_UI_Shop149_CoreWeaponLoadoutSlot"
)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(WIDGET_PATH)
core_texture = unreal.load_asset(CORE_TEXTURE_PATH)
require(blueprint is not None, f"Missing shop widget: {WIDGET_PATH}")
require(isinstance(core_texture, unreal.Texture2D), "Missing core slot texture")
infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widgets = {
    str(info.widget_name): info.widget
    for info in infos
    if info.widget is not None
}

layout = widgets.get("DesignerDualAttachmentLayout")
loadout = widgets.get("DesignerLoadoutCanvas")
require(isinstance(layout, unreal.CanvasPanel), "Missing dual attachment layout")
require(layout.get_parent() == loadout, "Dual layout is not owned by DesignerLoadoutCanvas")
require(
    isinstance(layout.get_editor_property("slot"), unreal.CanvasPanelSlot),
    "Dual layout cannot be freely transformed in Designer",
)

for index in range(5):
    button = widgets.get(f"DesignerDualAttachmentSlot{index}")
    scale = widgets.get(f"DesignerDualAttachmentSlotScale{index}")
    art = widgets.get(f"DesignerDualAttachmentSlotArt{index}")
    require(isinstance(button, unreal.Button), f"Missing dual slot {index}")
    require(button.get_parent() == layout, f"Dual slot {index} is not independently movable")
    require(
        isinstance(button.get_editor_property("slot"), unreal.CanvasPanelSlot),
        f"Dual slot {index} is not a Canvas child",
    )
    require(isinstance(scale, unreal.ScaleBox), f"Missing dual slot scale {index}")
    require(scale.get_parent() == button, f"Dual slot scale {index} is not inside its frame")
    require(
        scale.get_editor_property("stretch") == unreal.Stretch.SCALE_TO_FIT,
        f"Dual slot scale {index} does not preserve texture aspect ratio",
    )
    require(isinstance(art, unreal.Image), f"Missing dual slot art {index}")
    require(art.get_parent() == scale, f"Dual slot art {index} is not aspect-fitted in its frame")
    require(
        art.get_editor_property("visibility") == unreal.SlateVisibility.HIDDEN,
        f"Empty dual slot art {index} must not render as a white placeholder",
    )

core_button = widgets["DesignerDualAttachmentSlot0"]
require(
    core_button.get_editor_property("widget_style")
    .get_editor_property("normal")
    .get_editor_property("resource_object")
    == core_texture,
    "The core slot does not use 核心武器装配.png",
)
core_offsets = (
    core_button.get_editor_property("slot")
    .get_editor_property("layout_data")
    .get_editor_property("offsets")
)
require(
    abs(core_offsets.right - 89.0) < 0.1 and abs(core_offsets.bottom - 175.0) < 0.1,
    "The core slot must preserve its reviewed 89x175 aspect",
)

unreal.log("[Plan149DualSlotAudit] PASS one core + four non-core authored slots")
