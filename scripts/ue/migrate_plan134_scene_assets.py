"""One-shot Plan134 migration from legacy Construction authority to complete Arena Blueprints.

The migration uses the assets in this worktree only. It preserves all artist-owned
SC01 visual components, bakes legacy gameplay bounds into BoxComponent templates,
removes the one migration Arena and all authorized Plan52 tagged decorations from
Level00 (the authoritative baseline may already contain zero; any other count than
zero or the expected legacy 18 is rejected), then creates one native spawn anchor.
"""

import unreal


LEVEL_PATH = "/Game/Level00"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
GENERATED_TAG = "ReEchoGeneratedDecoration_Plan52"
CORE_COMPONENTS = {
    "SceneRoot",
    "MapRoot",
    "VisualRoot",
    "GameplayRoot",
    "Ground",
    "GroundDetail",
    "PlantRoot",
    "MidDecoration",
    "Foreground",
    "Atmosphere",
    "SceneEffects",
    "Collision",
    "Backdrop",
    "Floor",
    "WallNorth",
    "WallSouth",
    "WallEast",
    "WallWest",
    "CameraClampBounds",
    "PlayerBounds",
    "EnemySpawnBounds",
}


def fail(message):
    raise RuntimeError(f"[Plan134] {message}")


def object_path(value):
    return value.get_path_name() if value is not None else "None"


def component_entries(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is not None:
            name = str(obj.get_name()).removesuffix("_GEN_VARIABLE")
            result[name] = obj
    return result


def visual_snapshot(blueprint):
    snapshot = {}
    for name, component in component_entries(blueprint).items():
        if name in CORE_COMPONENTS or not isinstance(component, unreal.SceneComponent):
            continue
        entry = {
            "class": component.get_class().get_path_name(),
            "location": component.get_editor_property("relative_location"),
            "rotation": component.get_editor_property("relative_rotation"),
            "scale": component.get_editor_property("relative_scale3d"),
            "visible": component.get_editor_property("visible"),
            "hidden_in_game": component.get_editor_property("hidden_in_game"),
        }
        if isinstance(component, unreal.PrimitiveComponent):
            entry["collision"] = component.get_collision_enabled()
            entry["priority"] = component.get_editor_property(
                "translucency_sort_priority"
            )
            entry["materials"] = tuple(
                object_path(component.get_material(index))
                for index in range(component.get_num_materials())
            )
        if isinstance(component, unreal.StaticMeshComponent):
            entry["mesh"] = object_path(component.static_mesh)
        snapshot[name] = entry
    return snapshot


def set_bounds_component(component, half_extents, local_plane_z, z_offset):
    location = component.get_editor_property("relative_location")
    component.set_editor_property(
        "relative_location",
        unreal.Vector(location.x, location.y, local_plane_z + z_offset),
    )


def bake_legacy_backdrop_transform(component, half_extents):
    component.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -0.5))
    component.set_editor_property(
        "relative_rotation", unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0)
    )
    component.set_editor_property(
        "relative_scale3d",
        unreal.Vector(half_extents.y * 2.0 / 100.0, half_extents.x * 2.0 / 100.0, 1.0),
    )
    component.set_editor_property(
        "box_extent", unreal.Vector(half_extents.x, half_extents.y, 5.0)
    )


blueprints = {}
for scene_id in SCENE_IDS:
    blueprint = unreal.EditorAssetLibrary.load_asset(
        f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}"
    )
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Arena Blueprint {scene_id}")
    blueprints[scene_id] = blueprint

sc01_before = visual_snapshot(blueprints["SC01"])
if not sc01_before:
    fail("SC01 has no artist-owned visual components to preserve")

for scene_id, blueprint in blueprints.items():
    defaults = unreal.get_default_object(blueprint.generated_class())
    entries = component_entries(blueprint)
    for required in ("Backdrop", "CameraClampBounds", "PlayerBounds", "EnemySpawnBounds"):
        if required not in entries:
            fail(f"{scene_id} is missing {required}")
    if entries["Backdrop"].get_material(0) is None:
        fail(f"{scene_id} Backdrop Material Slot 0 is empty")
    bake_legacy_backdrop_transform(
        entries["Backdrop"], defaults.get_editor_property("backdrop_half_extents")
    )
    local_plane_z = defaults.get_editor_property("gameplay_plane_z")
    set_bounds_component(
        entries["CameraClampBounds"],
        defaults.get_editor_property("camera_clamp_half_extents"),
        local_plane_z,
        5.0,
    )
    set_bounds_component(
        entries["PlayerBounds"],
        defaults.get_editor_property("player_half_extents"),
        local_plane_z,
        10.0,
    )
    set_bounds_component(
        entries["EnemySpawnBounds"],
        defaults.get_editor_property("enemy_spawn_half_extents"),
        local_plane_z,
        15.0,
    )
    defaults.set_editor_property("use_editor_authored_scene_layout", True)
    defaults.set_editor_property("scene_profile", None)
    defaults.set_editor_property("map_material", None)
    defaults.set_editor_property("scene_registry", [])
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

sc01_after = visual_snapshot(blueprints["SC01"])
if sc01_after != sc01_before:
    changed = sorted(set(sc01_before).symmetric_difference(sc01_after))
    if not changed:
        changed = sorted(
            name for name in sc01_before if sc01_before[name] != sc01_after[name]
        )
    fail(f"SC01 artist component snapshot changed: {changed}")

for blueprint in blueprints.values():
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail(f"Could not save {blueprint.get_name()}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(LEVEL_PATH):
    fail(f"Could not load {LEVEL_PATH}")
actors = actor_subsystem.get_all_level_actors()
arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
anchor_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneSpawnAnchor")
camera_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaCameraActor")
if arena_class is None or anchor_class is None or camera_class is None:
    fail("Required native scene classes are unavailable")
arenas = unreal.EditorFilterLibrary.by_class(actors, arena_class)
anchors = unreal.EditorFilterLibrary.by_class(actors, anchor_class)
cameras = unreal.EditorFilterLibrary.by_class(actors, camera_class)
generated = [
    actor
    for actor in actors
    if GENERATED_TAG in [str(tag) for tag in actor.tags]
]
if len(arenas) != 1 or anchors or len(cameras) != 1 or len(generated) not in (0, 18):
    fail(
        "Level00 precondition mismatch: "
        f"arenas={len(arenas)} anchors={len(anchors)} cameras={len(cameras)} "
        f"tagged_decorations={len(generated)}"
    )

old_arena = arenas[0]
anchor = actor_subsystem.spawn_actor_from_class(
    anchor_class, old_arena.get_actor_location(), old_arena.get_actor_rotation()
)
if anchor is None:
    fail("Could not create Arena Scene Spawn Anchor")
anchor.set_actor_scale3d(old_arena.get_actor_scale3d())
anchor.set_actor_label("ArenaSceneSpawnAnchor")
for actor in generated:
    if not actor_subsystem.destroy_actor(actor):
        fail(f"Could not delete tagged decoration {actor.get_actor_label()}")
if not actor_subsystem.destroy_actor(old_arena):
    fail("Could not delete the fixed Level00 Arena")

remaining = actor_subsystem.get_all_level_actors()
if unreal.EditorFilterLibrary.by_class(remaining, arena_class):
    fail("Level00 still contains a fixed Arena")
if len(unreal.EditorFilterLibrary.by_class(remaining, anchor_class)) != 1:
    fail("Level00 does not contain exactly one Arena Scene Spawn Anchor")
if any(GENERATED_TAG in [str(tag) for tag in actor.tags] for actor in remaining):
    fail("Level00 still contains Plan52 tagged decorations")
if not level_subsystem.save_current_level():
    fail("Could not save migrated Level00")

unreal.log(
    f"[Plan134] Migrated {len(blueprints)} Arena Blueprints, preserved "
    f"{len(sc01_before)} SC01 artist components, deleted {len(generated)} tagged decorations, "
    "and replaced the fixed Arena with one spawn anchor"
)
