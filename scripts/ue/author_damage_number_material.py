"""Create and verify the damage-number TextRender material.

The engine's stock translucent text material does not guarantee that the
TextRender vertex alpha contributes to final opacity.  This material explicitly
multiplies the baked font mask by vertex alpha so SetTextRenderColor can animate
the number's fade.
"""

import unreal


ASSET_DIR = "/Game/ReEcho/Fonts/DamageNumbers"
ASSET_NAME = "M_DamageNumberText"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"


def fail(message):
    raise RuntimeError(f"[DamageNumberMaterial] {message}")


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def ensure_material():
    unreal.EditorAssetLibrary.make_directory(ASSET_DIR)
    created = not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH)
    material = (
        None if created else unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    )
    if created:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not isinstance(material, unreal.Material):
        fail(f"Asset is not a Material: {ASSET_PATH}")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("disable_depth_test", True)

    if created:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

        font_sample = expression(
            material, unreal.MaterialExpressionFontSampleParameter, -620, -80
        )
        font_sample.set_editor_property("parameter_name", "Font")
        vertex_color = expression(
            material, unreal.MaterialExpressionVertexColor, -620, 180
        )
        tinted_font = expression(
            material, unreal.MaterialExpressionMultiply, -320, -80
        )
        faded_mask = expression(
            material, unreal.MaterialExpressionMultiply, -320, 180
        )

        unreal.MaterialEditingLibrary.connect_material_expressions(
            font_sample, "RGB", tinted_font, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            vertex_color, "RGB", tinted_font, "B"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            font_sample, "A", faded_mask, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            vertex_color, "A", faded_mask, "B"
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            tinted_font, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            faded_mask, "", unreal.MaterialProperty.MP_OPACITY
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def verify(material):
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
        fail("Material is not translucent")
    if material.get_editor_property("shading_model") != unreal.MaterialShadingModel.MSM_UNLIT:
        fail("Material is not unlit")
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail(f"Saved asset is missing: {ASSET_PATH}")
    unreal.log(f"[DamageNumberMaterial] PASS {material.get_path_name()}")


verify(ensure_material())
