"""Audit the authored Plan132 Loadout WBP hierarchy and binding contract."""

import unreal


ASSET_PATHS = (
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip",
)

FONT_PATHS = (
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W_Font",
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W",
)


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    for font_path in FONT_PATHS:
        font_asset = unreal.load_asset(font_path)
        unreal.log(
            f"[Plan132LoadoutAudit] font={font_path} "
            f"type={font_asset.get_class().get_name() if font_asset else '<missing>'}"
        )
    for asset_path in ASSET_PATHS:
        blueprint = unreal.load_asset(asset_path)
        if not isinstance(blueprint, unreal.WidgetBlueprint):
            raise RuntimeError(f"Missing Plan132 WidgetBlueprint: {asset_path}")
        infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        widgets = [info.widget for info in infos if info.widget]
        roots = [widget for widget in widgets if widget.get_parent() is None]
        unreal.log(
            f"[Plan132LoadoutAudit] asset={asset_path} widgets={len(widgets)} "
            f"roots={[root.get_name() for root in roots]}"
        )
        if asset_path.endswith("WBP_ReEchoLoadoutEntry"):
            generated_class = blueprint.generated_class()
            default_object = unreal.get_default_object(generated_class)
            tooltip_class = default_object.get_editor_property("tooltip_widget_class")
            expected_tooltip_path = (
                "/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip."
                "WBP_ReEchoLoadoutTooltip_C"
            )
            actual_tooltip_path = (
                tooltip_class.get_path_name() if tooltip_class else "<missing>"
            )
            unreal.log(
                f"[Plan132LoadoutAudit] entry_tooltip_class={actual_tooltip_path}"
            )
            if actual_tooltip_path != expected_tooltip_path:
                raise RuntimeError(
                    "Plan132 entry does not reference the authored tooltip class"
                )
        if asset_path.endswith("WBP_ReEchoLoadoutSelection"):
            widget_by_name = {widget.get_name(): widget for widget in widgets}
            character_row = widget_by_name.get("CharacterRow")
            weapon_row = widget_by_name.get("WeaponRow")
            if not isinstance(character_row, unreal.HorizontalBox) or not isinstance(
                weapon_row, unreal.HorizontalBox
            ):
                raise RuntimeError("Plan132 selection preview rows are missing")
            for domain, row in (
                ("Character", character_row),
                ("Weapon", weapon_row),
            ):
                children = [
                    widget
                    for widget in widgets
                    if widget.get_parent() is row
                ]
                unreal.log(
                    f"[Plan132LoadoutAudit] {domain.lower()}_preview_entries="
                    f"{[child.get_name() for child in children]}"
                )
                expected_names = [f"{domain}Entry{index}" for index in range(4)]
                if [child.get_name() for child in children] != expected_names:
                    raise RuntimeError(
                        f"Plan132 {domain} Designer preview is not four authored entries"
                    )
                if any(
                    child.get_class().get_name() != "WBP_ReEchoLoadoutEntry_C"
                    for child in children
                ):
                    raise RuntimeError(
                        f"Plan132 {domain} preview does not use the runtime Entry WBP"
                    )
            design_canvas = widget_by_name.get("LoadoutDesignCanvas")
            arrow_names = (
                "CharacterSelectionArrow0",
                "CharacterSelectionArrow1",
                "CharacterSelectionArrow2",
                "SelectionArrow",
                "WeaponSelectionArrow0",
                "WeaponSelectionArrow1",
                "WeaponSelectionArrow2",
                "WeaponSelectionArrow3",
            )
            for arrow_name in arrow_names:
                arrow = widget_by_name.get(arrow_name)
                if not isinstance(arrow, unreal.Image) or arrow.get_parent() is not design_canvas:
                    raise RuntimeError(
                        f"Plan132 Designer-owned selection arrow is missing: {arrow_name}"
                    )
            unreal.log(
                f"[Plan132LoadoutAudit] designer_selection_arrows={list(arrow_names)}"
            )
        for info in infos:
            widget = info.widget
            if widget is None:
                continue
            parent = widget.get_parent()
            slot = widget.get_editor_property("slot")
            unreal.log(
                f"[Plan132LoadoutAudit] widget={widget.get_name()} "
                f"type={widget.get_class().get_name()} "
                f"parent={parent.get_name() if parent else '<none>'} "
                f"slot={slot.get_class().get_name() if slot else '<none>'} "
                f"variable={bool(info.get_editor_property('is_variable'))}"
            )
            if isinstance(widget, unreal.TextBlock):
                font = widget.get_editor_property("font")
                font_object = font.get_editor_property("font_object")
                outline = font.get_editor_property("outline_settings")
                unreal.log(
                    f"[Plan132LoadoutAudit] text={widget.get_name()} "
                    f"font={font_object.get_path_name() if font_object else '<default>'} "
                    f"size={font.get_editor_property('size')} "
                    f"outline={outline.get_editor_property('outline_size')}"
                )
        if asset_path.endswith("WBP_ReEchoLoadoutTooltip"):
            widget_by_name = {widget.get_name(): widget for widget in widgets}
            root = widget_by_name.get("TooltipRootSizeBox")
            frame = widget_by_name.get("TooltipFrame")
            title = widget_by_name.get("TitleText")
            description = widget_by_name.get("DescriptionText")
            if not isinstance(root, unreal.SizeBox):
                raise RuntimeError("Plan132 tooltip root SizeBox is missing")
            if not isinstance(title, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip TitleText is missing")
            if not isinstance(description, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip DescriptionText is missing")
            if not isinstance(frame, unreal.Border):
                raise RuntimeError("Plan132 tooltip frame Border is missing")
            frame_brush = frame.get_editor_property("background")
            frame_resource = frame_brush.get_editor_property("resource_object")
            expected_frame_path = (
                "/Game/ReEcho/Textures/UI/LoadoutSelection/"
                "T_UI_Loadout_DescriptionPanel.T_UI_Loadout_DescriptionPanel"
            )
            if (
                not isinstance(frame_resource, unreal.Texture2D)
                or frame_resource.get_path_name() != expected_frame_path
                or frame_brush.get_editor_property("draw_as")
                != unreal.SlateBrushDrawType.BOX
            ):
                raise RuntimeError(
                    "Plan132 tooltip frame does not use the scalable delivery texture"
                )
            if root.get_editor_property("width_override") != 380.0:
                raise RuntimeError("Plan132 tooltip authored width is not 380")
            if title.get_editor_property("font").size != 22:
                raise RuntimeError("Plan132 tooltip authored title font is not 22")
            if description.get_editor_property("font").size != 20:
                raise RuntimeError("Plan132 tooltip authored body font is not 20")
            default_object = unreal.get_default_object(blueprint.generated_class())
            has_desired_preview = default_object.call_method(
                "HasDesiredDesignerPreview"
            )
            unreal.log(
                "[Plan132LoadoutAudit] "
                f"tooltip_desired_preview={bool(has_desired_preview)} "
                f"tooltip_frame={frame_resource.get_path_name()} draw=Box"
            )
            if not has_desired_preview:
                raise RuntimeError(
                    "Plan132 tooltip Designer preview is not desired-size WYSIWYG"
                )
            preview_description = str(description.get_editor_property("text"))
            if len(preview_description) < 30:
                raise RuntimeError(
                    "Plan132 tooltip Designer preview description is not representative"
                )


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
