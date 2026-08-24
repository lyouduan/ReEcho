#!/usr/bin/env python3
"""Create the editable time-shard pickup prefab and its translucent material instance."""

import unreal


TEXTURE_PATH = "/Game/ReEcho/Textures/Pickups/T_TimeShard"
MATERIAL_PARENT_PATH = "/Paper2D/TranslucentUnlitSpriteMaterial"
MATERIAL_ROOT = "/Game/ReEcho/Materials/Pickups"
MATERIAL_NAME = "MI_TimeShardPickup"
MATERIAL_PATH = f"{MATERIAL_ROOT}/{MATERIAL_NAME}"
BLUEPRINT_ROOT = "/Game/ReEcho/Gameplay/Pickups"
BLUEPRINT_NAME = "BP_TimeShardPickup"
BLUEPRINT_PATH = f"{BLUEPRINT_ROOT}/{BLUEPRINT_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoTimeShardPickupActor"


def fail(message):
    raise RuntimeError(f"[Plan92] {message}")


texture = unreal.load_asset(TEXTURE_PATH)
parent_material = unreal.load_asset(MATERIAL_PARENT_PATH)
parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if not isinstance(texture, unreal.Texture2D):
    fail(f"missing pickup texture: {TEXTURE_PATH}")
if not isinstance(parent_material, unreal.MaterialInterface):
    fail(f"missing translucent sprite material: {MATERIAL_PARENT_PATH}")
if parent_class is None:
    fail(f"missing native pickup class: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(MATERIAL_ROOT)
material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
material_created = material is None
if material_created:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_ROOT,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )
    if material is None:
        fail(f"failed to create {MATERIAL_PATH}")
    material.set_editor_property("parent", parent_material)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        material, "SpriteTexture", texture
    )
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
elif not isinstance(material, unreal.MaterialInstanceConstant):
    fail(f"existing asset is not a MaterialInstanceConstant: {MATERIAL_PATH}")

unreal.EditorAssetLibrary.make_directory(BLUEPRINT_ROOT)
blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
blueprint_created = blueprint is None
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
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
elif not isinstance(blueprint, unreal.Blueprint):
    fail(f"existing asset is not a Blueprint: {BLUEPRINT_PATH}")

generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(generated_class, parent_class):
    fail(f"{BLUEPRINT_PATH} is not derived from {PARENT_CLASS_PATH}")

unreal.log(
    f"[Plan92] material={'created' if material_created else 'preserved'} "
    f"blueprint={'created' if blueprint_created else 'preserved'}: {BLUEPRINT_PATH}"
)
