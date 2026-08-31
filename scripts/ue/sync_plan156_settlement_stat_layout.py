"""Explicit one-shot copy of authored Victory stat presentation to Defeat.

Run with Run-EditorPythonLocked.ps1 after saving/closing the editor. This is
not a runtime binding: both pages remain independently editable afterwards.
main(verify_only=True) checks the saved pairs without compiling or saving.
"""

import unreal


ASSET = "/Game/ReEcho/UI/WBP_ReEchoRestart"
SUFFIXES = (
    "EncounterLabel", "EncounterValue",
    "TraitCountLabel", "TraitCountValue",
    "TimeShardsLabel", "TimeShardsValue",
    "EchoDamageValueLabel", "EchoDamageValue",
    "PlayerDamageValueLabel", "PlayerDamageValue",
    "ReactionCountValueLabel", "ReactionCountValue",
    "MaxHitValueLabel", "MaxHitValue",
    "KillCountValueLabel", "KillCountValue",
)
TEXT_PROPERTIES = (
    "font", "color_and_opacity", "shadow_offset", "shadow_color_and_opacity",
    "justification", "margin", "auto_wrap_text", "wrap_text_at",
    "wrapping_policy", "line_height_percentage", "apply_line_height_to_bottom_line",
    "min_desired_width", "text_transform_policy", "text_overflow_policy",
    "clipping", "render_transform", "render_transform_pivot", "render_opacity",
)
SLOT_PROPERTIES = ("layout_data", "auto_size", "z_order")


def encoded(value):
    if isinstance(value, unreal.StructBase):
        return value.export_text()
    return str(value)


def presentation(widget):
    slot = widget.get_editor_property("slot")
    return tuple(
        encoded(owner.get_editor_property(prop))
        for owner, properties in ((widget, TEXT_PROPERTIES), (slot, SLOT_PROPERTIES))
        for prop in properties
    )


def widget_map(toolset, blueprint):
    return {
        info.widget.get_name(): info.widget
        for info in toolset.call_method("GetWidgets", args=(blueprint,)).widgets
        if info.widget
    }


def pairs_from(widgets):
    pairs = []
    for suffix in SUFFIXES:
        source, target = (widgets[prefix + suffix] for prefix in ("Victory", "Defeat"))
        for prefix, widget in (("Victory", source), ("Defeat", target)):
            if not isinstance(widget, unreal.TextBlock):
                raise RuntimeError(f"Expected TextBlock: {prefix}{suffix}")
            if widget.get_parent() != widgets[prefix + "Canvas"]:
                raise RuntimeError(f"Unexpected parent: {widget.get_name()}")
            if not isinstance(widget.get_editor_property("slot"), unreal.CanvasPanelSlot):
                raise RuntimeError(f"Expected Canvas slot: {widget.get_name()}")
            presentation(widget)  # Preflight every reflected property before any writes.
        pairs.append((source, target))
    return pairs


def main(verify_only=False):
    toolset = unreal.UMGToolSet.get_default_object()
    blueprint = unreal.load_asset(ASSET)
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(f"Missing WidgetBlueprint: {ASSET}")
    widgets = widget_map(toolset, blueprint)
    pairs = pairs_from(widgets)
    source_before = {source.get_name(): presentation(source) for source, _ in pairs}
    # Protect names, hierarchy and all text content; only the 16 targets are written.
    names_before = set(widgets)
    parents_before = {
        name: widget.get_parent().get_name() if widget.get_parent() else None
        for name, widget in widgets.items()
    }
    text_before = {
        name: str(widget.get_editor_property("text"))
        for name, widget in widgets.items() if isinstance(widget, unreal.TextBlock)
    }
    if not verify_only:
        for source, target in pairs:
            for prop in TEXT_PROPERTIES:
                target.set_editor_property(prop, source.get_editor_property(prop))
            for prop in SLOT_PROPERTIES:
                target.get_editor_property("slot").set_editor_property(
                    prop, source.get_editor_property("slot").get_editor_property(prop)
                )
        if not toolset.call_method("CompileWidgetBlueprint", args=(blueprint,)):
            raise RuntimeError("Settlement blueprint compile failed")

    widgets = widget_map(toolset, blueprint)
    if set(widgets) != names_before:
        raise RuntimeError("Widget names changed")
    for name, widget in widgets.items():
        parent = widget.get_parent().get_name() if widget.get_parent() else None
        if parent != parents_before[name]:
            raise RuntimeError(f"Widget hierarchy changed: {name}")
        if name in text_before and str(widget.get_editor_property("text")) != text_before[name]:
            raise RuntimeError(f"Authored text changed: {name}")
    for source, target in pairs_from(widgets):
        if presentation(source) != source_before[source.get_name()]:
            raise RuntimeError(f"Victory presentation changed: {source.get_name()}")
        if presentation(target) != presentation(source):
            raise RuntimeError(f"Presentation mismatch: {target.get_name()}")
        unreal.log(f"[Plan156StatSync] matched {source.get_name()} -> {target.get_name()}")

    if not verify_only and not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError("Settlement blueprint save failed")
    unreal.log(f"[Plan156StatSync] SUCCESS pairs={len(pairs)} verify_only={verify_only}")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        unreal.SystemLibrary.request_exit_with_status(True, 1)
        raise
