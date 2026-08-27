"""One-shot repair for the Plan134 Backdrop transform omission; never edits inserts."""

import unreal


PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
CORE_COMPONENTS = {
    "SceneRoot", "MapRoot", "VisualRoot", "GameplayRoot", "Ground", "GroundDetail",
    "PlantRoot", "MidDecoration", "Foreground", "Atmosphere", "SceneEffects", "Collision",
    "Backdrop", "Floor", "WallNorth", "WallSouth", "WallEast", "WallWest",
    "CameraClampBounds", "PlayerBounds", "EnemySpawnBounds",
}


def fail(message):
    raise RuntimeError(f"[Plan134 Backdrop Repair] {message}")


def object_path(value):
    return value.get_path_name() if value is not None else "None"


def component_entries(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is not None:
            result[str(obj.get_name()).removesuffix("_GEN_VARIABLE")] = obj
    return result


def artist_snapshot(blueprint):
    snapshot = {}
    for name, component in component_entries(blueprint).items():
        if name in CORE_COMPONENTS or not isinstance(component, unreal.SceneComponent):
            continue
        values = {
            "class": component.get_class().get_path_name(),
            "location": component.get_editor_property("relative_location"),
            "rotation": component.get_editor_property("relative_rotation"),
            "scale": component.get_editor_property("relative_scale3d"),
            "visible": component.get_editor_property("visible"),
            "hidden_in_game": component.get_editor_property("hidden_in_game"),
        }
        if isinstance(component, unreal.PrimitiveComponent):
            values["collision"] = component.get_collision_enabled()
            values["priority"] = component.get_editor_property("translucency_sort_priority")
            values["materials"] = tuple(
                object_path(component.get_material(index))
                for index in range(component.get_num_materials())
            )
        if isinstance(component, unreal.StaticMeshComponent):
            values["mesh"] = object_path(component.static_mesh)
        snapshot[name] = values
    return snapshot


blueprints = {}
snapshots = {}
for scene_id in SCENE_IDS:
    blueprint = unreal.EditorAssetLibrary.load_asset(f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}")
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Arena Blueprint {scene_id}")
    blueprints[scene_id] = blueprint
    snapshots[scene_id] = artist_snapshot(blueprint)

for scene_id, blueprint in blueprints.items():
    defaults = unreal.get_default_object(blueprint.generated_class())
    if not defaults.get_editor_property("use_editor_authored_scene_layout"):
        fail(f"{scene_id} is not an already-migrated editor-authored scene")
    entries = component_entries(blueprint)
    backdrop = entries.get("Backdrop")
    if not isinstance(backdrop, unreal.StaticMeshComponent) or backdrop.static_mesh is None:
        fail(f"{scene_id} has no valid Backdrop mesh component")
    half_extents = defaults.get_editor_property("backdrop_half_extents")
    backdrop.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, -0.5))
    backdrop.set_editor_property(
        "relative_rotation", unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0)
    )
    backdrop.set_editor_property(
        "relative_scale3d",
        unreal.Vector(half_extents.y * 2.0 / 100.0, half_extents.x * 2.0 / 100.0, 1.0),
    )
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if artist_snapshot(blueprint) != snapshots[scene_id]:
        fail(f"{scene_id} artist component snapshot changed")

for blueprint in blueprints.values():
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail(f"Could not save {blueprint.get_name()}")

unreal.log("[Plan134] Repaired Backdrop transforms for SC01-SC04 without changing artist inserts")
