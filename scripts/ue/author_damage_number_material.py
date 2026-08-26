"""Create and verify the damage-number TextRender material.

The asset duplicates UE's stock translucent text material so its signed-distance
font reconstruction remains exact.  DamageOpacity stays at its default value of
one; damage-number animation is scale-only and does not drive material opacity.
"""

import unreal


ASSET_DIR = "/Game/ReEcho/Fonts/DamageNumbers"
ASSET_NAME = "M_DamageNumberTextOpacity"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"
ENGINE_MATERIAL_PATH = "/Engine/EngineMaterials/DefaultTextMaterialTranslucent"
OPACITY_PARAMETER = "DamageOpacity"


def fail(message):
    raise RuntimeError(f"[DamageNumberMaterial] {message}")


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def connect(source, source_output, destination, destination_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, source_output, destination, destination_input
    ):
        fail(
            f"Unable to connect {source.get_name()}:{source_output} to "
            f"{destination.get_name()}:{destination_input}"
        )


def connect_property(source, source_output, material_property):
    if not unreal.MaterialEditingLibrary.connect_material_property(
        source, source_output, material_property
    ):
        fail(f"Unable to connect {source.get_name()} to {material_property}")


def ensure_material():
    unreal.EditorAssetLibrary.make_directory(ASSET_DIR)
    created = not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH)
    if created:
        if not unreal.EditorAssetLibrary.duplicate_asset(
            ENGINE_MATERIAL_PATH, ASSET_PATH
        ):
            fail(f"Unable to duplicate {ENGINE_MATERIAL_PATH} to {ASSET_PATH}")
    material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not isinstance(material, unreal.Material):
        fail(f"Asset is not a Material: {ASSET_PATH}")

    opacity = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_OPACITY
    )
    if created:
        if opacity is None:
            fail("Duplicated engine material is missing its opacity graph")
        faded_opacity = expression(
            material, unreal.MaterialExpressionMultiply, 420, 80
        )
        connect(opacity, "", faded_opacity, "A")
        connect_property(faded_opacity, "", unreal.MaterialProperty.MP_OPACITY)
    elif not isinstance(opacity, unreal.MaterialExpressionMultiply):
        fail("Existing material final opacity is not a multiply expression")
    else:
        faded_opacity = opacity

    opacity_parameter = next(
        (
            node
            for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
            if isinstance(node, unreal.MaterialExpressionScalarParameter)
            and str(node.get_editor_property("parameter_name")) == OPACITY_PARAMETER
        ),
        None,
    )
    if opacity_parameter is None:
        opacity_parameter = expression(
            material, unreal.MaterialExpressionScalarParameter, 180, 190
        )
        opacity_parameter.set_editor_property("parameter_name", OPACITY_PARAMETER)
        opacity_parameter.set_editor_property("default_value", 1.0)
    connect(opacity_parameter, "", faded_opacity, "B")

    compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if compile_errors:
        fail("Material compile failed: " + " | ".join(compile_errors))
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def verify(material):
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
        fail("Material is not translucent")
    opacity = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_OPACITY
    )
    if not isinstance(opacity, unreal.MaterialExpressionMultiply):
        fail("Final opacity is not multiplied by the fixed opacity parameter")
    opacity_parameters = [
        node
        for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
        if isinstance(node, unreal.MaterialExpressionScalarParameter)
        and str(node.get_editor_property("parameter_name")) == OPACITY_PARAMETER
    ]
    if len(opacity_parameters) != 1:
        fail(f"Expected exactly one {OPACITY_PARAMETER} scalar parameter")
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail(f"Saved asset is missing: {ASSET_PATH}")
    unreal.log(f"[DamageNumberMaterial] PASS {material.get_path_name()}")


verify(ensure_material())
