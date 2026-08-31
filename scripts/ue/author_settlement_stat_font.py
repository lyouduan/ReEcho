"""Author the settlement stat text onto the shared title font (汉仪瑞意宋).

The restart widget is designer-authored, so ``BuildWidgetTree`` returns early and
never recreates these TextBlocks. The font therefore lives in the WBP asset and
must be authored there; runtime code only overrides the size.

Idempotent: re-running re-asserts the same font and skips widgets already set.
"""

import unreal

WIDGET_PATH = "/Game/ReEcho/UI/WBP_ReEchoRestart"
TITLE_FONT_PATH = (
    "/Game/SourceArt/UI/InteractionPlaceholder/Fonts/Titles/汉仪瑞意宋_80W_Font"
)
TITLE_TYPEFACE = "Font"

LABELS = (
    "EncounterLabel",
    "TraitCountLabel",
    "TimeShardsLabel",
    "EchoDamageValueLabel",
    "PlayerDamageValueLabel",
    "ReactionCountValueLabel",
    "MaxHitValueLabel",
    "KillCountValueLabel",
)
VALUES = (
    "EncounterValue",
    "TraitCountValue",
    "TimeShardsValue",
    "EchoDamageValue",
    "PlayerDamageValue",
    "ReactionCountValue",
    "MaxHitValue",
    "KillCountValue",
)
PREFIXES = ("Victory", "Defeat")


def font_label(widget):
    font = widget.get_editor_property("font")
    obj = font.get_editor_property("font_object")
    return (
        obj.get_path_name().rsplit("/", 1)[-1] if obj else "None",
        str(font.get_editor_property("typeface_font_name")),
        font.get_editor_property("size"),
    )


def main():
    title_font = unreal.load_asset(TITLE_FONT_PATH)
    if title_font is None:
        raise RuntimeError(f"Missing title font asset: {TITLE_FONT_PATH}")

    blueprint = unreal.load_asset(WIDGET_PATH)
    if blueprint is None:
        raise RuntimeError(f"Missing restart widget: {WIDGET_PATH}")

    toolset = unreal.UMGToolSet.get_default_object()
    widgets = {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }

    changed = []
    missing = []
    for prefix in PREFIXES:
        for base in LABELS + VALUES:
            name = f"{prefix}{base}"
            widget = widgets.get(name)
            if widget is None:
                missing.append(name)
                continue

            font = widget.get_editor_property("font")
            font.set_editor_property("font_object", title_font)
            font.set_editor_property("typeface_font_name", TITLE_TYPEFACE)
            widget.set_editor_property("font", font)

            current = font_label(widget)
            if current[0] != "汉仪瑞意宋_80W_Font.汉仪瑞意宋_80W_Font":
                raise RuntimeError(f"Font authoring failed for {name}: {current}")
            changed.append(f"{name} -> {current[0]} typeface={current[1]} size={current[2]}")

    if missing:
        raise RuntimeError(f"Missing settlement stat widgets: {missing}")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

    unreal.log(f"[SettlementStatFont] authored {len(changed)} text blocks")
    for entry in changed:
        unreal.log(f"    {entry}")


main()
