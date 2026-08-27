"""Read-only validation for Plan135 SC02-SC04 edge art assets."""

import importlib.util
from pathlib import Path

import unreal


EXPECTED_SIZES = {
    "SC02": {"上.png": (3760, 584), "下.png": (4275, 691), "左.png": (417, 1517), "右.png": (544, 1467)},
    "SC03": {
        "上边.png": (3931, 561), "下边.png": (3910, 397), "右边1.png": (324, 455),
        "右2.png": (215, 266), "右3.png": (189, 240), "左1.png": (217, 317),
        "左2.png": (255, 213), "左3.png": (232, 304),
    },
    "SC04": {
        "上边.png": (3384, 482), "下边.png": (4247, 514), "时钟.png": (361, 607),
        "月亮.png": (554, 499), "右边草.png": (386, 429), "右边草2.png": (386, 429),
        "左边草 拷贝 2.png": (386, 429), "右边人树.png": (518, 761),
        "左边人树.png": (283, 597), "左边人树-1.png": (141, 298),
        "左边花.png": (272, 189), "左边蘑菇.png": (386, 429),
    },
}


def fail(message):
    raise RuntimeError(f"[Plan135] {message}")


def load_import_contract():
    script_path = Path(unreal.Paths.project_dir()) / "scripts" / "ue" / "import_scene_edge_assets.py"
    spec = importlib.util.spec_from_file_location("plan135_import_contract", script_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


contract = load_import_contract()
validated = 0
validated_components = 0
check_initial_layout = "-Plan135InitialLayout" in unreal.SystemLibrary.get_command_line()
for scene_id, entries in contract.SCENES.items():
    art_root = f"/Game/ReEcho/Art/Scene/{scene_id}/EdgeInserts"
    material_root = f"{art_root}/Materials"
    parent = unreal.EditorAssetLibrary.load_asset(f"{material_root}/M_{scene_id}EdgeInsert")
    if not isinstance(parent, unreal.Material):
        fail(f"Missing master material for {scene_id}")
    if parent.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
        fail(f"Master material is not translucent for {scene_id}")
    if parent.get_editor_property("shading_model") != unreal.MaterialShadingModel.MSM_UNLIT:
        fail(f"Master material is not unlit for {scene_id}")
    if not parent.get_editor_property("two_sided"):
        fail(f"Master material is not two-sided for {scene_id}")

    for source_name, semantic_name in entries:
        source_path = (
            Path(unreal.Paths.project_content_dir())
            / "ReEcho" / "Art" / "Scene" / scene_id / "EdgeInserts" / source_name
        )
        if not source_path.is_file():
            fail(f"Missing source PNG: {source_path}")
        texture = unreal.EditorAssetLibrary.load_asset(f"{art_root}/T_{scene_id}Edge_{semantic_name}")
        if not isinstance(texture, unreal.Texture2D):
            fail(f"Missing texture for {scene_id}/{semantic_name}")
        actual_size = (texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
        if actual_size != EXPECTED_SIZES[scene_id][source_name]:
            fail(f"Unexpected dimensions for {scene_id}/{source_name}: {actual_size}")
        instance = unreal.EditorAssetLibrary.load_asset(f"{material_root}/MI_{scene_id}Edge_{semantic_name}")
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            fail(f"Missing material instance for {scene_id}/{semantic_name}")
        if instance.get_editor_property("parent") != parent:
            fail(f"Wrong material parent for {scene_id}/{semantic_name}")
        assigned = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
            instance, "InsertTexture"
        )
        if assigned != texture:
            fail(f"Wrong InsertTexture for {scene_id}/{semantic_name}")
        validated += 1

    blueprint = unreal.EditorAssetLibrary.load_asset(
        f"{contract.PREFAB_ROOT}/BP_ArenaScene_{scene_id}"
    )
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Missing scene Blueprint for {scene_id}")
    _, components = contract.component_entries(blueprint)
    plane = unreal.EditorAssetLibrary.load_asset(contract.PLANE_PATH)
    for name, semantic_name, parent_name, location, sort_priority in contract.PLACEMENTS[scene_id]:
        if name not in components:
            fail(f"Missing direct component {scene_id}.{name}")
        component = components[name][1]
        if not isinstance(component, unreal.StaticMeshComponent):
            fail(f"Unexpected component type for {scene_id}.{name}")
        component_data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(components[name][0])
        parent_handle = unreal.SubobjectDataBlueprintFunctionLibrary.get_parent_handle(component_data)
        parent_data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(parent_handle)
        attach_parent = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(parent_data)
        parent_actual = "" if attach_parent is None else str(attach_parent.get_name()).removesuffix("_GEN_VARIABLE")
        if parent_actual != parent_name:
            fail(f"Wrong parent for {scene_id}.{name}: {parent_actual}")
        if component.get_editor_property("static_mesh") != plane:
            fail(f"Wrong plane mesh for {scene_id}.{name}")
        expected_material = unreal.EditorAssetLibrary.load_asset(
            f"{material_root}/MI_{scene_id}Edge_{semantic_name}"
        )
        if component.get_material(0) != expected_material:
            fail(f"Wrong material for {scene_id}.{name}")
        if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
            fail(f"Collision enabled for {scene_id}.{name}")
        if component.get_editor_property("cast_shadow"):
            fail(f"Shadow enabled for {scene_id}.{name}")
        if check_initial_layout:
            source_name = next(source for source, semantic in entries if semantic == semantic_name)
            width, height = EXPECTED_SIZES[scene_id][source_name]
            expected_scale = unreal.Vector(height / 100.0, width / 100.0, 1.0)
            actual_location = component.get_editor_property("relative_location")
            actual_rotation = component.get_editor_property("relative_rotation")
            actual_scale = component.get_editor_property("relative_scale3d")
            if not actual_location.equals(unreal.Vector(*location)):
                fail(f"Unexpected initial location for {scene_id}.{name}: {actual_location}")
            if abs(actual_rotation.pitch - 90.0) > 0.01 or any(abs(value) > 0.01 for value in (actual_rotation.yaw, actual_rotation.roll)):
                fail(f"Unexpected initial rotation for {scene_id}.{name}: {actual_rotation}")
            if not actual_scale.equals(expected_scale):
                fail(f"Unexpected initial scale for {scene_id}.{name}: {actual_scale}")
            if component.get_editor_property("translucency_sort_priority") != sort_priority:
                fail(f"Unexpected initial sort priority for {scene_id}.{name}")
        validated_components += 1

if validated != 24:
    fail(f"Expected 24 entries, validated {validated}")
if validated_components != 24:
    fail(f"Expected 24 components, validated {validated_components}")
unreal.log(
    "[Plan135] Validation passed for 24 textures, 3 master materials, "
    f"24 material instances, and {validated_components} editable Blueprint components"
)
