#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
POINTER_PREFIX = b"version https://git-lfs.github.com/spec/v1"


class LfsSetupError(RuntimeError):
    pass


def run_git(*args: str, stream: bool = False) -> str:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=ROOT,
            check=False,
            text=True,
            encoding="utf-8",
            errors="replace",
            stdout=None if stream else subprocess.PIPE,
            stderr=None if stream else subprocess.STDOUT,
        )
    except FileNotFoundError as exc:
        raise LfsSetupError("Git is not installed or is not available on PATH") from exc

    output = "" if stream else (result.stdout or "").strip()
    if result.returncode != 0:
        detail = f"\n{output}" if output else ""
        raise LfsSetupError(f"git {' '.join(args)} failed with exit code {result.returncode}{detail}")
    return output


def tracked_lfs_paths() -> list[Path]:
    output = run_git("lfs", "ls-files", "--name-only")
    paths = [Path(line.strip()) for line in output.splitlines() if line.strip()]
    if not paths:
        raise LfsSetupError("HEAD contains no Git LFS paths; check .gitattributes and the current branch")
    return paths


def check_checkout() -> None:
    version = run_git("lfs", "version")
    run_git("lfs", "fsck")

    lfs_paths = tracked_lfs_paths()
    unresolved: list[str] = []
    missing: list[str] = []
    for relative_path in lfs_paths:
        path = (ROOT / relative_path).resolve()
        try:
            path.relative_to(ROOT)
        except ValueError as exc:
            raise LfsSetupError(f"Git LFS path escapes the repository: {relative_path}") from exc
        if not path.is_file():
            missing.append(relative_path.as_posix())
            continue
        with path.open("rb") as handle:
            if handle.read(256).startswith(POINTER_PREFIX):
                unresolved.append(relative_path.as_posix())

    if missing or unresolved:
        details = []
        if missing:
            details.append("missing: " + ", ".join(missing))
        if unresolved:
            details.append("still pointer files: " + ", ".join(unresolved))
        raise LfsSetupError("Git LFS checkout is not hydrated (" + "; ".join(details) + ")")

    print(f"[PASS] Git LFS checkout hydrated: files={len(lfs_paths)} ({version})")


def ensure_lfs_paths_clean_before_pull() -> None:
    paths = [path.as_posix() for path in tracked_lfs_paths()]
    dirty = run_git("status", "--porcelain=v1", "--", *paths)
    if dirty:
        raise LfsSetupError(
            "refusing git lfs pull because an LFS-tracked path has local changes; commit, stash, or preserve it first\n"
            + dirty
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Initialize and verify the ReEcho Git LFS checkout")
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify installed Git LFS and hydrated files without changing local Git configuration or downloading",
    )
    args = parser.parse_args()

    try:
        if not args.check:
            print("[INFO] Initializing Git LFS for this clone")
            run_git("lfs", "install", "--local", stream=True)
            ensure_lfs_paths_clean_before_pull()
            print("[INFO] Downloading Git LFS objects for the current checkout")
            run_git("lfs", "pull", stream=True)
        check_checkout()
    except LfsSetupError as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        print("Install Git LFS, then run: python scripts/setup_lfs.py", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
