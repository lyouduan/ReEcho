#!/usr/bin/env python3
"""Create the fade-capable reaction-label material and art-tunable Blueprint."""

import unreal


DEFAULT_TEXTURE_PATH = (
    "/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Burn"
)
MATERIAL_DIR = "/Game/ReEcho/Materials/UI/ElementReactions"
MATERIAL_NAME = "M_ElementReactionPopup"
MATERIAL_PATH = f"{MATERIAL_DIR}/{MATERIAL_NAME}"
BLUEPRINT_DIR = "/Game/ReEcho/UI/CombatHud"
BLUEPRINT_NAME = "BP_ReEchoElementReactionPopup"
BLUEPRINT_PATH = f"{BLUEPRINT_DIR}/{BLUEPRINT_NAME}"
PARENT_CLASS_PATH = "/Script/ReEcho.ReEchoElementReactionPopupActor"


def fail(message):
    raise RuntimeError(f"[ElementReactionPopupAuthor] {message}")


default_texture = unreal.load_asset(DEFAULT_TEXTURE_PATH)
parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
if not isinstance(default_texture, unreal.Texture2D):
    fail(f"Missing default reaction texture: {DEFAULT_TEXTURE_PATH}")
if parent_class is None:
    fail(f"Missing native popup class: {PARENT_CLASS_PATH}")

unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
material_created = not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH)
material = (
    None if material_created else unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
)
if material_created:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
if not isinstance(material, unreal.Material):
    fail(f"Unable to create Material: {MATERIAL_PATH}")
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
material.set_editor_property("disable_depth_test", True)
if material_created:
    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -420, 0
    )
    texture_sample.set_editor_property("parameter_name", "ReactionTexture")
    texture_sample.set_editor_property("texture", default_texture)
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 220
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    opacity_multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -120, 160
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
compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
if compile_errors:
    fail("Material compile failed: " + " | ".join(compile_errors))
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

unreal.EditorAssetLibrary.make_directory(BLUEPRINT_DIR)
blueprint_created = not unreal.EditorAssetLibrary.does_asset_exist(BLUEPRINT_PATH)
blueprint = (
    None if blueprint_created else unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
)
if blueprint_created:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        BLUEPRINT_NAME, BLUEPRINT_DIR, unreal.Blueprint, factory
    )
if not isinstance(blueprint, unreal.Blueprint):
    fail(f"Unable to create Blueprint: {BLUEPRINT_PATH}")
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
generated_class = blueprint.generated_class()
if generated_class is None or not unreal.MathLibrary.class_is_child_of(
    generated_class, parent_class
):
    fail(f"Blueprint does not derive from {PARENT_CLASS_PATH}")
defaults = unreal.get_default_object(generated_class)
if blueprint_created:
    defaults.set_editor_property("display_duration", 0.85)
    defaults.set_editor_property("spawn_height_cm", 105.0)
    defaults.set_editor_property("world_height_cm", 105.0)
    defaults.set_editor_property("rise_height_cm", 90.0)
    defaults.set_editor_property("fade_start_fraction", 0.15)
    defaults.set_editor_property("fade_curve_exponent", 1.3)
    defaults.set_editor_property("start_scale", 0.9)
    defaults.set_editor_property("end_scale", 1.0)
    defaults.set_editor_property("translucent_sort_priority", 30)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(
    blueprint, only_if_is_dirty=False
):
    fail(f"Unable to save Blueprint: {BLUEPRINT_PATH}")
unreal.log(
    f"[ElementReactionPopupAuthor] PASS material={MATERIAL_PATH} "
    f"blueprint={BLUEPRINT_PATH} status={'created' if blueprint_created else 'preserved'}"
)
