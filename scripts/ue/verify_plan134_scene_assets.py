"""Read-only Plan134 scene authority audit; never saves or mutates assets."""

import unreal


CATALOG_PATH = "/Game/ReEcho/Scene/DA_ArenaSceneCatalog"
PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
REQUIRED_COMPONENTS = (
    "Backdrop",
    "Ground",
    "GroundDetail",
    "MidDecoration",
    "Foreground",
    "Atmosphere",
    "SceneEffects",
    "CameraClampBounds",
    "PlayerBounds",
    "EnemySpawnBounds",
)
CORE_COMPONENTS = set(REQUIRED_COMPONENTS) | {
    "SceneRoot", "MapRoot", "VisualRoot", "GameplayRoot", "PlantRoot",
    "Collision", "Floor", "WallNorth", "WallSouth", "WallEast", "WallWest",
}
GENERATED_TAG = "ReEchoGeneratedDecoration_Plan52"
PROGRAM_PROPERTIES = (
    "gameplay_plane_z", "depth_sort_axis", "depth_sort_world_units_per_step",
    "depth_sort_base_priority", "depth_sort_priority_range",
    "default_contact_shadow_texture", "default_contact_shadow_size",
    "default_contact_shadow_opacity", "enable_parallax",
    "mid_decoration_parallax_factor", "foreground_parallax_factor",
    "atmosphere_parallax_factor", "maximum_parallax_offset",
)


def fail(message):
    raise RuntimeError(f"[Plan134] {message}")


def stable_value(value):
    if isinstance(value, unreal.Vector):
        return (value.x, value.y, value.z)
    if isinstance(value, unreal.Vector2D):
        return (value.x, value.y)
    if isinstance(value, unreal.Rotator):
        return (value.pitch, value.yaw, value.roll)
    if isinstance(value, unreal.IntPoint):
        return (value.x, value.y)
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    return value


def components(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj is not None:
            result[str(obj.get_name()).removesuffix("_GEN_VARIABLE")] = obj
    return result


catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
if not isinstance(catalog, unreal.ReEchoArenaSceneCatalog):
    fail("Single Arena Scene Catalog is missing")
registrations = catalog.get_editor_property("scenes")
registered_ids = [str(item.get_editor_property("scene_id")) for item in registrations]
if registered_ids != list(SCENE_IDS) or len(set(registered_ids)) != len(SCENE_IDS):
    fail(f"Catalog must map SC01-SC04 exactly once: {registered_ids}")

catalog_only = "-Plan134CatalogOnly" in unreal.SystemLibrary.get_command_line()
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
gameplay_plane_world_z = {}
core_contracts = {}

for scene_id, registration in zip(SCENE_IDS, registrations):
    blueprint = unreal.EditorAssetLibrary.load_asset(
        f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}"
    )
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Arena Blueprint {scene_id}")
    defaults = unreal.get_default_object(blueprint.generated_class())
    if str(defaults.get_editor_property("scene_id")) != scene_id:
        fail(f"Incorrect SceneId on {scene_id}")
    if registration.get_editor_property("arena_class") != blueprint.generated_class():
        fail(f"Catalog class mismatch for {scene_id}")
    if catalog_only:
        continue
    scene_components = components(blueprint)
    missing = [name for name in REQUIRED_COMPONENTS if name not in scene_components]
    if missing:
        fail(f"{scene_id} is missing required components: {missing}")
    backdrop = scene_components["Backdrop"]
    if backdrop.get_material(0) is None:
        fail(f"{scene_id} Backdrop Material Slot 0 is empty")
    if not defaults.get_editor_property("use_editor_authored_scene_layout"):
        fail(f"{scene_id} has not enabled editor-authored scene layout")
    if defaults.get_editor_property("scene_profile") is not None:
        fail(f"{scene_id} still has a SceneProfile visual authority")
    if defaults.get_editor_property("map_material") is not None:
        fail(f"{scene_id} still has a duplicate actor MapMaterial")
    if defaults.get_editor_property("scene_registry"):
        fail(f"{scene_id} still stores a duplicate SceneRegistry")
    core_contracts[scene_id] = {
        "properties": tuple(stable_value(defaults.get_editor_property(name)) for name in PROGRAM_PROPERTIES),
        "root_pose": tuple(
            (
                stable_value(scene_components[name].get_editor_property("relative_location")),
                stable_value(scene_components[name].get_editor_property("relative_rotation")),
            )
            for name in ("SceneRoot", "MapRoot")
        ),
        "gameplay_root": tuple(
            stable_value(scene_components["GameplayRoot"].get_editor_property(prop))
            for prop in ("relative_location", "relative_rotation", "relative_scale3d")
        ),
        "bounds": tuple(
            tuple(stable_value(scene_components[name].get_editor_property(prop)) for prop in (
                "relative_location", "relative_rotation", "relative_scale3d", "box_extent"
            ))
            for name in ("CameraClampBounds", "PlayerBounds", "EnemySpawnBounds")
        ),
        "collision_height": tuple(
            (
                scene_components[name].get_editor_property("relative_location").z,
                scene_components[name].get_editor_property("relative_scale3d").z,
            )
            for name in ("Floor", "WallNorth", "WallSouth", "WallEast", "WallWest")
        ),
    }
    for name in ("CameraClampBounds", "PlayerBounds", "EnemySpawnBounds"):
        extent = scene_components[name].get_editor_property("box_extent")
        if min(extent.x, extent.y) < 100.0:
            fail(f"{scene_id}.{name} is degenerate: {extent}")
    for name, component in scene_components.items():
        if name in CORE_COMPONENTS or not isinstance(component, unreal.PrimitiveComponent):
            continue
        if (
            isinstance(component, unreal.StaticMeshComponent)
            and component.static_mesh is None
            and component.get_material(0) is None
        ):
            continue
        if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
            fail(
                f"{scene_id} artist visual component has collision enabled: {name}"
            )

    spawned = actor_subsystem.spawn_actor_from_class(
        blueprint.generated_class(), unreal.Vector(), unreal.Rotator()
    )
    if spawned is None:
        fail(f"Could not create transient validation actor for {scene_id}")
    try:
        gameplay_plane_world_z[scene_id] = spawned.get_gameplay_plane_world_z()
        unreal.log(
            f"[Plan134] {scene_id} GameplayPlaneWorldZ={gameplay_plane_world_z[scene_id]:.3f}"
        )
        if not spawned.does_backdrop_cover_camera_bounds(1.0):
            fail(
                f"{scene_id} transformed Backdrop mesh XY bounds do not cover CameraClampBounds"
            )
    finally:
        actor_subsystem.destroy_actor(spawned)

if not catalog_only:
    reference_z = gameplay_plane_world_z["SC01"]
    mismatched_z = {
        scene_id: value
        for scene_id, value in gameplay_plane_world_z.items()
        if abs(value - reference_z) > 0.01
    }
    if mismatched_z:
        fail(f"Gameplay plane world Z differs from SC01={reference_z}: {mismatched_z}")
    reference_contract = core_contracts["SC01"]
    for scene_id in SCENE_IDS[1:]:
        if core_contracts[scene_id] != reference_contract:
            differing = [
                key for key in reference_contract
                if core_contracts[scene_id][key] != reference_contract[key]
            ]
            fail(f"{scene_id} non-visual core differs from SC01: {differing}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_subsystem.load_level("/Game/Level00"):
    fail("Could not load Level00")
actors = actor_subsystem.get_all_level_actors()
arena_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneActor")
anchor_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaSceneSpawnAnchor")
camera_class = unreal.load_class(None, "/Script/ReEcho.ReEchoArenaCameraActor")
arenas = unreal.EditorFilterLibrary.by_class(actors, arena_class)
anchors = unreal.EditorFilterLibrary.by_class(actors, anchor_class)
cameras = unreal.EditorFilterLibrary.by_class(actors, camera_class)
generated = [
    actor for actor in actors if GENERATED_TAG in [str(tag) for tag in actor.tags]
]
if arenas or len(anchors) != 1 or len(cameras) != 1 or generated:
    fail(
        "Level00 authority mismatch: "
        f"arenas={len(arenas)} anchors={len(anchors)} cameras={len(cameras)} "
        f"tagged_decorations={len(generated)}"
    )

if catalog_only:
    unreal.log("[Plan134] Read-only catalog audit passed: SC01-SC04 resolve exactly once")
else:
    unreal.log("[Plan134] Read-only scene audit passed: catalog, materials, visual roots and gameplay bounds")
