"""Create and verify the damage-number TextRender material.

The engine's stock translucent text material does not guarantee that the
TextRender vertex alpha contributes to final opacity.  This material explicitly
multiplies the baked font mask by vertex alpha so SetTextRenderColor can animate
the number's fade.
"""

import unreal


ASSET_DIR = "/Game/ReEcho/Fonts/DamageNumbers"
ASSET_NAME = "M_DamageNumberTextFade"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"
FONT_PATH = f"{ASSET_DIR}/F_DamageNumber_MFYuYue_Font"


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

        font = unreal.EditorAssetLibrary.load_asset(FONT_PATH)
        if not isinstance(font, unreal.Font):
            fail(f"Missing offline damage-number font: {FONT_PATH}")
        font_sample = expression(
            material, unreal.MaterialExpressionFontSampleParameter, -820, -120
        )
        font_sample.set_editor_property("parameter_name", "Font")
        font_sample.set_editor_property("font", font)
        font_sample.set_editor_property("font_texture_page", 0)
        vertex_color = expression(
            material, unreal.MaterialExpressionVertexColor, -820, 220
        )

        # FontSample and VertexColor expose anonymous masked outputs in UE 5.8.
        # Connect their full output to explicit masks instead of guessing pin names.
        font_mask = expression(
            material, unreal.MaterialExpressionComponentMask, -560, -120
        )
        font_mask.set_editor_property("r", True)
        vertex_rgb = expression(
            material, unreal.MaterialExpressionComponentMask, -560, 120
        )
        vertex_rgb.set_editor_property("r", True)
        vertex_rgb.set_editor_property("g", True)
        vertex_rgb.set_editor_property("b", True)
        vertex_alpha = expression(
            material, unreal.MaterialExpressionComponentMask, -560, 320
        )
        vertex_alpha.set_editor_property("a", True)
        faded_mask = expression(
            material, unreal.MaterialExpressionMultiply, -260, 80
        )

        connect(font_sample, "", font_mask, "")
        connect(vertex_color, "", vertex_rgb, "")
        connect(vertex_color, "", vertex_alpha, "")
        connect(font_mask, "", faded_mask, "A")
        connect(vertex_alpha, "", faded_mask, "B")
        connect_property(vertex_rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        connect_property(faded_mask, "", unreal.MaterialProperty.MP_OPACITY)

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
