"""Validate every generated audio catalog path inside Unreal Editor."""
from __future__ import annotations

import csv
from pathlib import Path

import unreal


CATALOG = Path(unreal.Paths.project_content_dir()).resolve() / "Data" / "audio_events.csv"
EXPECTED_ROWS = 31
ASSET_ROOT = "/Game/ReEcho/Audio/"


def main() -> None:
    with CATALOG.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))

    errors: list[str] = []
    if len(rows) != EXPECTED_ROWS:
        errors.append(f"expected {EXPECTED_ROWS} rows, found {len(rows)}")

    seen_ids: set[str] = set()
    for row in rows:
        event_id = row["EventId"]
        asset_path = row["AssetPath"]
        if event_id in seen_ids:
            errors.append(f"{event_id}: duplicate EventId")
        seen_ids.add(event_id)
        if not asset_path.startswith(ASSET_ROOT):
            errors.append(f"{event_id}: AssetPath is outside {ASSET_ROOT}: {asset_path!r}")
            continue

        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.SoundWave):
            errors.append(f"{event_id}: does not resolve to SoundWave: {asset_path}")
            continue

        expected_looping = row["EventType"] == "Loop"
        actual_looping = bool(asset.get_editor_property("looping"))
        if actual_looping != expected_looping:
            errors.append(
                f"{event_id}: looping={actual_looping}, expected {expected_looping}"
            )

        channels = int(asset.get_editor_property("num_channels"))
        if row["Spatial3D"] == "true" and channels != 1:
            errors.append(f"{event_id}: spatial SoundWave must be mono, found {channels} channels")

    if errors:
        raise RuntimeError("Plan46 audio asset validation failed:\n" + "\n".join(errors))
    unreal.log(f"Plan46 validated {len(rows)} catalog SoundWaves")


main()
