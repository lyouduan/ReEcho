"""Bind the delivered scalable frame to the Plan132 tooltip only."""

import unreal


TOOLTIP_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip"
FRAME_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/LoadoutSelection/T_UI_Loadout_DescriptionPanel"
)


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(TOOLTIP_PATH)
    frame_texture = unreal.load_asset(FRAME_TEXTURE_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Plan132 Tooltip WBP is missing: {TOOLTIP_PATH}")
    if not isinstance(frame_texture, unreal.Texture2D):
        raise RuntimeError(f"Plan132 Tooltip frame texture is missing: {FRAME_TEXTURE_PATH}")

    frame = widget_map(toolset, blueprint).get("TooltipFrame")
    if not isinstance(frame, unreal.Border):
        raise RuntimeError("Plan132 TooltipFrame Border is missing")
    background = frame.get_editor_property("background")
    background.set_editor_property("resource_object", frame_texture)
    background.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    background.set_editor_property(
        "margin", unreal.Margin(2.0 / 230.0, 2.0 / 134.0, 2.0 / 230.0, 2.0 / 134.0)
    )
    background.set_editor_property(
        "tint_color", unreal.SlateColor(unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    )
    frame.set_editor_property("background", background)
    frame.set_editor_property("brush_color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    frame.set_editor_property("padding", unreal.Margin(3.0, 3.0, 3.0, 3.0))

    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Tooltip WBP failed to compile after frame migration")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError("Plan132 Tooltip WBP failed to save after frame migration")
    unreal.log("[Plan132TooltipFrameMigration] frame=delivery-texture draw=Box")


if __name__ == "__main__":
    main()
