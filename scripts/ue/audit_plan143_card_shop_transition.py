#!/usr/bin/env python3
"""Audit Plan143 FileMediaSource assets without mutating them."""

import os
import unreal


ROOT = "/Game/ReEcho/UI/EncounterTransition"
SOURCES = {
    "FMS_EncounterEndToCardChoiceV2": "EncounterEndToCardChoiceV2.mov",
    "FMS_CardChoiceToShop": "CardChoiceToShop.mov",
}


def fail(message):
    raise RuntimeError(f"[Plan143] {message}")


for asset_name, movie_name in SOURCES.items():
    asset_path = f"{ROOT}/{asset_name}"
    source = unreal.EditorAssetLibrary.load_asset(asset_path)
    if source is None or not isinstance(source, unreal.FileMediaSource):
        fail(f"Missing FileMediaSource: {asset_path}")
    expected = os.path.normcase(
        os.path.abspath(
            os.path.join(
                unreal.Paths.project_content_dir(),
                "Movies",
                "EncounterTransition",
                movie_name,
            )
        )
    )
    stored_path = source.get_editor_property("file_path")
    actual = os.path.normcase(
        os.path.abspath(
            os.path.join(unreal.Paths.project_content_dir(), stored_path.removeprefix("./"))
            if stored_path.startswith("./")
            else stored_path
        )
    )
    if actual != expected:
        fail(f"{asset_path} resolves to {actual}, expected {expected}")
    if not os.path.isfile(expected):
        fail(f"Missing movie: {expected}")
    unreal.log(f"[Plan143] audited source={asset_path} movie={expected}")

unreal.log("[Plan143] AUDIT PASS")
