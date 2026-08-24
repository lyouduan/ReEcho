"""Read-only validation for the Plan90 VFX authoring assets and production isolation."""

import unreal


ROOT = "/Game/ReEcho/Testing/VFX"
BLUEPRINT_PATH = f"{ROOT}/BP_VFXPreviewRig"
LEVEL_PATH = f"{ROOT}/L_VFXAuthoring"
PARENT_PATH = "/Script/ReEcho.ReEchoVfxPreviewActor"


def fail(message):
    raise RuntimeError(f"[Plan90] {message}")


blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
parent_class = unreal.load_class(None, PARENT_PATH)
if not isinstance(blueprint, unreal.Blueprint) or parent_class is None:
    fail("preview Blueprint or native parent is missing")
generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(generated_class, parent_class):
    fail("preview Blueprint parent mismatch")
if not unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
    fail(f"missing test level {LEVEL_PATH}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(LEVEL_PATH):
    fail(f"could not load {LEVEL_PATH}")
rigs = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), generated_class)
if len(rigs) != 1:
    fail(f"expected exactly one preview rig, found {len(rigs)}")
preview_host_class = unreal.load_class(None, "/Script/ReEcho.ReEcho2DEditorPreviewActor")
preview_hosts = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), preview_host_class)
if len(preview_hosts) != 6:
    fail(f"expected six production Blueprint reference hosts, found {len(preview_hosts)}")
camera_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaCameraActor")
cameras = unreal.EditorFilterLibrary.by_class(actor_subsystem.get_all_level_actors(), camera_class)
if len(cameras) != 1:
    fail(f"expected one production camera reference, found {len(cameras)}")
scenario_target_class = unreal.load_class(None, "/Script/ReEcho.ReEchoVfxScenarioTargetActor")
scenario_targets = unreal.EditorFilterLibrary.by_class(
    actor_subsystem.get_all_level_actors(), scenario_target_class
)
if len(scenario_targets) < 4:
    fail(f"expected at least four editable scenario targets, found {len(scenario_targets)}")
orders = sorted(target.get_editor_property("conduct_order") for target in scenario_targets)
if orders[:4] != [0, 1, 2, 3]:
    fail(f"scenario target conduct order mismatch: {orders}")

dependencies = unreal.EditorAssetLibrary.find_package_referencers_for_asset(LEVEL_PATH, False)
production_referencers = [path for path in dependencies if not path.startswith(ROOT)]
if production_referencers:
    fail(f"production assets reference the test map: {production_referencers}")

default_object = unreal.get_default_object(generated_class)
notice = str(default_object.get_editor_property("authority_notice"))
if "not applied to production" not in notice.lower():
    fail("Sandbox authority warning is missing")
if default_object.get_editor_property("auto_release_on_begin_play"):
    fail("PIE must initialize Ready without automatically releasing VFX")
if abs(default_object.get_editor_property("camera_ortho_width") - 2560.0) > 0.01:
    fail("test camera default must match the production tilted orthographic width")
unreal.log("[Plan90] verified Blueprint parent, single rig, multi-target scenario, test-map isolation and Sandbox warning")
