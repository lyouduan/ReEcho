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

REQUIRED_VARIABLES = {
    "/Game/ReEcho/UI/WBP_ReEchoSettings": {
        "GraphicsSettingsButton": unreal.ReEchoIndexedButton,
        "AudioSettingsButton": unreal.ReEchoIndexedButton,
        "ControlsSettingsButton": unreal.ReEchoIndexedButton,
        "RestoreDefaultsButton": unreal.Button,
        "ApplyAndReturnButton": unreal.Button,
        "SettingsCloseButton": unreal.Button,
        "GraphicsPanel": unreal.Widget,
        "ControlsPanel": unreal.Widget,
        "MasterVolumeSlider": unreal.Slider,
        "MusicVolumeSlider": unreal.Slider,
        "CombatSfxVolumeSlider": unreal.Slider,
    },
    "/Game/ReEcho/UI/WBP_ReEchoRestart": {
        "RootPanel": unreal.VerticalBox,
        "TitleText": unreal.TextBlock,
        "MessageText": unreal.TextBlock,
        "ResumeButton": unreal.Button,
        "RestartButton": unreal.Button,
        "QuitButton": unreal.Button,
        "PauseSettingsButton": unreal.Button,
        "ArtPauseDimmer": unreal.Image,
        "ArtPauseResume": unreal.Image,
        "ArtPauseExitToMenu": unreal.Image,
        "ArtPauseExitGame": unreal.Image,
        "ArtPauseSaveAndExit": unreal.Image,
        "ArtPauseExitWithoutSave": unreal.Image,
        "ArtPauseBack": unreal.Image,
    },
}


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def get_widget_infos(toolset, blueprint):
    return [info for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets if info.widget]


def widget_name(widget):
    return widget.get_name() if widget else "<none>"


def direct_children(widget):
    if not isinstance(widget, unreal.PanelWidget):
        return []
    return [widget.get_child_at(index) for index in range(widget.get_children_count())]


def resolve_visual_root(button):
    parent = button.get_parent()
    if isinstance(parent, unreal.Overlay):
        parent_children = direct_children(parent)
        button_count = sum(isinstance(child, unreal.Button) for child in parent_children)
        if button_count == 1 and len(parent_children) > 1:
            return parent
    return button


toolset = unreal.UMGToolSet.get_default_object()
for asset_path in ASSET_PATHS:
    blueprint = load_required(asset_path)
    widget_infos = get_widget_infos(toolset, blueprint)
    widgets = [info.widget for info in widget_infos]
    info_by_name = {str(info.widget_name): info for info in widget_infos}
    unreal.log(f"[Plan80Audit] ASSET {asset_path} widgets={len(widgets)}")
    authored_roots = [widget_name(widget) for widget in widgets if widget.get_parent() is None]
    unreal.log(f"[Plan80Audit] ROOTS {asset_path} roots={authored_roots}")
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
    for required_name, required_type in REQUIRED_VARIABLES.get(asset_path, {}).items():
        info = info_by_name.get(required_name)
        if info is None or not isinstance(info.widget, required_type):
            actual_type = info.widget.get_class().get_name() if info and info.widget else "<missing>"
            raise RuntimeError(
                f"{asset_path} binding {required_name} expected {required_type.__name__}, got {actual_type}"
            )
        is_variable = info.get_editor_property("is_variable")
        if not is_variable:
            toolset.call_method("ToggleWidgetAsVariable", args=(blueprint, info.widget, True))
            refreshed_info = {
                str(candidate.widget_name): candidate for candidate in get_widget_infos(toolset, blueprint)
            }.get(required_name)
            is_variable = refreshed_info and refreshed_info.get_editor_property("is_variable")
        if not is_variable:
            raise RuntimeError(f"{asset_path} binding {required_name} could not be marked as variable")
        unreal.log(
            f"[Plan80Binding] {asset_path} {required_name} "
            f"type={info.widget.get_class().get_name()} is_variable={bool(is_variable)}"
        )
    if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
        raise RuntimeError(f"Widget failed to compile: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Widget failed to save: {asset_path}")
    unreal.log(f"[Plan80Audit] VERIFIED {asset_path}")
