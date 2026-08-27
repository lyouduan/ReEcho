#!/usr/bin/env python3
"""Author and verify Plan133's Stage 1-to-2 FileMediaSource."""

import os
import unreal


ROOT = "/Game/ReEcho/UI/EncounterTransition"
SOURCE_PATH = f"{ROOT}/FMS_Stage01To02"
SOUND_PATH = f"{ROOT}/S_Stage01To02"


def fail(message):
    raise RuntimeError(f"[Plan133] {message}")


unreal.EditorAssetLibrary.make_directory(ROOT)
source = unreal.EditorAssetLibrary.load_asset(SOURCE_PATH)
if source is None:
    source = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "FMS_Stage01To02",
        ROOT,
        unreal.FileMediaSource,
        unreal.FileMediaSourceFactoryNew(),
    )
if source is None or not isinstance(source, unreal.FileMediaSource):
    fail(f"Could not create FileMediaSource at {SOURCE_PATH}")

movie_path = os.path.join(
    unreal.Paths.project_content_dir(),
    "Movies",
    "EncounterTransition",
    "Stage01To02.mov",
)
if not os.path.isfile(movie_path):
    fail(f"Project movie is missing: {movie_path}")

source.set_file_path(movie_path)
if not unreal.EditorAssetLibrary.save_loaded_asset(source, only_if_is_dirty=False):
    fail(f"Could not save {SOURCE_PATH}")
if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_PATH):
    fail(f"Missing saved asset: {SOURCE_PATH}")

audio_path = os.path.join(
    unreal.Paths.project_content_dir(),
    "Movies",
    "EncounterTransition",
    "Stage01To02_Audio.wav",
)
if not os.path.isfile(audio_path):
    fail(f"Project audio is missing: {audio_path}")

if not unreal.EditorAssetLibrary.does_asset_exist(SOUND_PATH):
    task = unreal.AssetImportTask()
    task.filename = audio_path
    task.destination_path = ROOT
    task.destination_name = "S_Stage01To02"
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
if not unreal.EditorAssetLibrary.does_asset_exist(SOUND_PATH):
    fail(f"Could not import SoundWave at {SOUND_PATH}")

unreal.log(
    f"[Plan133] PASS source={SOURCE_PATH} movie={movie_path} "
    f"movie_bytes={os.path.getsize(movie_path)} sound={SOUND_PATH} audio_bytes={os.path.getsize(audio_path)}"
)
