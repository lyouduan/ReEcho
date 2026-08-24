"""Print the authored Settings widget tree for one-time legacy cleanup review."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")

infos = toolset.call_method("GetWidgets", args=(blueprint,)).widgets
for index, info in enumerate(infos):
    widget = info.widget
    if widget is None:
        continue
    parent = widget.get_parent()
    visibility = widget.get_editor_property("visibility")
    unreal.log(
        "[Plan86Tree] "
        f"{index:03d} name={widget.get_name()} "
        f"type={widget.get_class().get_name()} "
        f"parent={parent.get_name() if parent else '<root>'} "
        f"visibility={visibility}"
    )
