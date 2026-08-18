#!/usr/bin/env python3
"""Build and verify the minimal ReEcho combat VFX asset closure."""

from __future__ import annotations

import argparse
import csv
import hashlib
import re
import shutil
import sys
from collections import deque
from dataclasses import dataclass
from pathlib import Path


ROOT_PACKAGES = (
    "/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Charging_01",
    "/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02",
    "/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_BeAttacked_01",
    "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_02",
    "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_01",
    "/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_04",
    "/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01",
    "/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01",
)

# Project-side assets may only differ from the supplied art package through a named,
# reviewed Unreal Editor adaptation. The source hash remains authoritative provenance;
# this allowlist makes the derived project bytes equally explicit and auditable.
PROJECT_ADAPTATIONS = {
    "VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02.uasset": (
        "4DDCA3728ADD001D6BB35367666900B79C3A570ABDCD7C6C36596D47B3039295",
        1263687,
        "Fountain004/Fountain005 emitters use Local Space so component rotation aims the burst",
    ),
}

MANIFEST_FIELDS = ("Package", "RelativePath", "Sha256", "Bytes", "RequiredBy")
ASCII_PACKAGE_RE = re.compile(rb"/Game/[A-Za-z0-9_./-]+")


@dataclass(frozen=True)
class ManifestRow:
    package: str
    relative_path: str
    sha256: str
    size: int
    required_by: str


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def package_from_relative(relative_path: Path) -> str:
    return "/Game/" + relative_path.with_suffix("").as_posix()


def normalize_reference(reference: str) -> str:
    reference = reference.rstrip("./")
    if "." in reference:
        reference = reference.split(".", 1)[0]
    return reference


def extract_package_references(path: Path) -> set[str]:
    data = path.read_bytes()
    references = {normalize_reference(match.decode("ascii")) for match in ASCII_PACKAGE_RE.findall(data)}
    if len(data) >= 2:
        for offset in (0, 1):
            utf16_text = data[offset:].decode("utf-16-le", errors="ignore")
            references.update(
                normalize_reference(match)
                for match in re.findall(r"/Game/[A-Za-z0-9_./-]+", utf16_text)
            )
    return {reference for reference in references if reference.startswith("/Game/")}


def build_source_index(source_root: Path) -> dict[str, Path]:
    index: dict[str, Path] = {}
    for path in source_root.rglob("*.uasset"):
        relative_path = path.relative_to(source_root)
        package = package_from_relative(relative_path)
        if package in index:
            raise RuntimeError(f"duplicate package path: {package}")
        index[package] = path
    return index


def resolve_closure(source_root: Path) -> tuple[dict[str, Path], dict[str, set[str]]]:
    index = build_source_index(source_root)
    required_by: dict[str, set[str]] = {}
    missing_roots = [package for package in ROOT_PACKAGES if package not in index]
    if missing_roots:
        raise RuntimeError("missing root packages: " + ", ".join(missing_roots))

    for root_package in ROOT_PACKAGES:
        queue = deque([root_package])
        visited: set[str] = set()
        while queue:
            package = queue.popleft()
            if package in visited:
                continue
            visited.add(package)
            required_by.setdefault(package, set()).add(root_package)
            for reference in sorted(extract_package_references(index[package])):
                if reference in index:
                    if reference not in visited:
                        queue.append(reference)
                    continue
                # Serialized name tables can expose directory entries or a truncated prefix
                # alongside the complete package name. A complete indexed package with that
                # prefix proves the reference is not an unresolved dependency.
                if not any(candidate.startswith(reference) for candidate in index):
                    raise RuntimeError(f"unresolved /Game reference in {package}: {reference}")
    return index, required_by


def make_manifest_rows(source_root: Path) -> list[ManifestRow]:
    index, required_by = resolve_closure(source_root)
    rows = []
    for package in sorted(required_by):
        path = index[package]
        rows.append(
            ManifestRow(
                package=package,
                relative_path=path.relative_to(source_root).as_posix(),
                sha256=sha256_file(path),
                size=path.stat().st_size,
                required_by=";".join(sorted(required_by[package])),
            )
        )
    return rows


def write_manifest(path: Path, rows: list[ManifestRow]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=MANIFEST_FIELDS, lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow(
                {
                    "Package": row.package,
                    "RelativePath": row.relative_path,
                    "Sha256": row.sha256,
                    "Bytes": row.size,
                    "RequiredBy": row.required_by,
                }
            )


def read_manifest(path: Path) -> list[ManifestRow]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != MANIFEST_FIELDS:
            raise RuntimeError(f"unexpected manifest columns: {reader.fieldnames}")
        return [
            ManifestRow(
                package=row["Package"],
                relative_path=row["RelativePath"],
                sha256=row["Sha256"],
                size=int(row["Bytes"]),
                required_by=row["RequiredBy"],
            )
            for row in reader
        ]


def verify_rows(source_root: Path, content_root: Path, expected_rows: list[ManifestRow], require_copy: bool) -> None:
    actual_rows = make_manifest_rows(source_root)
    if actual_rows != expected_rows:
        raise RuntimeError("manifest does not match the current dependency closure; run --write-manifest")
    for row in expected_rows:
        source_path = source_root / Path(row.relative_path)
        if source_path.stat().st_size != row.size or sha256_file(source_path) != row.sha256:
            raise RuntimeError(f"source asset changed: {row.relative_path}")
        target_path = content_root / Path(row.relative_path)
        if require_copy and not target_path.is_file():
            raise RuntimeError(f"project asset missing: {row.relative_path}")
        if target_path.is_file():
            expected_target_hash, expected_target_size = expected_project_asset(row)
            if (
                target_path.stat().st_size != expected_target_size
                or sha256_file(target_path) != expected_target_hash
            ):
                raise RuntimeError(f"project asset conflicts with manifest: {row.relative_path}")


def expected_project_asset(row: ManifestRow) -> tuple[str, int]:
    adaptation = PROJECT_ADAPTATIONS.get(row.relative_path)
    if adaptation:
        return adaptation[0], adaptation[1]
    return row.sha256, row.size


def copy_rows(source_root: Path, content_root: Path, rows: list[ManifestRow]) -> None:
    for row in rows:
        source_path = source_root / Path(row.relative_path)
        target_path = content_root / Path(row.relative_path)
        if target_path.exists():
            expected_target_hash, expected_target_size = expected_project_asset(row)
            if (
                target_path.stat().st_size != expected_target_size
                or sha256_file(target_path) != expected_target_hash
            ):
                raise RuntimeError(f"refusing to overwrite different project asset: {row.relative_path}")
            continue
        if row.relative_path in PROJECT_ADAPTATIONS:
            raise RuntimeError(
                "adapted project asset is missing; restore the tracked project copy before importing: "
                + row.relative_path
            )
        target_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, target_path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--project-root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("Design/Art/VFX/combat_vfx_import_manifest.csv"),
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write-manifest", action="store_true")
    mode.add_argument("--copy", action="store_true")
    mode.add_argument("--check", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_root = args.source_root.resolve()
    project_root = args.project_root.resolve()
    content_root = project_root / "Content"
    manifest_path = args.manifest
    if not manifest_path.is_absolute():
        manifest_path = project_root / manifest_path
    if not source_root.is_dir():
        raise RuntimeError(f"source root does not exist: {source_root}")

    if args.write_manifest:
        rows = make_manifest_rows(source_root)
        write_manifest(manifest_path, rows)
        print(f"[PASS] wrote combat VFX manifest: assets={len(rows)} roots={len(ROOT_PACKAGES)}")
        return 0

    rows = read_manifest(manifest_path)
    verify_rows(source_root, content_root, rows, require_copy=args.check)
    if args.copy:
        copy_rows(source_root, content_root, rows)
        verify_rows(source_root, content_root, rows, require_copy=True)
        print(f"[PASS] copied combat VFX closure: assets={len(rows)}")
    else:
        print(f"[PASS] combat VFX closure verified: assets={len(rows)} roots={len(ROOT_PACKAGES)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"[FAIL] {error}", file=sys.stderr)
        raise SystemExit(1)
