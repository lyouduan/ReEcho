"""Print WBP_ReEchoRestart's authored hierarchy and Canvas geometry."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing pause widget: {ASSET_PATH}")

for index, info in enumerate(toolset.call_method("GetWidgets", args=(blueprint,)).widgets):
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
            f" offsets=({offsets.left:.1f},{offsets.top:.1f},{offsets.right:.1f},{offsets.bottom:.1f})"
            f" anchors=({anchors.minimum.x:.2f},{anchors.minimum.y:.2f})"
            f"-({anchors.maximum.x:.2f},{anchors.maximum.y:.2f})"
        )
    unreal.log(
        "[Plan86PauseTree] "
        f"{index:03d} name={widget.get_name()} type={widget.get_class().get_name()} "
        f"parent={parent.get_name() if parent else '<root>'} "
        f"visibility={widget.get_editor_property('visibility')} slot={slot.get_class().get_name() if slot else '<none>'}"
        f"{geometry}"
    )
