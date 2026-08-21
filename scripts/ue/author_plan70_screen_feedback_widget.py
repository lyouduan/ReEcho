"""Author and verify Plan70's editable screen-feedback Widget Blueprint.

Run with UnrealEditor-Cmd and -ExecutePythonScript. The operation is idempotent
and saves only through Unreal Editor APIs.
"""

import unreal


ASSET_DIR = "/Game/ReEcho/UI"
ASSET_NAME = "WBP_ReEchoPlayerScreenFeedback"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoPlayerScreenFeedbackWidget"


def fail(message):
    raise RuntimeError(f"[Plan70Widget] {message}")


def ensure_blueprint():
    blueprint = (
        unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH)
        else None
    )
    if blueprint is None:
        parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
        if parent_class is None:
            fail(f"Missing native parent class: {PARENT_CLASS_PATH}")
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME, ASSET_DIR, unreal.WidgetBlueprint, factory
        )
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        fail(f"Asset is not a WidgetBlueprint: {ASSET_PATH}")
    return blueprint


def set_if_present(default_object, name, value):
    try:
        default_object.set_editor_property(name, value)
        return True
    except Exception:
        return False


def configure_and_verify(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated_class = blueprint.generated_class()
    if generated_class is None:
        fail("Widget Blueprint has no generated class")
    default_object = unreal.get_default_object(generated_class)

    expected = {
        "low_health_threshold": 0.5,
        "maximum_low_health_intensity": 0.55,
        "low_health_curve_exponent": 2.0,
        "low_health_interpolation_speed": 5.0,
        "low_health_pulse_frequency": 1.2,
        "low_health_pulse_amplitude": 0.08,
        "hurt_pulse_duration": 0.5,
        "minimum_hurt_intensity": 0.55,
        "maximum_hurt_intensity": 0.8,
        "maximum_combined_intensity": 0.85,
        "material_edge_width": 0.18,
        "material_softness": 2.2,
    }
    configured = []
    for name, value in expected.items():
        if set_if_present(default_object, name, value):
            configured.append(name)
        else:
            fail(f"Generated class does not expose required tuning property: {name}")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail("Widget Blueprint failed to save")
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail("Saved Widget Blueprint is missing")

    reloaded = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not isinstance(reloaded, unreal.WidgetBlueprint):
        fail("Widget Blueprint failed to reload")
    unreal.log(
        f"[Plan70Widget] PASS {reloaded.get_path_name()} parent={PARENT_CLASS_PATH} "
        f"configured={sorted(configured)}"
    )


configure_and_verify(ensure_blueprint())
