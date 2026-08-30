"""Create and verify the art-tunable damage-number Blueprint."""

import unreal


ASSET_DIR = "/Game/ReEcho/UI/CombatHud"
ASSET_NAME = "BP_ReEchoDamageNumber"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoDamageNumberActor"


def fail(message):
    raise RuntimeError(f"[DamageNumberBlueprint] {message}")


parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if parent_class is None:
    fail(f"Native parent is unavailable: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(ASSET_DIR)
blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
created = blueprint is None
if created:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, ASSET_DIR, unreal.Blueprint, factory
    )
if not isinstance(blueprint, unreal.Blueprint):
    fail(f"Asset is not a Blueprint: {ASSET_PATH}")

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(
    generated_class, parent_class
):
    fail(f"Blueprint does not derive from {PARENT_CLASS_PATH}")

defaults = unreal.get_default_object(generated_class)
if created:
    defaults.set_editor_property("display_duration", 0.9)
    defaults.set_editor_property("float_speed", 70.0)
    defaults.set_editor_property("start_scale", 1.15)
    defaults.set_editor_property("end_scale", 0.85)
    defaults.set_editor_property("critical_size_scale", 1.35)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    fail(f"Unable to save {ASSET_PATH}")
unreal.log(
    f"[DamageNumberBlueprint] PASS {ASSET_PATH} "
    f"status={'created' if created else 'preserved'}"
)
