import unreal


ASSET_PATH = "/Game/ReEcho/UI/WBP_ReEchoTraitCardEntry"


blueprint = unreal.load_asset(ASSET_PATH)
if blueprint is None:
    raise RuntimeError(f"Required asset is missing: {ASSET_PATH}")

toolset = unreal.UMGToolSet.get_default_object()
result = toolset.call_method("GetWidgets", args=(blueprint,))
widgets = {str(info.widget_name): info.widget for info in result.widgets if info.widget}

for name, left, right in (
    ("PrimaryTagText", 26.0, 175.0),
    ("SecondaryTagText", 175.0, 26.0),
):
    text = widgets[name]
    text.set_editor_property("justification", unreal.TextJustify.CENTER)
    text.set_editor_property("auto_wrap_text", False)
    slot = text.get_editor_property("slot")
    slot.set_editor_property("padding", unreal.Margin(left, 478.0, right, 22.0))
    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    # CENTER with asymmetric top/bottom margins offsets the desired-size text
    # outside the card. FILL keeps the text inside the same bounds as the art.
    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)

if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
    raise RuntimeError("WBP_ReEchoTraitCardEntry failed to compile")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("WBP_ReEchoTraitCardEntry failed to save")

unreal.log("[Plan45TraitTags] aligned tag labels inside their card artwork")
