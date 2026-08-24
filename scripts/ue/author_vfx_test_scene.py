"""Create Plan90 test-only assets once; preserve an existing artist-edited scene."""

import unreal


ROOT = "/Game/ReEcho/Testing/VFX"
BLUEPRINT_PATH = f"{ROOT}/BP_VFXPreviewRig"
LEVEL_PATH = f"{ROOT}/L_VFXAuthoring"
PARENT_PATH = "/Script/ReEcho.ReEchoVfxPreviewActor"


def fail(message):
    raise RuntimeError(f"[Plan90] {message}")


unreal.EditorAssetLibrary.make_directory(ROOT)
parent_class = unreal.load_class(None, PARENT_PATH)
if parent_class is None:
    fail(f"native preview class unavailable: {PARENT_PATH}")

blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
if blueprint is None:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_VFXPreviewRig", ROOT, unreal.Blueprint, factory
    )
    if blueprint is None:
        fail(f"failed to create {BLUEPRINT_PATH}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail(f"failed to save {BLUEPRINT_PATH}")
    unreal.log(f"[Plan90] created {BLUEPRINT_PATH}")
else:
    unreal.log(f"[Plan90] preserved existing {BLUEPRINT_PATH}")

generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(generated_class, parent_class):
    fail(f"{BLUEPRINT_PATH} does not derive from {PARENT_PATH}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_exists = unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH)
if level_exists:
    if not level_subsystem.load_level(LEVEL_PATH):
        fail(f"failed to load {LEVEL_PATH}")
    unreal.log(f"[Plan90] preserving artist-edited level {LEVEL_PATH}; only missing labeled actors may be added")
else:
    if not level_subsystem.new_level(LEVEL_PATH):
        fail(f"failed to create {LEVEL_PATH}")

actors_by_label = {actor.get_actor_label(): actor for actor in actor_subsystem.get_all_level_actors()}
changed = False
if "VFX Preview Rig - Production Read Only / Sandbox" not in actors_by_label:
    rig = actor_subsystem.spawn_actor_from_class(generated_class, unreal.Vector(0, 0, 0))
    if rig is None:
        fail("failed to place BP_VFXPreviewRig")
    rig.set_actor_label("VFX Preview Rig - Production Read Only / Sandbox")
    changed = True
for label, location in (
    ("Source", unreal.Vector(0, 0, 0)),
    ("Target", unreal.Vector(600, 0, 0)),
    ("Forward +X", unreal.Vector(200, 0, 0)),
    ("Character Height 180cm", unreal.Vector(0, -300, 90)),
    ("Foreground Occlusion Reference", unreal.Vector(300, 250, 0)),
    ("Background Occlusion Reference", unreal.Vector(300, -250, 0)),
):
    if label not in actors_by_label:
        marker = actor_subsystem.spawn_actor_from_class(unreal.TargetPoint, location)
        marker.set_actor_label(label)
        marker.tags = [unreal.Name("ReEchoVfxTestOnly")]
        changed = True
preview_host_class = unreal.load_class(None, "/Script/ReEcho.ReEcho2DEditorPreviewActor")
reference_classes = (
    ("Player Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay.BP_PlayerGameplay_C"),
    ("Echo Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EchoGameplay.BP_EchoGameplay_C"),
    ("Slime Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Slime.BP_EnemyGameplay_Slime_C"),
    ("Rabbit Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Rabbit.BP_EnemyGameplay_Rabbit_C"),
    ("Fox Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_Fox.BP_EnemyGameplay_Fox_C"),
    ("Sheep Reference", "/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EnemyGameplay_TimeGuard.BP_EnemyGameplay_TimeGuard_C"),
)
if preview_host_class is None:
    fail("native ReEcho2DEditorPreviewActor is unavailable")
for index, (label, class_path) in enumerate(reference_classes):
    reference_class = unreal.load_class(None, class_path)
    if reference_class is None:
        fail(f"production reference class is unavailable: {class_path}")
    if label not in actors_by_label:
        host = actor_subsystem.spawn_actor_from_class(
            preview_host_class, unreal.Vector(-500 + index * 200, -600, 0)
        )
        host.set_actor_label(label)
        host.set_editor_property("preview_actor_class", reference_class)
        changed = True
camera_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaCameraActor")
if camera_class is None:
    fail("production Arena camera class is unavailable")
if "Production Tilted Orthographic Camera Reference" not in actors_by_label:
    camera = actor_subsystem.spawn_actor_from_class(camera_class, unreal.Vector(0, 0, 1000))
    camera.set_actor_label("Production Tilted Orthographic Camera Reference")
    changed = True
scenario_target_class = unreal.load_class(None, "/Script/ReEcho.ReEchoVfxScenarioTargetActor")
if scenario_target_class is None:
    fail("native ReEchoVfxScenarioTargetActor is unavailable")
for index in range(4):
    label = f"Scenario Target {index + 1} - Editable"
    if label not in actors_by_label:
        target = actor_subsystem.spawn_actor_from_class(
            scenario_target_class, unreal.Vector(300 + index * 180, 300 + index * 120, 0)
        )
        target.set_actor_label(label)
        target.set_editor_property("target_id", unreal.Name(f"Preview.Target.{index + 1}"))
        target.set_editor_property("conduct_order", index)
        target.tags = [unreal.Name("ReEchoVfxScenarioTarget"), unreal.Name("ReEchoVfxTestOnly")]
        changed = True
if changed or not level_exists:
    if not level_subsystem.save_current_level():
        fail(f"failed to save {LEVEL_PATH}")
    unreal.log(f"[Plan90] {'created' if not level_exists else 'completed'} {LEVEL_PATH}")
else:
    unreal.log(f"[Plan90] no missing test actors; preserved {LEVEL_PATH} without saving")

unreal.log("[Plan90] authoring complete; no production asset was written")
