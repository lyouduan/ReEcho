#!/usr/bin/env python3
"""Create a traceable ReEcho Windows package with one command."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from datetime import datetime
import json
import locale
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
import traceback
import uuid


SCRIPT_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PROJECT = SCRIPT_ROOT / "ReEcho.uproject"
ENGINE_FINDER = Path(__file__).with_name("Find-UnrealEngine.ps1")
DEFAULT_MAP = "/Game/Level00"
DEFAULT_SMOKE_SECONDS = 12.0
REMOTE_WORKTREE_ROOT = Path(tempfile.gettempdir()) / "ReEchoPackageWorktrees"


class PackageError(RuntimeError):
    pass


@dataclass
class SourceContext:
    root: Path
    project: Path
    revision: str
    branch: str
    source_label: str
    cleanup_nonce: str | None = None


@dataclass
class SmokeResult:
    process_alive: bool
    copied_logs: list[Path]
    session_logs: list[Path]
    warnings: list[str]


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Create a clean Win64 Development test package by default. "
            "The output, build log, smoke evidence, and source manifest are written to the Desktop."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--formal", action="store_true", help="build a Shipping formal package instead")
    parser.add_argument(
        "--remote-main",
        action="store_true",
        help="fetch and package exact origin/main from an isolated temporary worktree",
    )
    parser.add_argument("--engine-root", type=Path, help="UE 5.8 installation root")
    parser.add_argument("--output", type=Path, help="exact archive output directory; it must be empty")
    parser.add_argument("--map", default=DEFAULT_MAP, help="Unreal map package to cook")
    parser.add_argument(
        "--smoke-seconds",
        type=float,
        default=DEFAULT_SMOKE_SECONDS,
        help="packaged EXE smoke-test duration",
    )
    parser.add_argument("--no-smoke", action="store_true", help="skip launching the packaged EXE")
    parser.add_argument("--dry-run", action="store_true", help="prepare source and print the UAT command only")
    return parser.parse_args(argv)


def command_text(command: list[str]) -> str:
    return subprocess.list2cmdline([str(item) for item in command])


def configure_console_streams(*streams: object) -> None:
    for stream in streams:
        reconfigure = getattr(stream, "reconfigure", None)
        if callable(reconfigure):
            reconfigure(errors="replace")


def decode_process_output(raw: bytes, fallback_encoding: str | None = None) -> str:
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        encoding = fallback_encoding or locale.getpreferredencoding(False) or "utf-8"
        return raw.decode(encoding, errors="replace")


def run_capture(command: list[str], cwd: Path) -> str:
    completed = subprocess.run(
        [str(item) for item in command],
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    output = completed.stdout or ""
    if completed.returncode != 0:
        raise PackageError(
            f"Command failed with exit code {completed.returncode}: {command_text(command)}\n{output.strip()}"
        )
    return output.strip()


def run_streamed(command: list[str], cwd: Path, log_path: Path, heading: str) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    printable = command_text(command)
    print(f"[ReEchoPackage] {heading}: {printable}", flush=True)
    with log_path.open("a", encoding="utf-8", newline="\n") as log:
        log.write(f"\n===== {heading} =====\n{printable}\n\n")
        process = subprocess.Popen(
            [str(item) for item in command],
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        assert process.stdout is not None
        for raw_line in process.stdout:
            line = decode_process_output(raw_line)
            print(line, end="", flush=True)
            log.write(line)
            log.flush()
        return_code = process.wait()
        log.write(f"\n[exit code] {return_code}\n")
    if return_code != 0:
        raise PackageError(f"{heading} failed with exit code {return_code}; see {log_path}")


def git_output(root: Path, *args: str) -> str:
    return run_capture(["git", *args], root)


def prepare_source(remote_main: bool) -> SourceContext:
    if not remote_main:
        if not SCRIPT_PROJECT.is_file():
            raise PackageError(f"Project file not found: {SCRIPT_PROJECT}")
        revision = git_output(SCRIPT_ROOT, "rev-parse", "HEAD")
        branch = git_output(SCRIPT_ROOT, "branch", "--show-current") or "detached"
        return SourceContext(SCRIPT_ROOT, SCRIPT_PROJECT, revision, branch, "current")

    print("[ReEchoPackage] Fetching latest origin/main...", flush=True)
    git_output(SCRIPT_ROOT, "fetch", "origin", "main")
    revision = git_output(SCRIPT_ROOT, "rev-parse", "origin/main")
    REMOTE_WORKTREE_ROOT.mkdir(parents=True, exist_ok=True)
    nonce = uuid.uuid4().hex
    worktree = REMOTE_WORKTREE_ROOT / f"{revision[:8]}-{nonce[:8]}"
    git_output(SCRIPT_ROOT, "worktree", "add", "--detach", str(worktree), revision)
    marker = worktree / ".reecho-package-worktree"
    marker.write_text(nonce, encoding="ascii")
    project = worktree / "ReEcho.uproject"
    if not project.is_file():
        raise PackageError(f"Remote-main worktree does not contain ReEcho.uproject: {worktree}")
    return SourceContext(worktree, project, revision, "origin/main", "origin-main", nonce)


def path_is_within(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def cleanup_source(source: SourceContext | None) -> str | None:
    if not source or not source.cleanup_nonce:
        return None
    marker = source.root / ".reecho-package-worktree"
    try:
        if not path_is_within(source.root, REMOTE_WORKTREE_ROOT):
            raise PackageError(f"Refusing to clean worktree outside guarded root: {source.root}")
        if marker.read_text(encoding="ascii").strip() != source.cleanup_nonce:
            raise PackageError(f"Refusing to clean worktree with invalid marker: {source.root}")
        actual_revision = git_output(source.root, "rev-parse", "HEAD")
        if actual_revision != source.revision:
            raise PackageError(
                f"Refusing to clean worktree because HEAD changed: {actual_revision} != {source.revision}"
            )
        git_output(SCRIPT_ROOT, "worktree", "remove", "--force", str(source.root))
        git_output(SCRIPT_ROOT, "worktree", "prune")
        return None
    except Exception as error:
        return str(error)


def finish_cleanup(source: SourceContext | None, run_evidence: Path, output: Path | None) -> None:
    cleanup_note = cleanup_source(source) or ""
    if cleanup_note and output:
        evidence = output / "PackagingEvidence"
        evidence.mkdir(parents=True, exist_ok=True)
        (evidence / "CleanupWarning.txt").write_text(
            "Temporary worktree cleanup needs manual attention:\n" + cleanup_note + "\n",
            encoding="utf-8",
        )
    shutil.rmtree(run_evidence, ignore_errors=True)


def configuration_for(args: argparse.Namespace) -> str:
    return "Shipping" if args.formal else "Development"


def desktop_directory() -> Path:
    configured = os.environ.get("REECHO_PACKAGE_DESKTOP")
    if configured:
        return Path(configured).expanduser().resolve()
    return (Path.home() / "Desktop").resolve()


def default_output_path(source: SourceContext, configuration: str, timestamp: str) -> Path:
    package_kind = "Formal" if configuration == "Shipping" else "Test"
    name = (
        f"ReEcho_{source.source_label}_{source.revision[:8]}_"
        f"Win64_{configuration}_{package_kind}_{timestamp}"
    )
    return desktop_directory() / name


def prepare_output(output: Path) -> Path:
    output = output.expanduser().resolve()
    if output.exists() and any(output.iterdir()):
        raise PackageError(f"Output directory is not empty; choose a new directory: {output}")
    output.mkdir(parents=True, exist_ok=True)
    return output


def emergency_output(timestamp: str) -> Path:
    output = desktop_directory() / f"ReEcho_Package_FAILED_{timestamp}"
    suffix = 1
    while output.exists():
        output = desktop_directory() / f"ReEcho_Package_FAILED_{timestamp}_{suffix}"
        suffix += 1
    output.mkdir(parents=True, exist_ok=False)
    return output


def find_engine_root(explicit_root: Path | None, cwd: Path) -> Path:
    command = [
        "powershell.exe",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(ENGINE_FINDER),
    ]
    if explicit_root:
        command.extend(["-EngineRoot", str(explicit_root.expanduser().resolve())])
    output = run_capture(command, cwd)
    candidates = [line.strip() for line in output.splitlines() if line.strip()]
    if not candidates:
        raise PackageError("Find-UnrealEngine.ps1 returned no engine path")
    engine_root = Path(candidates[-1]).resolve()
    uat = engine_root / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat"
    if not uat.is_file():
        raise PackageError(f"RunUAT.bat was not found under {engine_root}")
    return engine_root


def make_uat_command(
    engine_root: Path,
    project_file: Path,
    output: Path,
    map_name: str,
    configuration: str,
) -> list[str]:
    uat = engine_root / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat"
    return [
        str(uat),
        "BuildCookRun",
        f"-project={project_file}",
        "-noP4",
        "-platform=Win64",
        f"-clientconfig={configuration}",
        "-clean",
        "-build",
        "-cook",
        f"-map={map_name}",
        "-stage",
        "-package",
        "-pak",
        "-prereqs",
        "-archive",
        f"-archivedirectory={output}",
        "-utf8output",
    ]


def unreal_editor_processes() -> list[dict[str, object]]:
    script = (
        "$p = Get-CimInstance Win32_Process | "
        "Where-Object { $_.Name -in @('UnrealEditor.exe','UnrealEditor-Cmd.exe') } | "
        "Select-Object ProcessId,Name,ExecutablePath,CommandLine; "
        "if ($p) { $p | ConvertTo-Json -Compress }"
    )
    output = run_capture(["powershell.exe", "-NoProfile", "-Command", script], SCRIPT_ROOT)
    if not output:
        return []
    parsed = json.loads(output)
    return parsed if isinstance(parsed, list) else [parsed]


def require_editor_closed() -> None:
    processes = unreal_editor_processes()
    if not processes:
        return
    details = []
    for process in processes:
        details.append(
            f"PID={process.get('ProcessId')} Name={process.get('Name')} "
            f"Project={process.get('CommandLine') or '<command line unavailable>'}"
        )
    raise PackageError(
        "Unreal Editor is still running. Save all assets and close UE, then run this script again. "
        "The script will not force-close it.\n" + "\n".join(details)
    )


def source_status(source: SourceContext) -> list[str]:
    output = git_output(
        source.root,
        "status",
        "--porcelain=v1",
        "--untracked-files=all",
        "--",
        "Content",
        "Config",
    )
    return [line for line in output.splitlines() if line.strip()]


def write_source_manifest(
    path: Path,
    source: SourceContext | None,
    configuration: str,
    map_name: str,
    status_lines: list[str],
    result: str,
    engine_root: Path | None,
    launcher: Path | None,
    note: str = "",
) -> None:
    lines = [
        "ReEcho Windows Package Source",
        f"Result={result}",
        f"GeneratedAt={datetime.now().astimezone().isoformat(timespec='seconds')}",
        f"Project={source.project if source else '<unavailable>'}",
        f"Branch={source.branch if source else '<unavailable>'}",
        f"GitSHA={source.revision if source else '<unavailable>'}",
        f"SourceMode={source.source_label if source else '<unavailable>'}",
        f"Configuration={configuration}",
        "Pipeline=Clean Build Cook Stage Package Pak Prereqs Archive",
        f"Map={map_name}",
        f"EngineRoot={engine_root or '<unavailable>'}",
        f"Launcher={launcher or '<unavailable>'}",
        f"ContentConfigDiffCount={len(status_lines)}",
        "ContentConfigDiffs:",
    ]
    lines.extend(status_lines or ["<clean>"])
    if note:
        lines.extend(["Notes:", note])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def find_packaged_exe(output: Path) -> Path:
    preferred = [output / "Windows" / "ReEcho.exe", output / "ReEcho.exe"]
    for candidate in preferred:
        if candidate.is_file():
            return candidate
    matches = sorted(
        (path for path in output.rglob("ReEcho.exe") if path.is_file()),
        key=lambda path: (len(path.parts), str(path).lower()),
    )
    if not matches:
        raise PackageError(f"Packaging completed but ReEcho.exe was not found under {output}")
    return matches[0]


def runtime_csv_names(source_root: Path) -> list[str]:
    manifest = source_root / "Content" / "Data" / "reecho_data_manifest.csv"
    if not manifest.is_file():
        raise PackageError(f"Runtime CSV manifest not found: {manifest}")
    with manifest.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = csv.DictReader(handle)
        if "FileName" not in (rows.fieldnames or []):
            raise PackageError(f"Runtime CSV manifest has no FileName column: {manifest}")
        names = [str(row.get("FileName", "")).strip() for row in rows]
    names = [name for name in names if name]
    names.append(manifest.name)
    return sorted(set(names), key=str.lower)


def packaged_content_directories(output: Path) -> list[Path]:
    return sorted(
        path
        for path in output.rglob("Content")
        if path.is_dir()
        and path.parent.name.lower() == "reecho"
        and all(part.lower() != "engine" for part in path.parts)
    )


def stage_and_verify_runtime_csvs(source_root: Path, output: Path) -> tuple[int, int]:
    source_data = source_root / "Content" / "Data"
    expected = runtime_csv_names(source_root)
    source_files = sorted(source_data.glob("*.csv"), key=lambda path: path.name.lower())
    if not source_files:
        raise PackageError(f"No runtime CSV files were found under {source_data}")
    content_dirs = packaged_content_directories(output)
    if not content_dirs:
        raise PackageError(f"No packaged game Content directory was found under {output}")
    copied = 0
    for content_dir in content_dirs:
        destination = content_dir / "Data"
        destination.mkdir(parents=True, exist_ok=True)
        for source_file in source_files:
            shutil.copy2(source_file, destination / source_file.name)
            copied += 1
    missing_sources = [name for name in expected if not (source_data / name).is_file()]
    if missing_sources:
        raise PackageError(
            "Runtime CSV files listed by manifest are missing from source:\n"
            + "\n".join(missing_sources)
        )
    missing = [
        str(content_dir / "Data" / name)
        for content_dir in content_dirs
        for name in expected
        if not (content_dir / "Data" / name).is_file()
    ]
    if missing:
        raise PackageError("Packaged runtime CSV verification failed:\n" + "\n".join(missing))
    print(
        f"[ReEchoPackage] Runtime CSV verification passed: {len(expected)} file(s) in "
        f"{len(content_dirs)} package Content directory/directories.",
        flush=True,
    )
    return copied, len(expected)


def candidate_log_roots(exe: Path, output: Path) -> list[tuple[str, Path]]:
    roots: list[tuple[str, Path]] = [
        ("package", exe.parent / "ReEcho" / "Saved" / "Logs"),
        ("package", exe.parent / "Saved" / "Logs"),
    ]
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        roots.append(("local", Path(local_app_data) / "ReEcho" / "Saved" / "Logs"))
    unique: list[tuple[str, Path]] = []
    seen: set[str] = set()
    for label, root in roots:
        key = str(root.resolve()).lower()
        if key not in seen:
            seen.add(key)
            unique.append((label, root))
    return unique


def log_snapshot(roots: list[tuple[str, Path]]) -> dict[str, tuple[int, int]]:
    snapshot: dict[str, tuple[int, int]] = {}
    for _, root in roots:
        if not root.is_dir():
            continue
        for path in root.glob("*.log"):
            try:
                stat = path.stat()
                snapshot[str(path.resolve()).lower()] = (stat.st_mtime_ns, stat.st_size)
            except OSError:
                continue
    return snapshot


def changed_logs(
    roots: list[tuple[str, Path]],
    before: dict[str, tuple[int, int]],
) -> list[tuple[str, Path]]:
    changed: list[tuple[str, Path]] = []
    for label, root in roots:
        if not root.is_dir():
            continue
        for path in root.glob("*.log"):
            try:
                stat = path.stat()
            except OSError:
                continue
            key = str(path.resolve()).lower()
            if before.get(key) != (stat.st_mtime_ns, stat.st_size):
                changed.append((label, path))
    return changed


def processes_under(root: Path) -> list[dict[str, object]]:
    script = (
        "$p = Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath } | "
        "Select-Object ProcessId,Name,ExecutablePath; if ($p) { $p | ConvertTo-Json -Compress }"
    )
    output = run_capture(["powershell.exe", "-NoProfile", "-Command", script], SCRIPT_ROOT)
    if not output:
        return []
    parsed = json.loads(output)
    processes = parsed if isinstance(parsed, list) else [parsed]
    return [
        process
        for process in processes
        if process.get("ExecutablePath")
        and path_is_within(Path(str(process["ExecutablePath"])), root)
    ]


def stop_processes(process_ids: set[int]) -> None:
    if not process_ids:
        return
    ids = ",".join(str(process_id) for process_id in sorted(process_ids))
    subprocess.run(
        [
            "powershell.exe",
            "-NoProfile",
            "-Command",
            f"Stop-Process -Id {ids} -Force -ErrorAction SilentlyContinue",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def analyze_smoke_logs(log_text: str) -> tuple[list[str], list[str]]:
    failures: list[str] = []
    warnings: list[str] = []
    if "Engine is initialized" not in log_text:
        failures.append("session log does not contain 'Engine is initialized'")
    if not re.search(r"\[RuntimeAssetPreload\].*Success=true", log_text, re.IGNORECASE):
        failures.append("session log does not report RuntimeAssetPreload Success=true")
    if re.search(r"Fatal error:", log_text, re.IGNORECASE):
        failures.append("session log contains a fatal error")
    missing_csv_patterns = (
        r"\.csv.*File could not be read",
        r"File could not be read.*\.csv",
        r"missing.*\.csv",
    )
    if any(re.search(pattern, log_text, re.IGNORECASE) for pattern in missing_csv_patterns):
        failures.append("session log reports a missing or unreadable CSV")
    if re.search(r"Ensure condition failed", log_text, re.IGNORECASE):
        warnings.append("session log contains a handled Ensure; inspect SmokeTest.log")
    return failures, warnings


def clean_package_smoke_state(output: Path) -> None:
    saved_dirs = sorted(
        {path.parent for path in output.rglob("Logs") if path.parent.name.lower() == "saved"},
        key=lambda path: len(path.parts),
        reverse=True,
    )
    for saved in saved_dirs:
        if path_is_within(saved, output) and saved.name.lower() == "saved":
            shutil.rmtree(saved, ignore_errors=True)


def smoke_test(
    exe: Path,
    output: Path,
    configuration: str,
    duration: float,
    evidence_dir: Path,
) -> SmokeResult:
    if duration <= 0:
        raise PackageError("Smoke-test duration must be greater than zero")
    smoke_log = evidence_dir / "SmokeTest.log"
    roots = candidate_log_roots(exe, output)
    before = log_snapshot(roots)
    launch_command = [str(exe), "-windowed", "-ResX=1280", "-ResY=720"]
    if configuration == "Development":
        launch_command.append("-log")
    startup_info = subprocess.STARTUPINFO()
    startup_info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup_info.wShowWindow = subprocess.SW_HIDE
    started = datetime.now().astimezone().isoformat(timespec="seconds")
    process = subprocess.Popen(launch_command, cwd=exe.parent, startupinfo=startup_info)
    alive_processes: list[dict[str, object]] = []
    process_alive = False
    try:
        time.sleep(duration)
        alive_processes = processes_under(output)
        process_alive = process.poll() is None or bool(alive_processes)
    finally:
        process_ids = {
            int(item["ProcessId"])
            for item in alive_processes
            if item.get("ProcessId") is not None
        }
        if process.poll() is None:
            process_ids.add(process.pid)
        stop_processes(process_ids)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            stop_processes({process.pid})
        time.sleep(1)
    evidence_logs = evidence_dir / "RuntimeLogs"
    evidence_logs.mkdir(parents=True, exist_ok=True)
    copied_logs: list[Path] = []
    for index, (label, source_log) in enumerate(changed_logs(roots, before), start=1):
        destination = evidence_logs / f"{label}-{index:02d}-{source_log.name}"
        try:
            shutil.copyfile(source_log, destination)
            copied_logs.append(destination)
        except OSError as error:
            with smoke_log.open("a", encoding="utf-8") as handle:
                handle.write(f"WARNING: could not copy {source_log}: {error}\n")
    session_logs = [
        path
        for path in copied_logs
        if path.name.lower().split("-", 2)[-1].startswith("reecho-session-") and path.stat().st_size > 0
    ]
    combined_text = "\n".join(
        path.read_text(encoding="utf-8", errors="replace") for path in session_logs
    )
    failures: list[str] = []
    warnings: list[str] = []
    if not process_alive:
        failures.append(f"packaged process did not remain alive for {duration:g} seconds")
    if configuration == "Development":
        if not session_logs:
            failures.append("no new non-empty ReEcho-session-*.log was generated")
        else:
            log_failures, warnings = analyze_smoke_logs(combined_text)
            failures.extend(log_failures)
    smoke_lines = [
        f"StartedAt={started}",
        f"Command={command_text(launch_command)}",
        f"DurationSeconds={duration:g}",
        f"ProcessAliveAtDeadline={str(process_alive).lower()}",
        f"ChangedLogs={len(copied_logs)}",
        f"SessionLogs={len(session_logs)}",
        "CopiedLogPaths:",
        *([str(path) for path in copied_logs] or ["<none>"]),
        "Warnings:",
        *(warnings or ["<none>"]),
        "Failures:",
        *(failures or ["<none>"]),
    ]
    smoke_log.write_text("\n".join(smoke_lines) + "\n", encoding="utf-8")
    clean_package_smoke_state(output)
    if failures:
        raise PackageError("Smoke test failed: " + "; ".join(failures))
    return SmokeResult(process_alive, copied_logs, session_logs, warnings)


def sync_evidence(run_evidence: Path, output: Path) -> Path:
    destination = output / "PackagingEvidence"
    destination.mkdir(parents=True, exist_ok=True)
    for item in run_evidence.iterdir():
        target = destination / item.name
        if item.is_dir():
            shutil.copytree(item, target, dirs_exist_ok=True)
        else:
            shutil.copy2(item, target)
    return destination


def write_failure_summary(
    evidence_dir: Path,
    error: BaseException,
    output: Path,
    source: SourceContext | None,
) -> None:
    summary = [
        "ReEcho Windows packaging failed.",
        f"Time={datetime.now().astimezone().isoformat(timespec='seconds')}",
        f"Output={output}",
        f"SourceProject={source.project if source else '<unavailable>'}",
        f"SourceSHA={source.revision if source else '<unavailable>'}",
        f"Error={error}",
        "",
        "Traceback:",
        "".join(traceback.format_exception(type(error), error, error.__traceback__)),
        "Build log: PackagingEvidence/BuildCookRun.log",
        "Smoke log: PackagingEvidence/SmokeTest.log",
    ]
    evidence_dir.mkdir(parents=True, exist_ok=True)
    (evidence_dir / "FailureSummary.txt").write_text("\n".join(summary), encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    configure_console_streams(sys.stdout, sys.stderr)
    args = parse_args(argv)
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    configuration = configuration_for(args)
    source: SourceContext | None = None
    output: Path | None = None
    engine_root: Path | None = None
    launcher: Path | None = None
    status_lines: list[str] = []
    run_evidence = Path(tempfile.mkdtemp(prefix="ReEchoPackageEvidence-"))
    failure: BaseException | None = None
    try:
        if platform.system() != "Windows":
            raise PackageError("This packaging script supports Windows only")
        source = prepare_source(args.remote_main)
        requested_output = args.output or default_output_path(source, configuration, timestamp)
        output = prepare_output(requested_output)
        status_lines = source_status(source)
        write_source_manifest(
            run_evidence / "ReEchoPackageSource.txt",
            source,
            configuration,
            args.map,
            status_lines,
            "IN_PROGRESS",
            None,
            None,
        )
        engine_root = find_engine_root(args.engine_root, source.root)
        command = make_uat_command(engine_root, source.project, output, args.map, configuration)
        print(f"[ReEchoPackage] Source: {source.branch} @ {source.revision}")
        print(f"[ReEchoPackage] Project: {source.project}")
        print(f"[ReEchoPackage] Configuration: {configuration}")
        print(f"[ReEchoPackage] Output: {output}")
        if args.dry_run:
            (run_evidence / "BuildCookRun.log").write_text(
                f"DRY RUN\n{command_text(command)}\n", encoding="utf-8"
            )
            write_source_manifest(
                run_evidence / "ReEchoPackageSource.txt",
                source,
                configuration,
                args.map,
                status_lines,
                "DRY_RUN",
                engine_root,
                None,
            )
            sync_evidence(run_evidence, output)
            finish_cleanup(source, run_evidence, output)
            print(f"[ReEchoPackage] Dry run complete: {command_text(command)}")
            return 0
        require_editor_closed()
        run_streamed(
            [sys.executable, str(source.root / "scripts" / "setup_lfs.py")],
            source.root,
            run_evidence / "BuildCookRun.log",
            "Git LFS setup and check",
        )
        run_streamed(command, source.root, run_evidence / "BuildCookRun.log", "Unreal BuildCookRun")
        launcher = find_packaged_exe(output)
        copied_csvs, expected_csvs = stage_and_verify_runtime_csvs(source.root, output)
        with (run_evidence / "BuildCookRun.log").open("a", encoding="utf-8") as log:
            log.write(
                f"\nRuntime CSV files expected={expected_csvs} copied={copied_csvs}\n"
                f"Launcher={launcher}\n"
            )
        if not args.no_smoke:
            smoke_test(launcher, output, configuration, args.smoke_seconds, run_evidence)
        else:
            (run_evidence / "SmokeTest.log").write_text(
                "Smoke test explicitly skipped with --no-smoke.\n", encoding="utf-8"
            )
        write_source_manifest(
            run_evidence / "ReEchoPackageSource.txt",
            source,
            configuration,
            args.map,
            status_lines,
            "SUCCESS",
            engine_root,
            launcher,
            "Smoke test was explicitly skipped." if args.no_smoke else "",
        )
        sync_evidence(run_evidence, output)
        shutil.copy2(run_evidence / "ReEchoPackageSource.txt", output / "ReEchoPackageSource.txt")
        finish_cleanup(source, run_evidence, output)
        print("[ReEchoPackage] BUILD SUCCESSFUL", flush=True)
        print(f"[ReEchoPackage] Package directory: {output}", flush=True)
        print(f"[ReEchoPackage] Launcher: {launcher}", flush=True)
        return 0
    except KeyboardInterrupt as error:
        failure = PackageError("Cancelled by user")
        failure.__cause__ = error
    except Exception as error:
        failure = error
    try:
        if output is None:
            output = emergency_output(timestamp)
        else:
            output.mkdir(parents=True, exist_ok=True)
        assert failure is not None
        write_failure_summary(run_evidence, failure, output, source)
        write_source_manifest(
            run_evidence / "ReEchoPackageSource.txt",
            source,
            configuration,
            args.map,
            status_lines,
            "FAILED",
            engine_root,
            launcher,
            str(failure),
        )
        sync_evidence(run_evidence, output)
        shutil.copy2(run_evidence / "ReEchoPackageSource.txt", output / "ReEchoPackageSource.txt")
        print(f"[ReEchoPackage] ERROR: {failure}", file=sys.stderr)
        print(f"[ReEchoPackage] Failure evidence: {output / 'PackagingEvidence'}", file=sys.stderr)
    finally:
        finish_cleanup(source, run_evidence, output)
    return 130 if failure and isinstance(failure.__cause__, KeyboardInterrupt) else 1


if __name__ == "__main__":
    raise SystemExit(main())
