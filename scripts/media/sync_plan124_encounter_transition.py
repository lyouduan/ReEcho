#!/usr/bin/env python3
"""Synchronize and audit the Plan124 encounter-transition PNG sequence."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DESTINATION = PROJECT_ROOT / "Content" / "Movies" / "EncounterTransition" / "CardChoice"
FRAME_COUNT = 121
WIDTH = 1920
HEIGHT = 1080


def expected_names() -> list[str]:
    return [f"frame_{index:04d}.png" for index in range(FRAME_COUNT)]


def png_size(path: Path) -> tuple[int, int]:
    with path.open("rb") as stream:
        signature = stream.read(24)
    if len(signature) != 24 or signature[:8] != b"\x89PNG\r\n\x1a\n" or signature[12:16] != b"IHDR":
        raise ValueError(f"not a valid PNG header: {path}")
    return struct.unpack(">II", signature[16:24])


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def audit(directory: Path) -> list[dict[str, object]]:
    names = expected_names()
    actual = sorted(path.name for path in directory.glob("*.png")) if directory.is_dir() else []
    if actual != names:
        missing = sorted(set(names) - set(actual))
        extra = sorted(set(actual) - set(names))
        raise RuntimeError(f"sequence mismatch in {directory}: missing={missing} extra={extra}")

    frames: list[dict[str, object]] = []
    for name in names:
        path = directory / name
        size = png_size(path)
        if size != (WIDTH, HEIGHT):
            raise RuntimeError(f"unexpected dimensions for {path}: {size}, expected {(WIDTH, HEIGHT)}")
        frames.append({"file": name, "width": size[0], "height": size[1], "sha256": sha256(path)})
    return frames


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source", type=Path, required=True, help="authoritative directory containing frame_0000.png"
    )
    parser.add_argument("--check", action="store_true", help="audit source and destination without writing")
    args = parser.parse_args()

    source_directory = args.source.resolve()
    source_frames = audit(source_directory)
    if not args.check:
        DESTINATION.mkdir(parents=True, exist_ok=True)
        for frame in source_frames:
            source = source_directory / str(frame["file"])
            destination = DESTINATION / source.name
            if not destination.exists() or sha256(destination) != frame["sha256"]:
                shutil.copy2(source, destination)

        manifest = {
            "frame_count": FRAME_COUNT,
            "frame_rate": 40,
            "dimensions": [WIDTH, HEIGHT],
            "source": "user-provided Plan124 frame sequence",
            "frames": source_frames,
        }
        (DESTINATION / "sequence_manifest.json").write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )

    destination_frames = audit(DESTINATION)
    for source, destination in zip(source_frames, destination_frames, strict=True):
        if source["sha256"] != destination["sha256"]:
            raise RuntimeError(f"hash mismatch: {source['file']}")

    manifest_path = DESTINATION / "sequence_manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError(f"missing manifest: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("frame_count") != FRAME_COUNT or manifest.get("frame_rate") != 40:
        raise RuntimeError("manifest metadata mismatch")
    if manifest.get("frames") != source_frames:
        raise RuntimeError("manifest frame hashes do not match the authoritative source")

    print(f"[PASS] {FRAME_COUNT} frames, {WIDTH}x{HEIGHT}, 40 FPS, source and destination hashes match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
