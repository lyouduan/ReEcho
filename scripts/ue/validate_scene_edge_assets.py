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

if validated != 24:
    fail(f"Expected 24 entries, validated {validated}")
unreal.log("[Plan135] Validation passed for 24 textures, 3 master materials, and 24 material instances")
