import unreal


ASSET_PATHS = (
    "/Game/ReEcho/UI/WBP_ReEchoStartMenu",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection",
    "/Game/ReEcho/UI/WBP_ReEchoLoadoutEntry",
    "/Game/ReEcho/UI/WBP_ReEchoSettings",
    "/Game/ReEcho/UI/WBP_ReEchoRestart",
    "/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice",
    "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry",
    "/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen",
    "/Game/ReEcho/UI/WBP_ReEchoStatsScreen",
    "/Game/ReEcho/UI/WBP_ReEchoAbout",
)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def get_widgets(toolset, blueprint):
    return [info.widget for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets if info.widget]


def widget_name(widget):
    return widget.get_name() if widget else "<none>"


def direct_children(widget):
    if not isinstance(widget, unreal.PanelWidget):
        return []
    return [widget.get_child_at(index) for index in range(widget.get_children_count())]


def resolve_visual_root(button):
    if button.get_children_count() > 0:
        return button
    parent = button.get_parent()
    if not isinstance(parent, unreal.Overlay):
        return button
    button_count = sum(isinstance(child, unreal.Button) for child in direct_children(parent))
    return parent if button_count == 1 else button


toolset = unreal.UMGToolSet.get_default_object()
for asset_path in ASSET_PATHS:
    blueprint = load_required(asset_path)
    widgets = get_widgets(toolset, blueprint)
    unreal.log(f"[Plan80Audit] ASSET {asset_path} widgets={len(widgets)}")
    for widget in widgets:
        if isinstance(widget, unreal.Button):
            parent = widget.get_parent()
            child = widget.get_child_at(0) if widget.get_children_count() else None
            visual_root = resolve_visual_root(widget)
            visual_members = ",".join(widget_name(member) for member in direct_children(visual_root))
            unreal.log(
                f"[Plan80Audit] BUTTON {widget_name(widget)} "
                f"parent={widget_name(parent)}:{parent.get_class().get_name() if parent else '<none>'} "
                f"child={widget_name(child)}:{child.get_class().get_name() if child else '<none>'} "
                f"visual_root={widget_name(visual_root)}:{visual_root.get_class().get_name()} "
                f"visual_members=[{visual_members}] "
                f"visibility={widget.get_editor_property('visibility')}"
            )
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError(f"Widget failed to compile: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Widget failed to save: {asset_path}")
    unreal.log(f"[Plan80Audit] VERIFIED {asset_path}")
