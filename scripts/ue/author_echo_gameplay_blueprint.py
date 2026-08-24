import unreal


ASSET_ROOT = "/Game/ReEcho/Gameplay/CharacterPrefabs"
ASSET_NAME = "BP_EchoGameplay"
ASSET_PATH = f"{ASSET_ROOT}/{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoEchoActor"


def fail(message):
    raise RuntimeError(f"[Plan71] {message}")


def validate_blueprint(blueprint, parent_class):
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"{ASSET_PATH} is not a Blueprint")
    generated_class = blueprint.generated_class()
    if generated_class is None:
        fail(f"{ASSET_PATH} has no generated class")
    if not unreal.MathLibrary.class_is_child_of(generated_class, parent_class):
        fail(f"{ASSET_PATH} is not derived from {PARENT_CLASS_PATH}")


parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if parent_class is None:
    fail(f"native parent is unavailable: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(ASSET_ROOT)
blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
created = blueprint is None
if created:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, ASSET_ROOT, unreal.Blueprint, factory
    )
    if blueprint is None:
        fail(f"failed to create {ASSET_PATH}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail(f"failed to save {ASSET_PATH}")

validate_blueprint(blueprint, parent_class)
reloaded = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
validate_blueprint(reloaded, parent_class)
unreal.log(
    f"[Plan71] Echo gameplay Blueprint {'created' if created else 'preserved'} and verified: {ASSET_PATH}"
)
