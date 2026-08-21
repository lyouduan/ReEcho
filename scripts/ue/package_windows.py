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
    parser.add_argument("--no-timestamp", action="store_true", help="do not append MMDDHHmm suffix to output folder")
    parser.add_argument("--timestamp-suffix", default="", help="override the generated MMDDHHmm output-folder suffix")
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


def git_one_line(args: list[str]) -> str:
    """Return the first line of a git command's output, or '' on failure."""
    completed = subprocess.run(
        ["git", *args],
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        return ""
    return completed.stdout.strip().splitlines()[0] if completed.stdout.strip() else ""


def git_lines(args: list[str]) -> list[str]:
    """Return all output lines of a git command, or [] on failure."""
    completed = subprocess.run(
        ["git", *args],
        cwd=PROJECT_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        return []
    return [line for line in completed.stdout.splitlines() if line.strip()]


def parse_commit_from_version_note(note_path: Path) -> str:
    """Extract the '提交哈希 / Commit' short hash from an existing VERSION.txt."""
    if not note_path.is_file():
        return ""
    for line in note_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "提交哈希 / Commit" in line and ":" in line:
            value = line.split(":", 1)[1].strip()
            return value.split()[0] if value else ""
    return ""


def find_previous_package_commit(current_output: Path) -> tuple[str, Path | None]:
    """Locate the most recent other desktop package and read its commit hash.

    Desktop folders are named ReEchoPackage_<MMDDHHmm>. We exclude the folder
    being built (current_output) and pick the newest remaining one, then parse
    its VERSION.txt for the commit hash that was current at that packaging time.
    Returns ('', None) when no usable previous package exists.
    """
    current_resolved = current_output.resolve()
    candidates: list[Path] = []
    for parent in (current_output.parents[1:],):
        # Walk up to the grandparent level; desktop is usually two levels above
        # output's own dir only via the timestamp suffix, so scan siblings too.
        break
    # Scan the parent of current_output's parent (i.e. the desktop root) for
    # ReEchoPackage_* siblings. current_output is <desktop>/ReEchoPackage_MMDDHHmm.
    desktop = current_output.parent
    if desktop.name.startswith("ReEchoPackage_"):
        desktop = desktop.parent
    for folder in sorted(desktop.glob("ReEchoPackage_*")):
        if folder.resolve() == current_resolved:
            continue
        note = folder / "VERSION.txt"
        if note.is_file():
            commit = parse_commit_from_version_note(note)
            if commit:
                candidates.append((folder, commit))
    if not candidates:
        return "", None
    # Newest by folder name (the MMDDHHmm suffix is lexicographically sortable).
    candidates.sort(key=lambda item: item[0].name, reverse=True)
    return candidates[0][1], candidates[0][0]


def collect_diff_since(prev_commit: str, head_commit: str) -> tuple[list[str], list[str]]:
    """Return (commit_oneline_list, diff_stat_lines) between prev and HEAD."""
    range_spec = f"{prev_commit}..{head_commit}"
    commits = git_lines(["log", "--reverse", "--pretty=%h %s", range_spec])
    stat = git_lines(["diff", "--stat", range_spec])
    return commits, stat


def count_vfx_in_package(output: Path) -> int | None:
    """Count VFX assets cooked into the package's main IoStore container.

    UE 5.8 defaults to IoStore, so game assets live in <output>/.../Paks/
    ReEcho-Windows.utoc (not .pak). The cooker mounts the project under the
    '../../../ReEcho/' prefix, so VFX paths match 'ReEcho/Content/VFX'. Returns
    None when the container or UnrealPak is unavailable (best-effort only).
    """
    pak_candidates = sorted(output.rglob("UnrealPak.exe"))
    # The cooker does not ship UnrealPak, so fall back to the engine binary used
    # by make_uat_command (Engine/Binaries/Win64/UnrealPak.exe).
    if not pak_candidates:
        engine_root = find_engine_root(None)
        pak_candidates = [engine_root / "Engine" / "Binaries" / "Win64" / "UnrealPak.exe"]
    utoc_candidates = [p for p in sorted(output.rglob("*.utoc")) if "global.utoc" not in p.name]
    if not pak_candidates or not utoc_candidates:
        return None
    pak_exe = pak_candidates[0]
    if not pak_exe.is_file():
        return None
    completed = subprocess.run(
        [str(pak_exe), str(utoc_candidates[0]), "-List"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    if completed.returncode != 0:
        return None
    return sum(1 for line in completed.stdout.splitlines() if "ReEcho/Content/VFX" in line)


def write_version_note(
    output: Path,
    csv_count: int,
    prev_commit: str = "",
    prev_folder: Path | None = None,
    diff_commits: list[str] | None = None,
    diff_stat: list[str] | None = None,
) -> Path:
    """Write a VERSION.txt into the packaged game root describing this build.

    Includes the git commit hash/title, packaging time, base branch, engine, the
    staged CSV / VFX counts, and — when a previous desktop package is known — a
    'delta since last package' section with raw commit/stat data plus a marker
    that an AI step can fill in as human-readable functional notes.
    """
    stamp = time.strftime("%Y-%m-%d %H:%M:%S")
    commit_hash = git_one_line(["rev-parse", "--short", "HEAD"]) or "(unknown)"
    commit_title = git_one_line(["log", "-1", "--pretty=%s"]) or "(unknown)"
    branch = git_one_line(["rev-parse", "--abbrev-ref", "HEAD"]) or "(unknown)"
    vfx_count = count_vfx_in_package(output)

    lines = [
        "ReEcho Windows Shipping Package",
        "=" * 40,
        f"打包时间 / Packaged at : {stamp}",
        f"基线分支 / Branch      : {branch}",
        f"提交哈希 / Commit      : {commit_hash}",
        f"提交标题 / Title       : {commit_title}",
        f"引擎     / Engine      : UE 5.8",
        "",
        "内容校验 / Contents",
        "-" * 40,
        f"运行时 CSV 入包数量   : {csv_count}",
    ]
    if vfx_count is not None:
        lines.append(f"VFX 资产入包数量        : {vfx_count}")
    else:
        lines.append("VFX 资产入包数量        : (未统计)")
    lines.append("")
    lines.append("启动地图 / Start map        : /Game/Level00")
    lines.append("可执行文件 / Executable     : ReEcho.exe (或其上层目录中的同名文件)")

    # --- Delta since previous desktop package (if any) -----------------------
    if prev_commit and diff_commits is not None and diff_stat is not None:
        prev_short = prev_commit.split()[0]
        lines.append("")
        lines.append("=" * 40)
        lines.append("相较上一次打包 / Delta since last package")
        lines.append("-" * 40)
        if prev_folder:
            lines.append(f"上次包 / Previous : {prev_folder.name} ({prev_short})")
        lines.append(f"本次包 / Current  : {output.name} ({commit_hash})")
        lines.append("")
        if diff_commits:
            lines.append(f"中间新增提交 ({len(diff_commits)} 条):")
            for entry in diff_commits:
                lines.append(f"  - {entry}")
        else:
            lines.append("中间无新增提交(与上次打包为同一 commit)。")
        lines.append("")
        if diff_stat:
            lines.append("文件级变更 (git diff --stat):")
            for entry in diff_stat:
                lines.append(f"  {entry}")
        else:
            lines.append("无文件级变更。")
        lines.append("")
        lines.append("[AI_SUMMARY]")
        lines.append("（此处由 AI 根据上述 commit 归纳：功能变化 / Bug 修复 / 其他重点。）")

    note_path = output / "VERSION.txt"
    note_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[ReEchoPackage] Version note written: {note_path}", flush=True)
    return note_path


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
    # Append a MMDDHHmm timestamp suffix so every package folder is uniquely
    # named by packaging time (e.g. ReEchoPackage_08211107): first 4 digits are
    # the date (MMDD), last 4 are the time (HHmm). An explicit --timestamp-suffix
    # overrides the generated one; pass --no-timestamp to disable.
    if not args.no_timestamp:
        stamp = args.timestamp_suffix or time.strftime("%m%d%H%M")
        output = output.parent / f"{output.name}_{stamp}"
    output = output.resolve()
    print(f"[ReEchoPackage] Output: {output}")
    engine_root = find_engine_root(args.engine_root)
    command = make_uat_command(engine_root, output, args.map, clean=not args.no_clean)

    print(f"[ReEchoPackage] Engine: {engine_root}")
    print(f"[ReEchoPackage] Project: {PROJECT_FILE}")
    if args.dry_run:
        print(f"[ReEchoPackage] Dry run: {subprocess.list2cmdline(command)}")
        return 0

    close_unreal_editor()
    # Capture the previous desktop package BEFORE building, so we can diff against
    # it. Must run before output.mkdir creates this run's folder.
    head_commit = git_one_line(["rev-parse", "--short", "HEAD"])
    prev_commit, prev_folder = find_previous_package_commit(output)
    diff_commits: list[str] = []
    diff_stat: list[str] = []
    if prev_commit and prev_commit != head_commit:
        diff_commits, diff_stat = collect_diff_since(prev_commit, head_commit)
        print(
            f"[ReEchoPackage] Delta vs previous package ({prev_commit}): "
            f"{len(diff_commits)} commit(s).",
            flush=True,
        )
    elif prev_commit:
        print("[ReEchoPackage] Same commit as previous package; no delta.", flush=True)
    else:
        print("[ReEchoPackage] No previous desktop package found; delta section omitted.", flush=True)
    output.mkdir(parents=True, exist_ok=True)
    run_checked(command)

    packaged_exe = find_packaged_exe(output)
    print(f"[ReEchoPackage] Package created: {packaged_exe}")
    csv_count = stage_runtime_csvs(output)
    write_version_note(output, csv_count, prev_commit, prev_folder, diff_commits, diff_stat)
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