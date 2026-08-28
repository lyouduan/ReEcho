#!/usr/bin/env python3
"""Author Plan143 FileMediaSource assets for the two transparent HAP transitions."""

import os
import unreal


ROOT = "/Game/ReEcho/UI/EncounterTransition"
SOURCES = {
    "FMS_EncounterEndToCardChoiceV2": "EncounterEndToCardChoiceV2.mov",
    "FMS_CardChoiceToShop": "CardChoiceToShop.mov",
}


def fail(message):
    raise RuntimeError(f"[Plan143] {message}")


unreal.EditorAssetLibrary.make_directory(ROOT)
for asset_name, movie_name in SOURCES.items():
    asset_path = f"{ROOT}/{asset_name}"
    source = unreal.EditorAssetLibrary.load_asset(asset_path)
    if source is None:
        source = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            ROOT,
            unreal.FileMediaSource,
            unreal.FileMediaSourceFactoryNew(),
        )
    if source is None or not isinstance(source, unreal.FileMediaSource):
        fail(f"Could not create FileMediaSource at {asset_path}")

    movie_path = os.path.join(
        unreal.Paths.project_content_dir(), "Movies", "EncounterTransition", movie_name
    )
    if not os.path.isfile(movie_path):
        fail(f"Project movie is missing: {movie_path}")
    source.set_file_path(movie_path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(source, only_if_is_dirty=False):
        fail(f"Could not save {asset_path}")
    unreal.log(
        f"[Plan143] authored source={asset_path} movie={movie_path} "
        f"bytes={os.path.getsize(movie_path)}"
    )

unreal.log("[Plan143] AUTHOR PASS")
