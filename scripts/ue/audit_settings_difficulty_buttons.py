"""Audit the authored fourth Settings tab and difficulty page."""

import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoSettings"
BUTTON_LIGHT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/T_UI_Pause_ButtonLight"
BUTTON_DARK = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/PauseAndCombat/T_UI_Pause_ButtonDark"
TAB_LIGHT = "/Game/ReEcho/Textures/UI/InteractionPlaceholder/Settings/T_UI_Settings_TabLight"
ROOTS = (
    ("DifficultyPartyRoot", "DifficultyPartyButton", "DifficultyPartyArt", "DifficultyPartyText", "派对", BUTTON_DARK),
    (
        "DifficultyStandardRoot",
        "DifficultyStandardButton",
        "DifficultyStandardArt",
        "DifficultyStandardText",
        "常规",
        BUTTON_LIGHT,
    ),
    (
        "DifficultyNightmareRoot",
        "DifficultyNightmareButton",
        "DifficultyNightmareArt",
        "DifficultyNightmareText",
        "噩梦",
        BUTTON_DARK,
    ),
)


toolset = unreal.UMGToolSet.get_default_object()
blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Missing Settings widget: {ASSET_PATH}")
widgets = {
    info.widget.get_name(): info.widget
    for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
    if info.widget
}
panel = widgets.get("Panel")
settings_layout = widgets.get("SettingsLayoutCanvas")
difficulty_tab_root = widgets.get("Overlay_Difficulty")
difficulty_tab_button = widgets.get("DifficultySettingsButton")
difficulty_tab_text = widgets.get("DifficultyTabText")
difficulty_panel = widgets.get("DifficultyPanel")
if not isinstance(panel, unreal.CanvasPanel) or not isinstance(settings_layout, unreal.CanvasPanel):
    raise RuntimeError("Missing authored Settings host Canvases")
if not isinstance(difficulty_tab_root, unreal.Overlay) or difficulty_tab_root.get_parent() is not panel:
    raise RuntimeError("Difficulty top tab is not authored directly in Panel")
if not isinstance(difficulty_tab_root.get_editor_property("slot"), unreal.CanvasPanelSlot):
    raise RuntimeError("Difficulty top tab is not freely adjustable in a Canvas slot")
if not isinstance(difficulty_tab_button, unreal.ReEchoIndexedButton):
    raise RuntimeError("DifficultySettingsButton is missing")
if not isinstance(difficulty_tab_text, unreal.TextBlock) or difficulty_tab_text.get_editor_property("text") != "难度":
    raise RuntimeError("The fourth top tab does not preserve the authored 难度 label")
tab_resource = (
    difficulty_tab_button.get_editor_property("widget_style")
    .get_editor_property("normal")
    .get_editor_property("resource_object")
)
if tab_resource is None or tab_resource.get_path_name().split(".")[0] != TAB_LIGHT:
    raise RuntimeError("Difficulty tab is not the selected Designer preview")
if not isinstance(difficulty_panel, unreal.CanvasPanel) or difficulty_panel.get_parent() is not settings_layout:
    raise RuntimeError("DifficultyPanel is not an independent authored category page")
if not isinstance(difficulty_panel.get_editor_property("slot"), unreal.CanvasPanelSlot):
    raise RuntimeError("DifficultyPanel is not freely adjustable in a Canvas slot")
if "DifficultyLabel" in widgets:
    raise RuntimeError("The obsolete inline DifficultyLabel still exists in ControlsPanel")

for root_name, button_name, art_name, text_name, expected_text, expected_texture_path in ROOTS:
    root = widgets.get(root_name)
    button = widgets.get(button_name)
    art = widgets.get(art_name)
    text = widgets.get(text_name)
    if not isinstance(root, unreal.Overlay) or root.get_parent() is not difficulty_panel:
        raise RuntimeError(f"{root_name} is not authored directly in DifficultyPanel")
    if not isinstance(root.get_editor_property("slot"), unreal.CanvasPanelSlot):
        raise RuntimeError(f"{root_name} is not freely adjustable in a Canvas slot")
    if not isinstance(button, unreal.ReEchoIndexedButton) or button.get_parent() is not root:
        raise RuntimeError(f"{button_name} is not the authored input layer")
    if not isinstance(art, unreal.Image) or art.get_parent() is not root:
        raise RuntimeError(f"{art_name} is not the authored visual layer")
    if not isinstance(text, unreal.TextBlock) or text.get_parent() is not root:
        raise RuntimeError(f"{text_name} is not the authored label layer")
    if text.get_editor_property("text") != expected_text:
        raise RuntimeError(f"{text_name} has unexpected default text")
    resource = art.get_editor_property("brush").get_editor_property("resource_object")
    if resource is None or resource.get_path_name().split(".")[0] != expected_texture_path:
        raise RuntimeError(f"{art_name} has unexpected default pause-button art")
    if art.get_editor_property("visibility") != unreal.SlateVisibility.HIT_TEST_INVISIBLE:
        raise RuntimeError(f"{art_name} can intercept button input")

if "DifficultyComboBox" in widgets or "DifficultySettingsPanel" in widgets:
    raise RuntimeError("Legacy runtime-style difficulty widgets were saved into the Blueprint")
if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoSettings failed to compile during difficulty audit")
unreal.log("[SettingsDifficultyAudit] fourth tab and WYSIWYG difficulty page passed")
