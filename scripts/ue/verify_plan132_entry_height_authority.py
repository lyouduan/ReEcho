"""Compile/save the Plan132 Entry WBP and prove Blueprint-owned heights survive."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry"


def get_size_boxes(toolset, blueprint):
    infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    widgets = {info.widget.get_name(): info.widget for info in infos if info.widget}
    root = widgets.get("EntryRootSizeBox")
    portrait = widgets.get("PortraitSize")
    if not isinstance(root, unreal.SizeBox) or not isinstance(portrait, unreal.SizeBox):
        raise RuntimeError("Plan132 Entry authored SizeBox hierarchy is missing")
    return root, portrait


def heights(toolset, blueprint):
    root, portrait = get_size_boxes(toolset, blueprint)
    return (
        root.get_editor_property("height_override"),
        portrait.get_editor_property("height_override"),
    )


def main():
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(ASSET_PATH)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing Plan132 WidgetBlueprint: {ASSET_PATH}")

    before = heights(toolset, blueprint)
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError("Plan132 Entry WBP failed to compile")
    after_compile = heights(toolset, blueprint)
    if after_compile != before:
        raise RuntimeError(
            f"Entry heights changed during compile: before={before} after={after_compile}"
        )
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Plan132 Entry WBP failed to save")
    after_save = heights(toolset, blueprint)
    if after_save != before:
        raise RuntimeError(
            f"Entry heights changed during save: before={before} after={after_save}"
        )
    unreal.log(
        "[Plan132EntryHeightAuthority] PASS "
        f"entry={before[0]} portrait={before[1]} compile/save preserved"
    )


main()
