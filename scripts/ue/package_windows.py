#!/usr/bin/env python3
"""Build, cook, package, and smoke-test the ReEcho Windows Shipping build."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path
import platform
import subprocess
import sys
import time

PROJECT_ROOT = Path(__file__).resolve().parents[2]
PROJECT_FILE = PROJECT_ROOT / "ReEcho.uproject"
ENGINE_FINDER = Path(__file__).with_name("Find-UnrealEngine.ps1")
DEFAULT_OUTPUT = PROJECT_ROOT / "Packages" / "Windows"
DEFAULT_MAP = "/Game/Level00"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a clean Win64 Shipping package for ReEcho.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--engine-root", type=Path, help="UE 5.8 installation root")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="archive output directory")
    parser.add_argument("--map", default=DEFAULT_MAP, help="Unreal map package to cook")
    parser.add_argument("--smoke-seconds", type=float, default=10.0, help="packaged EXE smoke-test duration")
    parser.add_argument("--no-smoke", action="store_true", help="skip launching the packaged EXE")
    parser.add_argument("--no-clean", action="store_true", help="reuse existing cooked data")
    parser.add_argument("--dry-run", action="store_true", help="print the UAT command without closing UE or building")
    return parser.parse_args()


def run_checked(command: list[str], cwd: Path = PROJECT_ROOT) -> None:
    printable = subprocess.list2cmdline(command)
    print(f"[ReEchoPackage] Running: {printable}", flush=True)
    completed = subprocess.run(command, cwd=cwd, check=False)
    if completed.returncode != 0:
        raise RuntimeError(f"Command failed with exit code {completed.returncode}: {printable}")


def close_unreal_editor() -> None:
    """Force-close editor processes so DLLs and MCP port 8000 are not held during Cook."""
    for image_name in ("UnrealEditor.exe", "UnrealEditor-Cmd.exe"):
        subprocess.run(
            ["taskkill", "/F", "/T", "/IM", image_name],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    print("[ReEchoPackage] Unreal Editor processes closed.", flush=True)


def find_engine_root(explicit_root: Path | None) -> Path:
    command = [
        "powershell.exe",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(ENGINE_FINDER),
    ]
    if explicit_root:
        command.extend(["-EngineRoot", str(explicit_root.resolve())])

    completed = subprocess.run(
        command,
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"Unable to locate Unreal Engine: {detail}")

    candidates = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
    if not candidates:
        raise RuntimeError("Find-UnrealEngine.ps1 returned no engine path.")
    engine_root = Path(candidates[-1]).resolve()
    uat = engine_root / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat"
    if not uat.is_file():
        raise RuntimeError(f"RunUAT.bat was not found under {engine_root}")
    return engine_root


def make_uat_command(engine_root: Path, output: Path, map_name: str, clean: bool) -> list[str]:
    uat = engine_root / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat"
    command = [
        str(uat),
        "BuildCookRun",
        f"-project={PROJECT_FILE}",
        "-noP4",
        "-platform=Win64",
        "-clientconfig=Shipping",
        "-build",
        "-cook",
    ]
    if clean:
        command.append("-clean")
    command.extend(
        [
            f"-map={map_name}",
            "-stage",
            "-package",
            "-pak",
            "-prereqs",
            "-archive",
            f"-archivedirectory={output}",
            "-utf8output",
        ]
    )
    return command


def find_packaged_exe(output: Path) -> Path:
    preferred = output / "ReEcho.exe"
    if preferred.is_file():
        return preferred
    matches = sorted(output.rglob("ReEcho.exe")) if output.is_dir() else []
    if not matches:
        raise RuntimeError(f"Packaging succeeded but ReEcho.exe was not found under {output}")
    return matches[0]


def stage_runtime_csvs(output: Path) -> int:
    """Copy every runtime CSV from the project's loose Content/Data into the
    packaged game's Content/Data.

    The runtime registry reads CSVs from disk via FFileHelper, but the cooker
    only stages files that are referenced by cooked assets. Pure data CSVs in
    Content/Data are not asset-referenced, so a subset (the 6 stage/encounter/
    spawn/attribute tables) is silently dropped from Shipping packages,
    causing a LowLevelFatalError at startup. Copying them explicitly after the
    archive guarantees all manifest CSVs ship as loose files — the same
    mechanism the 22 already-staged CSVs rely on.
    """
    source_data = PROJECT_ROOT / "Content" / "Data"
    if not source_data.is_dir():
        print("[ReEchoPackage] Skipping CSV staging: Content/Data not found.", flush=True)
        return 0
    csv_files = sorted(source_data.glob("*.csv"))
    if not csv_files:
        print("[ReEchoPackage] Skipping CSV staging: no CSV files in Content/Data.", flush=True)
        return 0

    content_dirs = [
        d
        for d in output.rglob("Content")
        if d.is_dir() and "Engine" not in d.parts
    ]
    if not content_dirs:
        print(
            f"[ReEchoPackage] WARNING: no (non-Engine) Content directory under {output}; "
            "runtime CSVs were NOT staged into the package.",
            flush=True,
        )
        return 0

    copied = 0
    for content_dir in content_dirs:
        dest = content_dir / "Data"
        dest.mkdir(parents=True, exist_ok=True)
        for csv in csv_files:
            shutil.copy2(csv, dest / csv.name)
            copied += 1
    print(
        f"[ReEchoPackage] Staged {len(csv_files)} runtime CSV(s) into "
        f"{len(content_dirs)} Content/Data dir(s) ({copied} files copied).",
        flush=True,
    )
    return copied


def smoke_test(exe: Path, duration: float) -> None:
    if duration <= 0.0:
        print("[ReEchoPackage] Smoke test skipped because duration is zero.", flush=True)
        return

    startup_info = subprocess.STARTUPINFO()
    startup_info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup_info.wShowWindow = subprocess.SW_HIDE
    process = subprocess.Popen([str(exe)], cwd=exe.parent, startupinfo=startup_info)
    try:
        time.sleep(duration)
        exit_code = process.poll()
        if exit_code is not None:
            raise RuntimeError(f"Packaged game exited during smoke test with code {exit_code}")
        print(f"[ReEchoPackage] Smoke test passed: process stayed alive for {duration:g}s.", flush=True)
    finally:
        if process.poll() is None:
            subprocess.run(
                ["taskkill", "/F", "/T", "/PID", str(process.pid)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )


def main() -> int:
    args = parse_args()
    if platform.system() != "Windows":
        raise RuntimeError("This packaging script supports Windows only.")
    if not PROJECT_FILE.is_file():
        raise RuntimeError(f"Project file not found: {PROJECT_FILE}")

    output = args.output if args.output.is_absolute() else PROJECT_ROOT / args.output
    output = output.resolve()
    engine_root = find_engine_root(args.engine_root)
    command = make_uat_command(engine_root, output, args.map, clean=not args.no_clean)

    print(f"[ReEchoPackage] Engine: {engine_root}")
    print(f"[ReEchoPackage] Project: {PROJECT_FILE}")
    print(f"[ReEchoPackage] Output: {output}")
    if args.dry_run:
        print(f"[ReEchoPackage] Dry run: {subprocess.list2cmdline(command)}")
        return 0

    close_unreal_editor()
    output.mkdir(parents=True, exist_ok=True)
    run_checked(command)

    packaged_exe = find_packaged_exe(output)
    print(f"[ReEchoPackage] Package created: {packaged_exe}")
    stage_runtime_csvs(output)
    if not args.no_smoke:
        smoke_test(packaged_exe, args.smoke_seconds)
    print("[ReEchoPackage] BUILD SUCCESSFUL", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("[ReEchoPackage] Cancelled by user.", file=sys.stderr)
        raise SystemExit(130)
    except Exception as error:
        print(f"[ReEchoPackage] ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)