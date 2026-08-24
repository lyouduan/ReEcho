#!/usr/bin/env python3
"""Create the editable time-shard pickup prefab and its fade-capable material."""

import unreal


TEXTURE_PATH = "/Game/ReEcho/Textures/Pickups/T_TimeShard"
MATERIAL_ROOT = "/Game/ReEcho/Materials/Pickups"
MASTER_MATERIAL_NAME = "M_TimeShardPickup"
MASTER_MATERIAL_PATH = f"{MATERIAL_ROOT}/{MASTER_MATERIAL_NAME}"
MATERIAL_NAME = "MI_TimeShardPickup"
MATERIAL_PATH = f"{MATERIAL_ROOT}/{MATERIAL_NAME}"
BLUEPRINT_ROOT = "/Game/ReEcho/Gameplay/Pickups"
BLUEPRINT_NAME = "BP_TimeShardPickup"
BLUEPRINT_PATH = f"{BLUEPRINT_ROOT}/{BLUEPRINT_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoTimeShardPickupActor"


def fail(message):
    raise RuntimeError(f"[Plan92] {message}")


texture = unreal.load_asset(TEXTURE_PATH)
parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if not isinstance(texture, unreal.Texture2D):
    fail(f"missing pickup texture: {TEXTURE_PATH}")
if parent_class is None:
    fail(f"missing native pickup class: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(MATERIAL_ROOT)
master_material_created = not unreal.EditorAssetLibrary.does_asset_exist(MASTER_MATERIAL_PATH)
master_material = (
    None
    if master_material_created
    else unreal.EditorAssetLibrary.load_asset(MASTER_MATERIAL_PATH)
)
if master_material_created:
    master_material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MASTER_MATERIAL_NAME,
        MATERIAL_ROOT,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
if not isinstance(master_material, unreal.Material):
    fail(f"failed to create {MASTER_MATERIAL_PATH}")
master_material.set_editor_property("disable_depth_test", True)
if master_material_created:
    master_material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    master_material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    master_material.set_editor_property("two_sided", True)
    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        master_material, unreal.MaterialExpressionTextureSampleParameter2D, -420, 0
    )
    texture_sample.set_editor_property("parameter_name", "SpriteTexture")
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        master_material, unreal.MaterialExpressionScalarParameter, -420, 220
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    opacity_multiply = unreal.MaterialEditingLibrary.create_material_expression(
        master_material, unreal.MaterialExpressionMultiply, -120, 160
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        texture_sample, "A", opacity_multiply, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        opacity, "", opacity_multiply, "B"
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        texture_sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity_multiply, "", unreal.MaterialProperty.MP_OPACITY
    )
unreal.MaterialEditingLibrary.recompile_material(master_material)
unreal.EditorAssetLibrary.save_loaded_asset(master_material, only_if_is_dirty=False)

material_created = not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH)
material = None if material_created else unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
if material_created:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_ROOT,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )
    if material is None:
        fail(f"failed to create {MATERIAL_PATH}")
elif not isinstance(material, unreal.MaterialInstanceConstant):
    fail(f"existing asset is not a MaterialInstanceConstant: {MATERIAL_PATH}")
material.set_editor_property("parent", master_material)
unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
    material, "SpriteTexture", texture
)
unreal.MaterialEditingLibrary.update_material_instance(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

unreal.EditorAssetLibrary.make_directory(BLUEPRINT_ROOT)
blueprint_created = not unreal.EditorAssetLibrary.does_asset_exist(BLUEPRINT_PATH)
blueprint = None if blueprint_created else unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
if blueprint_created:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        BLUEPRINT_NAME, BLUEPRINT_ROOT, unreal.Blueprint, factory
    )
    if blueprint is None:
        fail(f"failed to create {BLUEPRINT_PATH}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated_class = blueprint.generated_class()
    defaults = unreal.get_default_object(generated_class)
    defaults.set_editor_property("pickup_texture", texture)
    defaults.set_editor_property("pickup_material", material)
    defaults.set_editor_property("visual_world_height_cm", 76.0)
    defaults.set_editor_property("ground_sort_priority", -60)
    defaults.set_editor_property("show_ground_shadow", True)
    defaults.set_editor_property("snap_to_arena_ground_plane", True)
    defaults.set_editor_property("landing_bounce_height_cm", 18.0)
    defaults.set_editor_property("landing_bounce_duration_seconds", 0.45)
    defaults.set_editor_property("landing_bounce_count", 2)
    defaults.set_editor_property("collection_rise_duration_seconds", 0.28)
    defaults.set_editor_property("collection_rise_height_cm", 90.0)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
elif not isinstance(blueprint, unreal.Blueprint):
    fail(f"existing asset is not a Blueprint: {BLUEPRINT_PATH}")

generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(generated_class, parent_class):
    fail(f"{BLUEPRINT_PATH} is not derived from {PARENT_CLASS_PATH}")

unreal.log(
    f"[Plan92] master_material={'created' if master_material_created else 'updated'} "
    f"material={'created' if material_created else 'updated'} "
    f"blueprint={'created' if blueprint_created else 'preserved'}: {BLUEPRINT_PATH}"
)
