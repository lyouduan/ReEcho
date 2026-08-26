"""Validate every generated audio catalog path inside Unreal Editor."""
from __future__ import annotations

import csv
from pathlib import Path

import unreal


CATALOG = Path(unreal.Paths.project_content_dir()).resolve() / "Data" / "audio_events.csv"
EXPECTED_ROWS = 52
ASSET_ROOT = "/Game/ReEcho/Audio/"
EXPECTED_SILENT_EVENTS = {
    "Combat.Block",
    "Combat.Kill",
    "Enemy.Attack",
    "Boss.Spawn",
    "Boss.Attack",
    "Echo.Spawn",
    "Echo.Attack",
    "Echo.End",
}
REMOVED_ASSETS = {
    "/Game/ReEcho/Audio/Combat/Combat_Block.Combat_Block",
    "/Game/ReEcho/Audio/Combat/Combat_Kill.Combat_Kill",
    "/Game/ReEcho/Audio/Enemy/Enemy_Attack.Enemy_Attack",
    "/Game/ReEcho/Audio/Boss/Boss_Spawn.Boss_Spawn",
    "/Game/ReEcho/Audio/Boss/Boss_Attack.Boss_Attack",
    "/Game/ReEcho/Audio/Echo/Echo_Spawn.Echo_Spawn",
    "/Game/ReEcho/Audio/Echo/Echo_Attack.Echo_Attack",
    "/Game/ReEcho/Audio/Echo/Echo_End.Echo_End",
}


def main() -> None:
    with CATALOG.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))

    errors: list[str] = []
    if len(rows) != EXPECTED_ROWS:
        errors.append(f"expected {EXPECTED_ROWS} rows, found {len(rows)}")

    seen_keys: set[tuple[str, str]] = set()
    silent_events: set[str] = set()
    sound_wave_count = 0
    for row in rows:
        event_id = row["EventId"]
        variant_id = row["VariantId"]
        key = (event_id, variant_id)
        asset_path = row["AssetPath"]
        label = f"{event_id}/{variant_id}" if variant_id else event_id
        if key in seen_keys:
            errors.append(f"{label}: duplicate EventId/VariantId")
        seen_keys.add(key)
        if not asset_path:
            if variant_id or event_id not in EXPECTED_SILENT_EVENTS:
                errors.append(f"{label}: unexpected empty AssetPath")
            else:
                silent_events.add(event_id)
            continue
        if not asset_path.startswith(ASSET_ROOT):
            errors.append(f"{label}: AssetPath is outside {ASSET_ROOT}: {asset_path!r}")
            continue

        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.SoundWave):
            errors.append(f"{label}: does not resolve to SoundWave: {asset_path}")
            continue
        sound_wave_count += 1

        expected_looping = row["EventType"] == "Loop"
        actual_looping = bool(asset.get_editor_property("looping"))
        if actual_looping != expected_looping:
            errors.append(
                f"{label}: looping={actual_looping}, expected {expected_looping}"
            )

        channels = int(asset.get_editor_property("num_channels"))
        if row["Spatial3D"] == "true" and channels != 1:
            errors.append(f"{label}: spatial SoundWave must be mono, found {channels} channels")

    if silent_events != EXPECTED_SILENT_EVENTS:
        errors.append(
            f"silent event set mismatch: expected {sorted(EXPECTED_SILENT_EVENTS)}, found {sorted(silent_events)}"
        )
    for asset_path in sorted(REMOVED_ASSETS):
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            errors.append(f"retired placeholder asset still exists: {asset_path}")

    if errors:
        raise RuntimeError("Audio asset validation failed:\n" + "\n".join(errors))
    unreal.log(
        f"Validated {len(rows)} catalog rows: {sound_wave_count} bound SoundWave rows, "
        f"{len(silent_events)} intentional silent rows"
    )


main()
