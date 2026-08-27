"""Align non-visual Arena program contracts to SC01 while preserving scene visuals."""

import unreal


ROOT = "/Game/ReEcho/Scene/Prefabs"
IDS = ("SC01", "SC02", "SC03", "SC04")
PROGRAM_PROPERTIES = (
    "gameplay_plane_z", "depth_sort_axis", "depth_sort_world_units_per_step",
    "depth_sort_base_priority", "depth_sort_priority_range",
    "default_contact_shadow_texture", "default_contact_shadow_size",
    "default_contact_shadow_opacity", "enable_parallax",
    "mid_decoration_parallax_factor", "foreground_parallax_factor",
    "atmosphere_parallax_factor", "maximum_parallax_offset",
)
BOUNDS = ("CameraClampBounds", "PlayerBounds", "EnemySpawnBounds")
COLLISION = ("Floor", "WallNorth", "WallSouth", "WallEast", "WallWest")
VISUAL_CORE = {
    "SceneRoot", "MapRoot", "VisualRoot", "Ground", "GroundDetail", "PlantRoot",
    "MidDecoration", "Foreground", "Atmosphere", "SceneEffects", "Backdrop",
}


def fail(message):
    raise RuntimeError(f"[Plan134 Core Alignment] {message}")


def components(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    result = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if obj:
            result[str(obj.get_name()).removesuffix("_GEN_VARIABLE")] = obj
    return result


def object_path(value):
    return value.get_path_name() if value is not None else "None"


def visual_snapshot(bp):
    snapshot = {}
    for name, component in components(bp).items():
        if name not in VISUAL_CORE and not (
            isinstance(component, unreal.SceneComponent)
            and name not in BOUNDS and name not in COLLISION and name not in ("GameplayRoot", "Collision")
        ):
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
            values["materials"] = tuple(
                object_path(component.get_material(i)) for i in range(component.get_num_materials())
            )
            values["collision"] = component.get_collision_enabled()
            values["priority"] = component.get_editor_property("translucency_sort_priority")
        if isinstance(component, unreal.StaticMeshComponent):
            values["mesh"] = object_path(component.static_mesh)
        snapshot[name] = values
    return snapshot


blueprints = {}
entries = {}
defaults = {}
snapshots = {}
for scene_id in IDS:
    bp = unreal.EditorAssetLibrary.load_asset(f"{ROOT}/BP_ArenaScene_{scene_id}")
    if not isinstance(bp, unreal.Blueprint):
        fail(f"Missing {scene_id}")
    blueprints[scene_id] = bp
    entries[scene_id] = components(bp)
    defaults[scene_id] = unreal.get_default_object(bp.generated_class())
    snapshots[scene_id] = visual_snapshot(bp)

source_entries = entries["SC01"]
source_defaults = defaults["SC01"]
for scene_id in IDS[1:]:
    target_entries = entries[scene_id]
    target_defaults = defaults[scene_id]
    for prop in PROGRAM_PROPERTIES:
        target_defaults.set_editor_property(prop, source_defaults.get_editor_property(prop))

    target_entries["GameplayRoot"].set_editor_property(
        "relative_location", source_entries["GameplayRoot"].get_editor_property("relative_location")
    )
    target_entries["GameplayRoot"].set_editor_property(
        "relative_rotation", source_entries["GameplayRoot"].get_editor_property("relative_rotation")
    )
    target_entries["GameplayRoot"].set_editor_property(
        "relative_scale3d", source_entries["GameplayRoot"].get_editor_property("relative_scale3d")
    )

    for name in BOUNDS:
        source = source_entries[name]
        target = target_entries[name]
        target.set_editor_property("relative_location", source.get_editor_property("relative_location"))
        target.set_editor_property("relative_rotation", source.get_editor_property("relative_rotation"))
        target.set_editor_property("relative_scale3d", source.get_editor_property("relative_scale3d"))
        target.set_editor_property("box_extent", source.get_editor_property("box_extent"))

    # These legacy collision meshes attach directly to MapRoot. Their XY layout remains
    # scene-local; only the SC01 gameplay-height relationship and vertical thickness align.
    for name in COLLISION:
        source = source_entries[name]
        target = target_entries[name]
        source_location = source.get_editor_property("relative_location")
        source_scale = source.get_editor_property("relative_scale3d")
        target.set_editor_property(
            "relative_location", unreal.Vector(0.0, 0.0, source_location.z)
        )
        target.set_editor_property(
            "relative_rotation", unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0)
        )
        target.set_editor_property(
            "relative_scale3d", unreal.Vector(1.0, 1.0, source_scale.z)
        )

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprints[scene_id])
    if visual_snapshot(blueprints[scene_id]) != snapshots[scene_id]:
        fail(f"{scene_id} visual component snapshot changed")

for bp in blueprints.values():
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        fail(f"Could not save {bp.get_name()}")

unreal.log("[Plan134] Aligned SC02-SC04 world-space program contracts to SC01; visual snapshots unchanged")
