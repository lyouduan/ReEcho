"""Build reviewed Plan40 collision-track DataAssets from explicit JSON annotations.

Usage inside Unreal Editor Python:
  unreal.py <project> -ExecutePythonScript=.../build_plan40_collision_tracks.py

The script intentionally does not inspect alpha pixels or invent polygons.
"""

import json
from pathlib import Path
import unreal


REPO = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ANNOTATION_DIR = REPO / "Content" / "ReEcho" / "Art" / "Animation2D" / "CollisionAnnotations"
OUTPUT_ROOT = "/Game/ReEcho/Animation2D/Collision"


def fail(message):
    raise RuntimeError(f"Plan40 collision annotation: {message}")


def polygon(vertices, label):
    if not 3 <= len(vertices) <= 16:
        fail(f"{label} must contain 3-16 vertices")
    result = unreal.ReEcho2DCollisionPolygon()
    result.set_editor_property("vertices", [unreal.Vector2D(float(x), float(y)) for x, y in vertices])
    return result


def get_or_create(name):
    path = f"{OUTPUT_ROOT}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        if not isinstance(existing, unreal.ReEcho2DFrameCollisionTrack):
            fail(f"{path} exists with the wrong class")
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.ReEcho2DFrameCollisionTrack)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, OUTPUT_ROOT, unreal.ReEcho2DFrameCollisionTrack, factory
    )


if not ANNOTATION_DIR.is_dir():
    fail(f"annotation directory does not exist: {ANNOTATION_DIR}")

unreal.EditorAssetLibrary.make_directory(OUTPUT_ROOT)
written = []
for path in sorted(ANNOTATION_DIR.glob("*.json")):
    data = json.loads(path.read_text(encoding="utf-8"))
    required = {"asset_name", "flipbook", "source_revision", "pixels_per_unreal_unit", "pivot_pixels", "frames"}
    missing = sorted(required - data.keys())
    if missing:
        fail(f"{path.name} missing fields: {', '.join(missing)}")
    if not data["source_revision"].strip():
        fail(f"{path.name} has an empty source_revision")
    flipbook = unreal.EditorAssetLibrary.load_asset(data["flipbook"])
    if not isinstance(flipbook, unreal.PaperFlipbook):
        fail(f"{path.name} flipbook is missing or not PaperFlipbook: {data['flipbook']}")
    if len(data["frames"]) != flipbook.get_num_frames():
        fail(f"{path.name} frame count does not match {data['flipbook']}")

    frames = []
    for index, source in enumerate(data["frames"]):
        frame = unreal.ReEcho2DFrameCollision()
        frame.set_editor_property(
            "body_hurtboxes",
            [polygon(value, f"{path.name} frame {index} body") for value in source.get("body_hurtboxes", [])],
        )
        frame.set_editor_property(
            "weapon_attack_hitboxes",
            [polygon(value, f"{path.name} frame {index} weapon") for value in source.get("weapon_attack_hitboxes", [])],
        )
        frame.set_editor_property("attack_active", bool(source.get("attack_active", False)))
        frames.append(frame)

    track = get_or_create(data["asset_name"])
    track.set_editor_property("source_flipbook", flipbook)
    track.set_editor_property("source_revision", data["source_revision"])
    track.set_editor_property("pixels_per_unreal_unit", float(data["pixels_per_unreal_unit"]))
    track.set_editor_property("pivot_pixels", unreal.Vector2D(*map(float, data["pivot_pixels"])))
    track.set_editor_property("frames", frames)
    unreal.EditorAssetLibrary.save_loaded_asset(track, only_if_is_dirty=False)
    written.append(track.get_path_name())

if not written:
    fail("no reviewed *.json annotations were found; no assets were created")
unreal.log(f"Plan40 collision tracks saved: {len(written)} ({', '.join(written)})")
