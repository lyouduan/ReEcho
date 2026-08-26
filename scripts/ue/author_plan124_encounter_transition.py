#!/usr/bin/env python3
"""Author and verify Plan124's persistent ImgMedia/UI asset chain."""

import os
import unreal


ROOT = "/Game/ReEcho/UI/EncounterTransition"
SOURCE_PATH = f"{ROOT}/IMS_EncounterTransition"
FILE_SOURCE_PATH = f"{ROOT}/FMS_EncounterTransition"
PLAYER_PATH = f"{ROOT}/MP_EncounterTransition"
TEXTURE_PATH = f"{ROOT}/MT_EncounterTransition"
MATERIAL_PATH = f"{ROOT}/M_UI_EncounterTransition"


def fail(message):
    raise RuntimeError(f"[Plan124] {message}")


def create_or_load(name, path, asset_class, factory_class):
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing is not None:
        if not isinstance(existing, asset_class):
            fail(f"Unexpected asset type at {path}: {type(existing)}")
        return existing, False
    factory = factory_class()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, asset_class, factory)
    if asset is None:
        fail(f"Could not create {path}")
    return asset, True


unreal.EditorAssetLibrary.make_directory(ROOT)

source, _ = create_or_load(
    "IMS_EncounterTransition",
    SOURCE_PATH,
    unreal.ImgMediaSource,
    unreal.ImgMediaSourceFactoryNew,
)
sequence_path = os.path.join(
    unreal.Paths.project_content_dir(),
    "Movies",
    "EncounterTransition",
    "CardChoice",
    "frame_0000.png",
)
source.set_sequence_path(sequence_path)
source.set_editor_property("frame_rate_override", unreal.FrameRate(40, 1))
source.set_editor_property("fill_gaps_in_sequence", False)

file_source, _ = create_or_load(
    "FMS_EncounterTransition",
    FILE_SOURCE_PATH,
    unreal.FileMediaSource,
    unreal.FileMediaSourceFactoryNew,
)
file_source.set_file_path(
    os.path.join(
        unreal.Paths.project_content_dir(),
        "Movies",
        "EncounterTransition",
        "EncounterTransitionAlpha.mov",
    )
)

player, _ = create_or_load(
    "MP_EncounterTransition", PLAYER_PATH, unreal.MediaPlayer, unreal.MediaPlayerFactoryNew
)
player.set_editor_property("loop", False)

texture, _ = create_or_load(
    "MT_EncounterTransition", TEXTURE_PATH, unreal.MediaTexture, unreal.MediaTextureFactoryNew
)
texture.set_editor_property("media_player", player)
texture.set_editor_property("auto_clear", True)
texture.set_editor_property("clear_color", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
texture.set_editor_property("new_style_output", True)

material, _ = create_or_load(
    "M_UI_EncounterTransition", MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew
)
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("two_sided", True)
parameter_samples = [
    node
    for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
    if isinstance(node, unreal.MaterialExpressionTextureSampleParameter2D)
    and str(node.get_editor_property("parameter_name")) == "MediaTexture"
]
if parameter_samples:
    media_sample = parameter_samples[0]
else:
    media_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -300, -100
    )
    media_sample.set_editor_property("parameter_name", "MediaTexture")
media_sample.set_editor_property("texture", texture)
media_sample.set_editor_property(
    "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
)
unreal.MaterialEditingLibrary.connect_material_property(
    media_sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
)
opacity_parameters = [
    node
    for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
    if isinstance(node, unreal.MaterialExpressionScalarParameter)
    and str(node.get_editor_property("parameter_name")) == "MediaOpacity"
]
if opacity_parameters:
    opacity = opacity_parameters[0]
else:
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -40, 220
    )
    opacity.set_editor_property("parameter_name", "MediaOpacity")
opacity.set_editor_property("default_value", 1.0)
unreal.MaterialEditingLibrary.connect_material_property(
    media_sample, "A", unreal.MaterialProperty.MP_OPACITY
)
compile_errors = unreal.MaterialEditingLibrary.recompile_material(material)
if compile_errors:
    fail("Material compile failed: " + " | ".join(compile_errors))

for asset in (source, file_source, player, texture, material):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        fail(f"Could not save {asset.get_path_name()}")

for path in (SOURCE_PATH, FILE_SOURCE_PATH, PLAYER_PATH, TEXTURE_PATH, MATERIAL_PATH):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        fail(f"Missing saved asset: {path}")

unreal.log(
    f"[Plan124] PASS source={SOURCE_PATH} file_source={FILE_SOURCE_PATH} player={PLAYER_PATH} "
    f"texture={TEXTURE_PATH} material={MATERIAL_PATH}"
)
