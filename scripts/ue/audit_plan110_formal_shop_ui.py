"""Audit the authored hierarchy of the Plan110 inventory-shop widget."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing shop widget: {ASSET_PATH}")

widgets = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
widget_map = {info.widget.get_name(): info.widget for info in widgets if info.widget is not None}
for index, info in enumerate(widgets):
    widget = info.widget
    if widget is None:
        continue
    parent = widget.get_parent()
    slot = widget.get_editor_property("slot")
    geometry = ""
    if isinstance(slot, unreal.CanvasPanelSlot):
        layout = slot.get_editor_property("layout_data")
        offsets = layout.get_editor_property("offsets")
        anchors = layout.get_editor_property("anchors")
        geometry = (
            f" offsets=({offsets.left:.1f},{offsets.top:.1f},"
            f"{offsets.right:.1f},{offsets.bottom:.1f})"
            f" anchors=({anchors.minimum.x:.2f},{anchors.minimum.y:.2f})"
            f"-({anchors.maximum.x:.2f},{anchors.maximum.y:.2f})"
            f" z={slot.get_editor_property('z_order')}"
        )
    unreal.log(
        "[Plan110ShopTree] "
        f"{index:03d} name={widget.get_name()} "
        f"type={widget.get_class().get_name()} "
        f"parent={parent.get_name() if parent else '<root>'} "
        f"variable={info.get_editor_property('is_variable')} "
        f"visibility={widget.get_editor_property('visibility')} "
        f"slot={slot.get_class().get_name() if slot else '<none>'}"
        f"{geometry}"
    )

unreal.log(f"[Plan110ShopTree] TOTAL={len(widgets)}")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


required_canvas_children = (
    "DesignerShopPresentationCanvas",
    "DesignerLoadoutCanvas",
)
for name in required_canvas_children:
    widget = widget_map.get(name)
    require(isinstance(widget, unreal.CanvasPanel), f"Missing authored canvas: {name}")
    require(
        isinstance(widget.get_editor_property("slot"), unreal.CanvasPanelSlot),
        f"{name} must be directly positionable on the root canvas",
    )

presentation = widget_map["DesignerShopPresentationCanvas"]
loadout = widget_map["DesignerLoadoutCanvas"]
for name in (
    "DesignerCurrencyText",
    "ShopRefreshButton",
    "ArtFormalShopFrame",
    "ArtFormalAssemblyPanel",
    "ArtFormalLoadoutTree",
    "ArtFormalSaveAndLeave",
):
    widget = widget_map.get(name)
    require(widget is not None, f"Missing formal shop widget: {name}")
    require(widget.get_parent() == presentation, f"{name} must remain on the formal presentation canvas")
    require(isinstance(widget.get_editor_property("slot"), unreal.CanvasPanelSlot), f"{name} geometry is not authored")

for index in range(3):
    for prefix in ("DesignerPartOfferCard", "DesignerPackOfferCard"):
        card = widget_map.get(f"{prefix}{index}")
        require(isinstance(card, unreal.CanvasPanel), f"Missing authored offer card: {prefix}{index}")
        require(card.get_parent() == presentation, f"{prefix}{index} is not directly movable")
    for prefix in ("DesignerPartOfferBuy", "DesignerPackOfferBuy"):
        button = widget_map.get(f"{prefix}{index}")
        require(
            button is not None and button.get_class().get_name() == "ReEchoIndexedButton",
            f"{prefix}{index} must preserve indexed purchase dispatch",
        )
    for prefix in ("DesignerPartOffer", "DesignerPackOffer"):
        card = widget_map[f"{prefix}Card{index}"]
        for suffix in ("Base", "Icon", "Description", "Cost", "Buy"):
            child = widget_map.get(f"{prefix}{suffix}{index}")
            require(child is not None, f"Missing editable card element: {prefix}{suffix}{index}")
            if suffix == "Description":
                label_overlay = child.get_parent()
                require(
                    isinstance(label_overlay, unreal.Overlay) and label_overlay.get_parent() == card,
                    f"{prefix}{suffix}{index} must remain in its card-owned label Overlay",
                )
                require(
                    isinstance(label_overlay.get_editor_property("slot"), unreal.CanvasPanelSlot),
                    f"{prefix}{suffix}{index} Overlay is not freely positionable in Designer",
                )
                require(
                    isinstance(child.get_editor_property("slot"), unreal.OverlaySlot),
                    f"{prefix}{suffix}{index} must keep overlay-relative alignment",
                )
            else:
                if suffix == "Icon" and prefix == "DesignerPartOffer":
                    scale = widget_map.get(f"DesignerPartOfferIconScale{index}")
                    require(isinstance(scale, unreal.ScaleBox), f"Missing aspect-fit offer icon host {index}")
                    require(scale.get_parent() == card, f"Offer icon host {index} is not freely positionable")
                    require(
                        isinstance(scale.get_editor_property("slot"), unreal.CanvasPanelSlot),
                        f"Offer icon host {index} is not a Canvas child",
                    )
                    require(child.get_parent() == scale, f"Offer icon {index} bypasses aspect fitting")
                    require(
                        scale.get_editor_property("stretch") == unreal.Stretch.SCALE_TO_FIT,
                        f"Offer icon {index} does not preserve texture aspect ratio",
                    )
                else:
                    require(child.get_parent() == card, f"{prefix}{suffix}{index} must remain inside its card Canvas")
                    require(
                        isinstance(child.get_editor_property("slot"), unreal.CanvasPanelSlot),
                        f"{prefix}{suffix}{index} is not freely positionable in Designer",
                    )
    attachment = widget_map.get(f"DesignerAttachmentSlot{index}")
    attachment_art = widget_map.get(f"DesignerAttachmentSlotArt{index}")
    require(isinstance(attachment, unreal.Button), f"Missing attachment slot {index}")
    require(attachment.get_parent() == loadout, f"Attachment slot {index} is not designer-positionable")
    require(isinstance(attachment_art, unreal.Image), f"Missing attachment slot art {index}")
    require(
        attachment_art.get_editor_property("visibility") == unreal.SlateVisibility.HIDDEN,
        f"Empty attachment slot art {index} must not render as a white placeholder",
    )

for index in range(12):
    card_slot = widget_map.get(f"DesignerCardSlot{index}")
    card_art = widget_map.get(f"DesignerCardSlotArt{index}")
    require(isinstance(card_slot, unreal.Button), f"Missing card slot {index}")
    require(card_slot.get_parent() == loadout, f"Card slot {index} is not designer-positionable")
    require(isinstance(card_art, unreal.Image), f"Missing card slot art {index}")
    require(card_art.get_parent() == card_slot, f"Card slot art {index} is not layered inside its frame")

weapon_button = widget_map.get("DesignerWeaponInteractionButton")
require(isinstance(weapon_button, unreal.Button), "Missing authored weapon interaction button")
require(weapon_button.get_parent() == loadout, "Weapon interaction geometry must remain designer-authored")
weapon_scale = widget_map.get("DesignerWeaponInteractionScale")
weapon_art = widget_map.get("DesignerWeaponInteractionArt")
require(isinstance(weapon_scale, unreal.ScaleBox), "Missing equipped-weapon aspect-fit host")
require(weapon_scale.get_parent() == weapon_button, "Equipped weapon scale must stay inside its interaction button")
require(isinstance(weapon_art, unreal.Image), "Missing equipped weapon art")
require(weapon_art.get_parent() == weapon_scale, "Equipped weapon art bypasses aspect fitting")
require(
    weapon_scale.get_editor_property("stretch") == unreal.Stretch.SCALE_TO_FIT,
    "Equipped weapon art does not preserve texture aspect ratio",
)
clock = widget_map.get("DesignerShopClock")
require(isinstance(clock, unreal.Image), "Missing authored shop-clock hover target")
require(clock.get_parent() == loadout, "Shop-clock geometry must remain designer-authored")

for logic_root_name in ("Overlay_0", "Overlay_1"):
    logic_root = widget_map.get(logic_root_name)
    require(logic_root is not None, f"Missing compatibility logic root: {logic_root_name}")
    require(
        logic_root.get_editor_property("visibility") == unreal.SlateVisibility.COLLAPSED,
        f"Compatibility logic root is still visible: {logic_root_name}",
    )

for retired_visual_name in (
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
):
    require(
        retired_visual_name not in widget_map,
        f"Retired visual widget was not cleaned up: {retired_visual_name}",
    )

runtime_texture_root = "/Game/ReEcho/Textures/UI/InventoryShop/Plan110"
required_textures = (
    "OfferCard",
    "BuyButton",
    "RefreshButton",
    "CurrencyFrame",
    "WeaponLoadoutSlot",
    "LoadoutCardSlot",
    "EmptyCardSlotIcon",
    "AssemblyPanel",
    "LoadoutTreePanel",
    "SaveAndLeave",
)
for texture_name in required_textures:
    asset_path = f"{runtime_texture_root}/T_UI_Shop110_{texture_name}"
    texture = unreal.load_asset(asset_path)
    require(isinstance(texture, unreal.Texture2D), f"Missing runtime shop texture: {asset_path}")
    require(
        texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_UI,
        f"Shop texture is not in the UI texture group: {asset_path}",
    )

empty_icon = unreal.load_asset(
    f"{runtime_texture_root}/T_UI_Shop110_EmptyCardSlotIcon"
)
slot_frame = unreal.load_asset(
    f"{runtime_texture_root}/T_UI_Shop110_LoadoutCardSlot"
)
for index in range(12):
    card_slot = widget_map[f"DesignerCardSlot{index}"]
    card_art = widget_map[f"DesignerCardSlotArt{index}"]
    require(
        card_slot.get_editor_property("widget_style")
        .get_editor_property("normal")
        .get_editor_property("resource_object")
        == slot_frame,
        f"Card slot {index} does not preserve the authored frame",
    )
    require(
        card_art.get_editor_property("visibility") == unreal.SlateVisibility.HIDDEN,
        f"Card slot {index} still previews product placeholder art",
    )

for index in range(3):
    pack_art = widget_map[f"DesignerPackOfferIcon{index}"]
    require(
        pack_art.get_editor_property("brush").get_editor_property("resource_object")
        == empty_icon,
        f"Card-pack offer {index} does not preview the reviewed lock icon",
    )

for forbidden_reference in (
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan110/ShopTarget",
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan110/ShopAttachmentTooltipTarget",
    "/Game/ReEcho/Textures/UI/InventoryShop/Plan110/ShopStatsTarget",
):
    require(
        not unreal.EditorAssetLibrary.does_asset_exist(forbidden_reference),
        f"Reference composite must not be a runtime page texture: {forbidden_reference}",
    )

unreal.log("[Plan110ShopAudit] PASS authored controls, stable actions, and reviewed runtime slices")
