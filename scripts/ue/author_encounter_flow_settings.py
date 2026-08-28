#!/usr/bin/env python3
"""Create the dedicated Blueprint that owns encounter-flow tuning."""

import unreal


BLUEPRINT_ROOT = "/Game/ReEcho/Gameplay/Encounter"
BLUEPRINT_NAME = "BP_EncounterFlowSettings"
BLUEPRINT_PATH = f"{BLUEPRINT_ROOT}/{BLUEPRINT_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoEncounterFlowSettings"


def fail(message):
    raise RuntimeError(f"[EncounterFlowSettings] {message}")


parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if parent_class is None:
    fail(f"missing native settings class: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(BLUEPRINT_ROOT)
blueprint_created = not unreal.EditorAssetLibrary.does_asset_exist(BLUEPRINT_PATH)
blueprint = None if blueprint_created else unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
if blueprint_created:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        BLUEPRINT_NAME, BLUEPRINT_ROOT, unreal.Blueprint, factory
    )
if not isinstance(blueprint, unreal.Blueprint):
    fail(f"failed to create or load Blueprint: {BLUEPRINT_PATH}")

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(
    generated_class, parent_class
):
    fail(f"{BLUEPRINT_PATH} is not derived from {PARENT_CLASS_PATH}")

if blueprint_created:
    defaults = unreal.get_default_object(generated_class)
    defaults.set_editor_property("post_entry_invulnerability_seconds", 0.5)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log(
    f"[EncounterFlowSettings] blueprint="
    f"{'created' if blueprint_created else 'preserved'}: {BLUEPRINT_PATH}"
)
