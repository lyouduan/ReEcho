"""Align SC02-SC04 wall spatial transforms exactly to SC01.

Only WallEast, WallNorth, WallSouth and WallWest are changed. All other
component properties are snapshotted and verified before saving each BP.
"""

import unreal


PREFAB_ROOT = "/Game/ReEcho/Scene/Prefabs"
SCENE_IDS = ("SC01", "SC02", "SC03", "SC04")
WALLS = ("WallEast", "WallNorth", "WallSouth", "WallWest")
TRANSFORM_PROPERTIES = ("relative_location", "relative_rotation", "relative_scale3d")


def fail(message):
    raise RuntimeError(f"[Plan135 Walls] {message}")


def stable(value):
    if isinstance(value, unreal.Vector):
        return (value.x, value.y, value.z)
    if isinstance(value, unreal.Rotator):
        return (value.pitch, value.yaw, value.roll)
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


def component_snapshot(component):
    values = {
        "class": component.get_class().get_path_name(),
        "transform": tuple(stable(component.get_editor_property(prop)) for prop in TRANSFORM_PROPERTIES),
        "visible": component.get_editor_property("visible"),
        "hidden_in_game": component.get_editor_property("hidden_in_game"),
    }
    if isinstance(component, unreal.PrimitiveComponent):
        values["collision"] = component.get_collision_enabled()
        values["materials"] = tuple(
            stable(component.get_material(index)) for index in range(component.get_num_materials())
        )
    if isinstance(component, unreal.StaticMeshComponent):
        values["mesh"] = stable(component.get_editor_property("static_mesh"))
    if isinstance(component, unreal.BoxComponent):
        values["box_extent"] = stable(component.get_editor_property("box_extent"))
    return values


blueprints = {}
scene_components = {}
for scene_id in SCENE_IDS:
    blueprint = unreal.EditorAssetLibrary.load_asset(f"{PREFAB_ROOT}/BP_ArenaScene_{scene_id}")
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing Blueprint for {scene_id}")
    blueprints[scene_id] = blueprint
    scene_components[scene_id] = components(blueprint)
    missing = [name for name in WALLS if name not in scene_components[scene_id]]
    if missing:
        fail(f"{scene_id} missing walls: {missing}")

source = scene_components["SC01"]
for scene_id in SCENE_IDS[1:]:
    target = scene_components[scene_id]
    preserved = {
        name: component_snapshot(component)
        for name, component in target.items()
        if name not in WALLS and isinstance(component, unreal.SceneComponent)
    }
    for wall_name in WALLS:
        for prop in TRANSFORM_PROPERTIES:
            target[wall_name].set_editor_property(prop, source[wall_name].get_editor_property(prop))
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprints[scene_id])
    after = components(blueprints[scene_id])
    for name, expected in preserved.items():
        if name not in after or component_snapshot(after[name]) != expected:
            fail(f"{scene_id}.{name} changed while aligning walls")
    for wall_name in WALLS:
        source_transform = tuple(stable(source[wall_name].get_editor_property(prop)) for prop in TRANSFORM_PROPERTIES)
        target_transform = tuple(stable(after[wall_name].get_editor_property(prop)) for prop in TRANSFORM_PROPERTIES)
        if target_transform != source_transform:
            fail(f"{scene_id}.{wall_name} transform differs from SC01 after compile")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprints[scene_id], only_if_is_dirty=False):
        fail(f"Could not save {scene_id}")

unreal.log("[Plan135 Walls] SC02-SC04 WallEast/North/South/West transforms exactly match SC01")
