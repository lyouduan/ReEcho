"""Create and verify the fade-capable damage-number TextRender material.

The asset duplicates UE's stock translucent text material so its signed-distance
font reconstruction remains exact, then multiplies that opacity by TextRender's
vertex alpha.  SetTextRenderColor can therefore fade without changing glyphs.
"""

import unreal


ASSET_DIR = "/Game/ReEcho/Fonts/DamageNumbers"
ASSET_NAME = "M_DamageNumberTextOpacity"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"
ENGINE_MATERIAL_PATH = "/Engine/EngineMaterials/DefaultTextMaterialTranslucent"


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

    if created:
        default_opacity = unreal.MaterialEditingLibrary.get_material_property_input_node(
            material, unreal.MaterialProperty.MP_OPACITY
        )
        vertex_color = next(
            (
                node
                for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
                if isinstance(node, unreal.MaterialExpressionVertexColor)
            ),
            None,
        )
        if default_opacity is None or vertex_color is None:
            fail("Duplicated engine material is missing its opacity or vertex color graph")

        faded_opacity = expression(
            material, unreal.MaterialExpressionMultiply, 420, 80
        )

        connect(default_opacity, "", faded_opacity, "A")
        # Select A at the VertexColor node. Passing the full output through a
        # ComponentMask fails SM5 compilation because the existing Base Color
        # consumer narrows that shared expression to float3.
        connect(vertex_color, "A", faded_opacity, "B")
        connect_property(faded_opacity, "", unreal.MaterialProperty.MP_OPACITY)

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
        fail("Final opacity is not multiplied by TextRender vertex alpha")
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        fail(f"Saved asset is missing: {ASSET_PATH}")
    unreal.log(f"[DamageNumberMaterial] PASS {material.get_path_name()}")


verify(ensure_material())
