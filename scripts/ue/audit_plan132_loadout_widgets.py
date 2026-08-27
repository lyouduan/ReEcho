"""Audit the authored Plan132 Loadout WBP hierarchy and binding contract."""

import unreal


ASSET_PATHS = (
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry",
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


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
