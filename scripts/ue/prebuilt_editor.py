#!/usr/bin/env python3
"""Create or verify the curated Win64 Unreal Editor module bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BINARY_DIR = ROOT / "Binaries" / "Win64"
MODULES_PATH = BINARY_DIR / "UnrealEditor.modules"
MANIFEST_PATH = BINARY_DIR / "ReEchoEditor.prebuilt.json"
TARGET_PATH = BINARY_DIR / "ReEchoEditor.target"
SCHEMA_VERSION = 1


class PrebuiltError(RuntimeError):
    pass


def relative(path: Path, root: Path = ROOT) -> str:
    return path.relative_to(root).as_posix()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_source_file(path: Path) -> str:
    content = path.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    return hashlib.sha256(content).hexdigest()


def source_paths(root: Path = ROOT) -> list[Path]:
    paths = [root / "ReEcho.uproject"]
    source_root = root / "Source"
    if source_root.is_dir():
        paths.extend(path for path in source_root.rglob("*") if path.is_file())

    plugins_root = root / "Plugins"
    if plugins_root.is_dir():
        paths.extend(plugins_root.rglob("*.uplugin"))
        paths.extend(path for path in plugins_root.glob("**/Source/**/*") if path.is_file())

    unique = {path.resolve(): path for path in paths if path.is_file()}
    return sorted(unique.values(), key=lambda path: relative(path, root))


def source_hashes(root: Path = ROOT) -> dict[str, str]:
    return {relative(path, root): sha256_source_file(path) for path in source_paths(root)}


def combined_fingerprint(hashes: dict[str, str]) -> str:
    digest = hashlib.sha256()
    for path, file_hash in sorted(hashes.items()):
        digest.update(path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_hash.encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise PrebuiltError(f"missing required prebuilt file: {relative(path)}") from exc
    except json.JSONDecodeError as exc:
        raise PrebuiltError(f"invalid JSON in {relative(path)}: {exc}") from exc


def project_contract(root: Path = ROOT) -> tuple[str, list[str]]:
    project_path = root / "ReEcho.uproject"
    project = load_json(project_path)
    association = project.get("EngineAssociation")
    modules = sorted(module.get("Name") for module in project.get("Modules", []) if module.get("Name"))
    if not association or not modules:
        raise PrebuiltError("ReEcho.uproject must declare EngineAssociation and at least one module")
    return str(association), modules


def built_contract(root: Path = ROOT) -> tuple[str, dict[str, str]]:
    modules_path = root / "Binaries" / "Win64" / "UnrealEditor.modules"
    document = load_json(modules_path)
    build_id = document.get("BuildId")
    modules = document.get("Modules")
    if not build_id or not isinstance(modules, dict) or not modules:
        raise PrebuiltError(f"{relative(modules_path, root)} lacks BuildId or Modules")
    return str(build_id), {str(name): str(filename) for name, filename in modules.items()}


def create_manifest(root: Path = ROOT) -> dict:
    association, declared_modules = project_contract(root)
    build_id, built_modules = built_contract(root)
    missing_modules = sorted(set(declared_modules) - set(built_modules))
    if missing_modules:
        raise PrebuiltError(f"UnrealEditor.modules is missing project modules: {', '.join(missing_modules)}")

    binary_dir = root / "Binaries" / "Win64"
    target_path = binary_dir / "ReEchoEditor.target"
    target = load_json(target_path)
    target_build_id = str(target.get("Version", {}).get("BuildId", ""))
    target_contract = (
        target.get("TargetName"),
        target.get("Platform"),
        target.get("Configuration"),
        target.get("Project"),
    )
    if target_contract != ("ReEchoEditor", "Win64", "Development", "../../ReEcho.uproject"):
        raise PrebuiltError(f"{relative(target_path, root)} does not describe the Development Win64 project Editor")
    if target_build_id != build_id:
        raise PrebuiltError(
            f"{relative(target_path, root)} BuildId {target_build_id!r} does not match UnrealEditor.modules {build_id!r}"
        )

    binary_names = {"ReEchoEditor.target", "UnrealEditor.modules", *built_modules.values()}
    missing_binaries = sorted(name for name in binary_names if not (binary_dir / name).is_file())
    if missing_binaries:
        raise PrebuiltError(f"missing built module files: {', '.join(missing_binaries)}")

    hashes = source_hashes(root)
    return {
        "schema_version": SCHEMA_VERSION,
        "target": "ReEchoEditor",
        "platform": "Win64",
        "configuration": "Development",
        "engine_association": association,
        "engine_build_id": build_id,
        "source_fingerprint": combined_fingerprint(hashes),
        "source_files": hashes,
        "modules": built_modules,
        "binaries": {name: sha256_file(binary_dir / name) for name in sorted(binary_names)},
    }


def update(root: Path = ROOT) -> dict:
    binary_dir = root / "Binaries" / "Win64"
    for contract_path in (binary_dir / "UnrealEditor.modules", binary_dir / "ReEchoEditor.target"):
        document = load_json(contract_path)
        temporary = contract_path.with_suffix(contract_path.suffix + ".tmp")
        temporary.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(temporary, contract_path)

    manifest = create_manifest(root)
    manifest_path = root / "Binaries" / "Win64" / "ReEchoEditor.prebuilt.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = manifest_path.with_suffix(manifest_path.suffix + ".tmp")
    temporary.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, manifest_path)
    return manifest


def check(root: Path = ROOT) -> dict:
    manifest_path = root / "Binaries" / "Win64" / "ReEchoEditor.prebuilt.json"
    recorded = load_json(manifest_path)
    expected = create_manifest(root)

    if recorded.get("schema_version") != SCHEMA_VERSION:
        raise PrebuiltError(f"unsupported prebuilt schema version: {recorded.get('schema_version')!r}")
    if recorded != expected:
        recorded_sources = recorded.get("source_files", {})
        expected_sources = expected["source_files"]
        changed_sources = sorted(
            path
            for path in set(recorded_sources) | set(expected_sources)
            if recorded_sources.get(path) != expected_sources.get(path)
        )
        recorded_binaries = recorded.get("binaries", {})
        expected_binaries = expected["binaries"]
        changed_binaries = sorted(
            path
            for path in set(recorded_binaries) | set(expected_binaries)
            if recorded_binaries.get(path) != expected_binaries.get(path)
        )
        details = []
        if changed_sources:
            details.append("stale source fingerprint: " + ", ".join(changed_sources))
        if changed_binaries:
            details.append("binary hash mismatch: " + ", ".join(changed_binaries))
        if not details:
            details.append("engine, target, configuration, or module contract changed")
        raise PrebuiltError("prebuilt Editor bundle is stale: " + "; ".join(details))
    return recorded


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "update"))
    parser.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    try:
        manifest = update(root) if args.action == "update" else check(root)
    except PrebuiltError as exc:
        print(f"[FAIL] {exc}")
        return 1
    verb = "refreshed" if args.action == "update" else "verified"
    print(
        f"[PASS] prebuilt Editor bundle {verb}: "
        f"modules={len(manifest['modules'])} build_id={manifest['engine_build_id']} "
        f"source={manifest['source_fingerprint'][:12]}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
