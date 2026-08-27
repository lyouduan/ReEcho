#!/usr/bin/env python3
"""Author and verify Plan130's full-screen countdown ghost post-process material."""

import unreal


MATERIAL_DIR = "/Game/ReEcho/Materials/PostProcess/EncounterTransition"
MATERIAL_NAME = "M_PP_EncounterCountdownGhost_V4"
MATERIAL_PATH = f"{MATERIAL_DIR}/{MATERIAL_NAME}"
PARAMETERS = {
    "EffectStrength": 0.0,
    "GhostOffsetPixels": 3.0,
    "BlurRadiusPixels": 0.0,
    "GhostOpacity": 0.0,
    "PulsePhase": 0.0,
    "ClearCenterRadius": 0.28,
    "EdgeBlurRadius": 0.62,
    "EdgeMaskPower": 1.4,
    "RGBSeparationEnabled": 1.0,
    "RGBSeparationStrength": 1.0,
}

CUSTOM_CODE = r"""
float2 uv = GetDefaultSceneTextureUV(Parameters, 14);
float2 invSize = View.ViewSizeAndInvSize.zw;
float aspectRatio = View.ViewSizeAndInvSize.x * View.ViewSizeAndInvSize.w;
float2 centeredUV = uv - 0.5;
centeredUV.x *= aspectRatio;
float radialDistance = length(centeredUV);
float edgeMask = smoothstep(ClearCenterRadius, max(ClearCenterRadius + 0.001, EdgeBlurRadius), radialDistance);
edgeMask = pow(saturate(edgeMask), max(EdgeMaskPower, 0.001));
float localStrength = saturate(EffectStrength) * edgeMask;
float2 direction = normalize(float2(cos(PulsePhase), sin(PulsePhase * 0.7) * 0.35 + 0.001));
float2 perpendicular = float2(-direction.y, direction.x);
float2 ghostStep = direction * GhostOffsetPixels * invSize;
float2 blurStep = perpendicular * BlurRadiusPixels * invSize;

float3 ghost1 = SceneTextureLookup(saturate(uv + ghostStep), 14, true).rgb;
float3 ghost2 = SceneTextureLookup(saturate(uv + ghostStep * 2.0), 14, true).rgb;
float3 ghostBack = SceneTextureLookup(saturate(uv - ghostStep * 0.5), 14, true).rgb;
float3 blurPositive = SceneTextureLookup(saturate(uv + blurStep), 14, true).rgb;
float3 blurNegative = SceneTextureLookup(saturate(uv - blurStep), 14, true).rgb;

float3 channelGhost = float3(ghostBack.r, ghost1.g, ghost2.b);
float3 neutralGhost = SceneColor * 0.45 + ghost1 * 0.27 + ghost2 * 0.18 + ghostBack * 0.10;
float3 separatedGhost = SceneColor * 0.35 + ghost1 * 0.20 + ghost2 * 0.12 + ghostBack * 0.08 + channelGhost * 0.25;
float separationBlend = saturate(RGBSeparationEnabled) * saturate(RGBSeparationStrength);
float3 ghostComposite = lerp(neutralGhost, separatedGhost, separationBlend);
float3 ghosted = lerp(SceneColor, ghostComposite, saturate(GhostOpacity));
float3 blurred = ghosted * 0.6 + blurPositive * 0.2 + blurNegative * 0.2;
return lerp(SceneColor, blurred, localStrength);
"""


def fail(message):
    raise RuntimeError(f"[Plan130] {message}")


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def custom_input(name):
    value = unreal.CustomInput()
    value.set_editor_property("input_name", name)
    return value


def ensure_material():
    asset_exists = unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH)
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH) if asset_exists else None
    if material is None:
        unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            MATERIAL_NAME,
            MATERIAL_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not isinstance(material, unreal.Material):
        fail(f"Asset is not a Material: {MATERIAL_PATH}")
    if asset_exists:
        return material

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property(
        "blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING
    )
    material.set_editor_property("blendable_priority", 0)
    material.set_editor_property("blendable_output_alpha", False)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    scene_color = expression(material, unreal.MaterialExpressionSceneTexture, -1100, -220)
    scene_color.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    scene_color.set_editor_property("filtered", True)

    scalar_nodes = {}
    for index, (name, default_value) in enumerate(PARAMETERS.items()):
        node = expression(
            material, unreal.MaterialExpressionScalarParameter, -1100, 20 + index * 130
        )
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", default_value)
        scalar_nodes[name] = node

    custom = expression(material, unreal.MaterialExpressionCustom, -520, -180)
    custom.set_editor_property("description", "Plan130 asymmetric countdown ghost blur")
    custom.set_editor_property("code", CUSTOM_CODE)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property(
        "inputs",
        [custom_input(name) for name in ["SceneColor", *PARAMETERS.keys()]],
    )

    unreal.MaterialEditingLibrary.connect_material_expressions(scene_color, "Color", custom, "SceneColor")
    for name, node in scalar_nodes.items():
        unreal.MaterialEditingLibrary.connect_material_expressions(node, "", custom, name)
    unreal.MaterialEditingLibrary.connect_material_property(
        custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if compile_errors:
        fail("Material compile failed: " + " | ".join(compile_errors))
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        fail(f"Could not save {MATERIAL_PATH}")
    return material


def verify(material):
    if material.get_editor_property("material_domain") != unreal.MaterialDomain.MD_POST_PROCESS:
        fail("Material domain is not Post Process")
    if (
        material.get_editor_property("blendable_location")
        != unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING
    ):
        fail("Material does not run after tonemapping")
    scalar_names = {
        str(name)
        for name in unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)
    }
    missing = set(PARAMETERS) - scalar_names
    if missing:
        fail(f"Missing scalar parameters: {sorted(missing)}")
    custom_nodes = [
        node
        for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
        if isinstance(node, unreal.MaterialExpressionCustom)
    ]
    if len(custom_nodes) != 1 or "SceneTextureLookup" not in custom_nodes[0].get_editor_property("code"):
        fail("Expected one multi-sample SceneTexture custom expression")
    compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if compile_errors:
        fail("Material verification compile failed: " + " | ".join(compile_errors))
    if not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        fail(f"Saved asset is missing: {MATERIAL_PATH}")
    unreal.log(
        f"[Plan130] PASS {material.get_path_name()} domain=PostProcess "
        f"location=AfterTonemapping scalars={sorted(scalar_names)}"
    )


verify(ensure_material())
