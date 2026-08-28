"""Deterministically remove white RGB contamination from Fox 2D alpha fringes."""

from __future__ import annotations

import argparse
import hashlib
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image


PROJECT_ROOT = Path(__file__).resolve().parents[2]
TEXTURE_DIRS = (
    PROJECT_ROOT / "Content/ReEcho/Art/Animation2D/Enemies/Fox/Born/Textures",
    PROJECT_ROOT / "Content/ReEcho/Art/Animation2D/Enemies/Fox/Walk/Textures",
)
ALPHA_SEED_THRESHOLD = 32


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()[:16]


def canonical_bleed(rgba: np.ndarray) -> np.ndarray:
    """Fill alpha-fringe RGB from the nearest protected pixel using deterministic L1 distance."""
    height, width, _ = rgba.shape
    protected = rgba[:, :, 3] >= ALPHA_SEED_THRESHOLD
    if not protected.any():
        raise RuntimeError("texture has no protected visible pixels")

    result = rgba.copy()
    visited = protected.copy()
    owner_y = np.full((height, width), -1, dtype=np.int32)
    owner_x = np.full((height, width), -1, dtype=np.int32)
    seed_y, seed_x = np.nonzero(protected)
    owner_y[seed_y, seed_x] = seed_y
    owner_x[seed_y, seed_x] = seed_x
    queue = deque(zip(seed_y.tolist(), seed_x.tolist()))

    # Fixed neighbor order makes ties stable across machines and repeated runs.
    while queue:
        y, x = queue.popleft()
        source_y = owner_y[y, x]
        source_x = owner_x[y, x]
        for next_y, next_x in ((y - 1, x), (y, x - 1), (y, x + 1), (y + 1, x)):
            if 0 <= next_y < height and 0 <= next_x < width and not visited[next_y, next_x]:
                visited[next_y, next_x] = True
                owner_y[next_y, next_x] = source_y
                owner_x[next_y, next_x] = source_x
                queue.append((next_y, next_x))

    fringe = ~protected
    result[fringe, :3] = rgba[owner_y[fringe], owner_x[fringe], :3]
    return result


def process(path: Path, apply: bool) -> tuple[bool, str]:
    with Image.open(path) as source:
        if source.mode != "RGBA":
            raise RuntimeError(f"expected RGBA source: {path.relative_to(PROJECT_ROOT)} mode={source.mode}")
        rgba = np.array(source, dtype=np.uint8)

    repaired = canonical_bleed(rgba)
    if not np.array_equal(rgba[:, :, 3], repaired[:, :, 3]):
        raise RuntimeError(f"alpha changed unexpectedly: {path.relative_to(PROJECT_ROOT)}")
    protected = rgba[:, :, 3] >= ALPHA_SEED_THRESHOLD
    if not np.array_equal(rgba[protected, :3], repaired[protected, :3]):
        raise RuntimeError(f"visible body RGB changed unexpectedly: {path.relative_to(PROJECT_ROOT)}")

    changed = not np.array_equal(rgba, repaired)
    if changed and apply:
        Image.fromarray(repaired, mode="RGBA").save(path, format="PNG", optimize=False, compress_level=9)

    relative = path.relative_to(PROJECT_ROOT).as_posix()
    evidence = (
        f"{relative} size={rgba.shape[1]}x{rgba.shape[0]} "
        f"alpha={digest(rgba[:, :, 3].tobytes())} "
        f"body={digest(rgba[protected].tobytes())} "
        f"canonical={digest(repaired.tobytes())} changed={int(changed)}"
    )
    return changed, evidence


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true", help="write canonical alpha-bleed RGB to the PNG files")
    args = parser.parse_args()

    paths = sorted(path for directory in TEXTURE_DIRS for path in directory.glob("*.png"))
    if len(paths) != 29:
        raise RuntimeError(f"expected 29 Fox Born/Walk PNG files, found {len(paths)}")

    changed = []
    for path in paths:
        needs_change, evidence = process(path, args.apply)
        print(evidence)
        if needs_change:
            changed.append(path)

    if changed and not args.apply:
        print(f"FOX_ALPHA_BLEED_FAILED noncanonical={len(changed)}")
        return 1
    print(f"FOX_ALPHA_BLEED_OK files={len(paths)} modified={len(changed) if args.apply else 0}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
