"""Author and verify Plan70's reusable full-screen UI hurt vignette material.

Run with UnrealEditor-Cmd and -ExecutePythonScript. The operation is idempotent:
the graph is authored on first creation; an existing hard-referenced material is
validated without mutation so reruns cannot hit the Material Editor rooted-asset assertion.
"""

import unreal


MATERIAL_DIR = "/Game/ReEcho/Materials/UI"
MATERIAL_NAME = "M_UI_PlayerHurtVignette"
MATERIAL_PATH = f"{MATERIAL_DIR}/{MATERIAL_NAME}"


def fail(message):
    raise RuntimeError(f"[Plan70] {message}")


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def ensure_material():
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
        fail(f"Asset is not a Material: {MATERIAL_PATH}")

    # A loaded UI material can be rooted by a Widget CDO hard reference. Rebuilding
    # its expression graph in that state asserts inside the Material Editor. Existing
    # assets are therefore verified as an idempotent no-op; drift fails verification.
    if asset_exists:
        return material

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, -100)
    u = expression(material, unreal.MaterialExpressionComponentMask, -1000, -100)
    u.set_editor_property("r", True)
    one_minus_u = expression(material, unreal.MaterialExpressionOneMinus, -1000, 80)
    edge_distance = expression(material, unreal.MaterialExpressionMin, -780, -20)
    edge_width = expression(material, unreal.MaterialExpressionScalarParameter, -1000, 300)
    edge_width.set_editor_property("parameter_name", "EdgeWidth")
    edge_width.set_editor_property("default_value", 0.18)
    normalized_distance = expression(material, unreal.MaterialExpressionDivide, -560, -20)
    edge_mask = expression(material, unreal.MaterialExpressionOneMinus, -340, -20)
    saturated_mask = expression(material, unreal.MaterialExpressionSaturate, -120, -20)
    softness = expression(material, unreal.MaterialExpressionScalarParameter, -340, 220)
    softness.set_editor_property("parameter_name", "Softness")
    softness.set_editor_property("default_value", 2.2)
    soft_mask = expression(material, unreal.MaterialExpressionPower, 100, -20)
    tint = expression(material, unreal.MaterialExpressionVectorParameter, 100, 220)
    tint.set_editor_property("parameter_name", "TintColor")
    tint.set_editor_property("default_value", unreal.LinearColor(0.65, 0.01, 0.005, 1.0))

    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", u, "")
    unreal.MaterialEditingLibrary.connect_material_expressions(u, "", one_minus_u, "")
    unreal.MaterialEditingLibrary.connect_material_expressions(u, "", edge_distance, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(one_minus_u, "", edge_distance, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(edge_distance, "", normalized_distance, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(edge_width, "", normalized_distance, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(normalized_distance, "", edge_mask, "")
    unreal.MaterialEditingLibrary.connect_material_expressions(edge_mask, "", saturated_mask, "")
    unreal.MaterialEditingLibrary.connect_material_expressions(saturated_mask, "", soft_mask, "Base")
    unreal.MaterialEditingLibrary.connect_material_expressions(softness, "", soft_mask, "Exp")
    unreal.MaterialEditingLibrary.connect_material_property(
        tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        soft_mask, "", unreal.MaterialProperty.MP_OPACITY
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def verify(material):
    if material.get_editor_property("material_domain") != unreal.MaterialDomain.MD_UI:
        fail("Material domain is not User Interface")
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
        fail("Material blend mode is not Translucent")
    scalar_names = {
        str(name)
        for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)
    }
    vector_names = {
        str(name)
        for name in unreal.MaterialEditingLibrary.get_vector_parameter_names(material)
    }
    if not {"EdgeWidth", "Softness"}.issubset(scalar_names):
        fail(f"Missing scalar parameters: {scalar_names}")
    if "TintColor" not in vector_names:
        fail(f"Missing TintColor parameter: {vector_names}")
    if not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        fail(f"Saved asset is missing: {MATERIAL_PATH}")
    unreal.log(
        f"[Plan70] PASS {material.get_path_name()} domain=UI blend=Translucent "
        f"scalars={sorted(scalar_names)} vectors={sorted(vector_names)}"
    )


verify(ensure_material())
