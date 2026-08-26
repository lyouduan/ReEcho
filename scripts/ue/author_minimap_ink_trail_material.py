#!/usr/bin/env python3
"""Create and verify the Slate-compatible minimap ink-trail UI material."""

import unreal


TIP_PATH = "/Game/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkBrushTip"
GRAIN_PATH = "/Game/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkGrain"
MATERIAL_DIR = "/Game/ReEcho/Materials/UI"
MATERIAL_NAME = "M_UI_MinimapInkTrail"
MATERIAL_PATH = f"{MATERIAL_DIR}/{MATERIAL_NAME}"


def fail(message):
    raise RuntimeError(f"[MinimapInkTrailAuthor] {message}")


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


tip = unreal.load_asset(TIP_PATH)
grain = unreal.load_asset(GRAIN_PATH)
if not isinstance(tip, unreal.Texture2D):
    fail(f"Missing brush-tip Texture2D: {TIP_PATH}")
if not isinstance(grain, unreal.Texture2D):
    fail(f"Missing grain Texture2D: {GRAIN_PATH}")

unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
asset_exists = unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH)
material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH) if asset_exists else None
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
if not isinstance(material, unreal.Material):
    fail(f"Unable to create Material: {MATERIAL_PATH}")

if not asset_exists:
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    tip_sample = expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, -180
    )
    tip_sample.set_editor_property("parameter_name", "BrushTipTexture")
    tip_sample.set_editor_property("texture", tip)
    grain_sample = expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -720, 80
    )
    grain_sample.set_editor_property("parameter_name", "GrainTexture")
    grain_sample.set_editor_property("texture", grain)
    grain_strength = expression(
        material, unreal.MaterialExpressionScalarParameter, -720, 300
    )
    grain_strength.set_editor_property("parameter_name", "GrainStrength")
    grain_strength.set_editor_property("default_value", 0.65)
    one = expression(material, unreal.MaterialExpressionConstant, -720, 450)
    one.set_editor_property("r", 1.0)
    grain_lerp = expression(material, unreal.MaterialExpressionLinearInterpolate, -430, 100)
    tip_grain = expression(material, unreal.MaterialExpressionMultiply, -180, -40)
    vertex_color = expression(material, unreal.MaterialExpressionVertexColor, -180, 220)
    final_opacity = expression(material, unreal.MaterialExpressionMultiply, 80, 20)

    unreal.MaterialEditingLibrary.connect_material_expressions(one, "", grain_lerp, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grain_sample, "R", grain_lerp, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(
        grain_strength, "", grain_lerp, "Alpha"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(tip_sample, "A", tip_grain, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(grain_lerp, "", tip_grain, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(tip_grain, "", final_opacity, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(vertex_color, "A", final_opacity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        vertex_color, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        final_opacity, "", unreal.MaterialProperty.MP_OPACITY
    )

compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
if compile_errors:
    fail("Material compile failed: " + " | ".join(compile_errors))
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

if material.get_editor_property("material_domain") != unreal.MaterialDomain.MD_UI:
    fail("Material domain is not User Interface")
if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
    fail("Material blend mode is not Translucent")
texture_parameters = {
    str(name)
    for name in unreal.MaterialEditingLibrary.get_texture_parameter_names(material)
}
scalar_parameters = {
    str(name)
    for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)
}
if not {"BrushTipTexture", "GrainTexture"}.issubset(texture_parameters):
    fail(f"Missing texture parameters: {sorted(texture_parameters)}")
if "GrainStrength" not in scalar_parameters:
    fail(f"Missing GrainStrength: {sorted(scalar_parameters)}")
unreal.log(
    f"[MinimapInkTrailAuthor] PASS {MATERIAL_PATH} "
    f"status={'preserved' if asset_exists else 'created'}"
)
