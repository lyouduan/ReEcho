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
            title = widget_by_name.get("TitleText")
            description = widget_by_name.get("DescriptionText")
            if not isinstance(root, unreal.SizeBox):
                raise RuntimeError("Plan132 tooltip root SizeBox is missing")
            if not isinstance(title, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip TitleText is missing")
            if not isinstance(description, unreal.TextBlock):
                raise RuntimeError("Plan132 tooltip DescriptionText is missing")
            if root.get_editor_property("width_override") != 380.0:
                raise RuntimeError("Plan132 tooltip authored width is not 380")
            if title.get_editor_property("font").size != 22:
                raise RuntimeError("Plan132 tooltip authored title font is not 22")
            if description.get_editor_property("font").size != 20:
                raise RuntimeError("Plan132 tooltip authored body font is not 20")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
