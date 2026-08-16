#!/usr/bin/env python3
"""Deterministically sync the canonical ReEcho XLSX tables to runtime CSV."""

from __future__ import annotations

import argparse
import csv
import filecmp
import importlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
from typing import Callable, Iterable


ROOT = Path(__file__).resolve().parents[2]
DATA_DIR = ROOT / "Content" / "Data"
CANONICAL_XLSX = ROOT / "Design" / "Data" / "ReEchoData.xlsx"
CANONICAL_ENEMY_XLSX = ROOT / "Design" / "Data" / "ReEchoEnemyData.xlsx"
CANONICAL_AUDIO_XLSX = ROOT / "Design" / "Data" / "ReEchoAudioEvents.xlsx"
CANONICAL_WORKBOOKS = (CANONICAL_XLSX, CANONICAL_ENEMY_XLSX, CANONICAL_AUDIO_XLSX)
REQUIREMENTS = Path(__file__).with_name("requirements.txt")
EXPECTED_OPENPYXL = "3.1.5"
TRANSACTION_FILE = ".reecho_csv_publish_transaction.json"

TABLE_TO_CSV = {
    "tblCharacters": "characters.csv",
    "tblCharacterAliases": "character_aliases.csv",
    "tblCards": "cards.csv",
    "tblCardEffects": "card_effects.csv",
    "tblElements": "elements.csv",
    "tblStatuses": "statuses.csv",
    "tblReactions": "reactions.csv",
    "tblWeaponTypes": "weapon_types.csv",
    "tblWeapons": "weapons.csv",
    "tblAttackSteps": "attack_steps.csv",
    "tblSlotTypes": "slot_types.csv",
    "tblSlotProfiles": "slot_profiles.csv",
    "tblParts": "parts.csv",
    "tblPartEffects": "part_effects.csv",
    "tblRuntimeSmoke": "runtime_smoke.csv",
    "tblRuntimeSmokeEffects": "runtime_smoke_effects.csv",
    "tblEnemies": "enemies.csv",
    "tblEnemyAbilities": "enemy_abilities.csv",
    "tblBossPhases": "boss_phases.csv",
    "tblAudioEvents": "audio_events.csv",
}

SYSTEM_TABLES = frozenset({"tblRuntimeSmoke", "tblRuntimeSmokeEffects"})
AUTHORING_TABLES = frozenset(TABLE_TO_CSV) - SYSTEM_TABLES
SYSTEM_SHEETS = ("_WorkbookMeta", "_ExportMap", "_SystemData")
LOCKED_REFERENCE_SHEETS = ("属性S", "怪物体系M", "经济系统")

AUTHORING_LIST_VALIDATION_COLUMNS = {
    "tblCharacters": frozenset({"Enabled", "RoleId", "DefaultWeaponId", "PassiveBehaviorId", "RandomElementProjectiles"}),
    "tblCharacterAliases": frozenset({"CanonicalCharacterId"}),
    "tblCards": frozenset({"PromotionRoleId", "OfferGroup", "Enabled", "Offerable", "StackPolicy", "ConflictPolicy", "ReviewStatus"}),
    "tblCardEffects": frozenset({"CardId", "Trigger", "EffectKind", "Target", "ValueOp", "BehaviorId", "ParamName"}),
    "tblElements": frozenset({"Id", "SourceWorkbookId", "Role", "VisualKey", "Enabled"}),
    "tblStatuses": frozenset({"BehaviorId", "StackPolicy", "RefreshPolicy", "Enabled"}),
    "tblReactions": frozenset({
        "TriggerElementId", "AttachmentElementId", "BehaviorId", "FormulaId", "StatusId", "CanCrit",
        "AffectedByEchoEfficiency", "ClearsAttachment", "Enabled",
    }),
    "tblWeaponTypes": frozenset({"Id", "BaseAttackPatternId", "SlotProfileId", "Enabled"}),
    "tblWeapons": frozenset({"WeaponTypeId", "VisualKey", "InputSlot", "StartSelectable", "AttackPatternId", "Enabled"}),
    "tblAttackSteps": frozenset({"AttackPatternId", "Invulnerable", "BehaviorId", "FormulaId", "ConditionId", "Enabled"}),
    "tblSlotTypes": frozenset({"Enabled"}),
    "tblSlotProfiles": frozenset({"WeaponTypeId", "SlotTypeId", "Required", "Enabled"}),
    "tblParts": frozenset({"WeaponTypeId", "SlotTypeId", "Rarity", "Enabled", "ReviewStatus", "ImplementationStatus"}),
    "tblPartEffects": frozenset({
        "PartId", "Trigger", "EffectKind", "Target", "ValueOp", "BehaviorId", "FormulaId", "AttackPatternId",
        "ParamName", "StackPolicy", "Enabled",
    }),
    "tblEnemies": frozenset({"Archetype", "BehaviorProfileId", "PresentationId", "Enabled", "Boss"}),
    "tblEnemyAbilities": frozenset({"OwnerEnemyId", "BehaviorId", "Enabled", "TargetingMode", "LockTiming"}),
    "tblBossPhases": frozenset({"BossEnemyId", "EchoPolicy", "RefillHealthPolicy", "Enabled"}),
    "tblAudioEvents": frozenset({"Bus", "EventType", "Spatial3D", "PausePolicy"}),
}

REFERENCE_LIST_VALIDATION_FORMULAS = {
    ("tblCharacters", "DefaultWeaponId"): 'INDIRECT("tblWeapons[Id]")',
    ("tblCharacterAliases", "CanonicalCharacterId"): 'INDIRECT("tblCharacters[Id]")',
    ("tblCardEffects", "CardId"): 'INDIRECT("tblCards[Id]")',
    ("tblReactions", "TriggerElementId"): 'INDIRECT("tblElements[Id]")',
    ("tblReactions", "AttachmentElementId"): 'INDIRECT("tblElements[Id]")',
    ("tblWeaponTypes", "SlotProfileId"): 'INDIRECT("tblSlotProfiles[Id]")',
    ("tblWeapons", "WeaponTypeId"): 'INDIRECT("tblWeaponTypes[Id]")',
    ("tblSlotProfiles", "SlotTypeId"): 'INDIRECT("tblSlotTypes[Id]")',
    ("tblParts", "SlotTypeId"): 'INDIRECT("tblSlotTypes[Id]")',
    ("tblPartEffects", "PartId"): 'INDIRECT("tblParts[PartId]")',
    ("tblEnemyAbilities", "OwnerEnemyId"): 'INDIRECT("tblEnemies[Id]")',
    ("tblBossPhases", "BossEnemyId"): 'INDIRECT("tblEnemies[Id]")',
}


@dataclass(frozen=True)
class ColumnSpec:
    kind: str
    required: bool
    min_value: Decimal | None = None
    max_value: Decimal | None = None
    reference_table: str | None = None


@dataclass(frozen=True)
class ExportOwner:
    sheet: str
    table: str
    output_csv: str
    sort_key: int


class SyncError(Exception):
    pass


class LocatedError(SyncError):
    def __init__(self, sheet: str, table: str, row: int, column: str, message: str) -> None:
        super().__init__(f"{sheet}:{table}:row {row}:column {column}: {message}")


def check_dependencies() -> None:
    try:
        openpyxl = importlib.import_module("openpyxl")
    except ImportError as exc:
        raise SyncError(
            "Missing XLSX dependency 'openpyxl'. Install the repository-locked dependency with:\n"
            f"  {sys.executable} -m pip install -r {REQUIREMENTS}"
        ) from exc
    version = getattr(openpyxl, "__version__", "")
    if version != EXPECTED_OPENPYXL:
        raise SyncError(
            f"Unsupported openpyxl version {version!r}; expected {EXPECTED_OPENPYXL!r}. Reinstall with:\n"
            f"  {sys.executable} -m pip install -r {REQUIREMENTS}"
        )


def stable_id(value: str) -> bool:
    return bool(value) and value == value.strip() and re.fullmatch(r"[A-Za-z0-9_.-]+", value) is not None


def decimal_from_text(value: str) -> Decimal:
    try:
        parsed = Decimal(value)
    except InvalidOperation as exc:
        raise SyncError("Value must be a finite number") from exc
    if not parsed.is_finite():
        raise SyncError("Value must be a finite number")
    return parsed


def format_decimal(value: int | float | Decimal) -> str:
    parsed = Decimal(str(value))
    if parsed == parsed.to_integral_value():
        return str(parsed.quantize(Decimal(1)))
    return format(parsed.normalize(), "f")


def read_manifest_whitelist(data_dir: Path = DATA_DIR) -> set[str]:
    with (data_dir / "reecho_data_manifest.csv").open("r", encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    return {row["FileName"] for row in rows}


def load_schema() -> dict[str, list[str]]:
    schema: dict[str, list[str]] = {}
    with (DATA_DIR / "csv_schema.csv").open("r", encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            schema.setdefault(row["TableId"], []).append(row["ColumnName"])
    return schema


def load_schema_specs() -> dict[str, dict[str, ColumnSpec]]:
    specs: dict[str, dict[str, ColumnSpec]] = {}
    with (DATA_DIR / "csv_schema.csv").open("r", encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            specs.setdefault(row["TableId"], {})[row["ColumnName"]] = ColumnSpec(
                kind=row["Type"],
                required=row["Required"] == "true",
                min_value=Decimal(row["Min"]) if row["Min"] else None,
                max_value=Decimal(row["Max"]) if row["Max"] else None,
                reference_table=row["ReferencesTable"] or None,
            )
    return specs


def table_id_for_csv(output_csv: str) -> str:
    with (DATA_DIR / "reecho_data_manifest.csv").open("r", encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            if row["FileName"] == output_csv:
                return row["TableId"]
    raise SyncError(f"Manifest does not contain {output_csv}")


def workbook_tables(workbook) -> dict[str, tuple[object, object]]:
    found = {}
    for sheet in workbook.worksheets:
        for table in sheet.tables.values():
            found[table.name] = (sheet, table)
    return found


def cell_text(cell, sheet: str, table: str, header: str, spec: ColumnSpec, row_number: int) -> str:
    if cell.data_type == "f" or (isinstance(cell.value, str) and cell.value.startswith("=")):
        raise LocatedError(sheet, table, row_number, header, "Formula cells are not allowed in export tables")
    if cell.value is None:
        text = ""
    elif isinstance(cell.value, bool):
        text = "true" if cell.value else "false"
    elif isinstance(cell.value, (int, float, Decimal)):
        text = format_decimal(cell.value)
    else:
        text = str(cell.value)

    if spec.required and text == "":
        raise LocatedError(sheet, table, row_number, header, "Required value is empty")
    if text == "" and not spec.required:
        return text

    if spec.kind in {"StableId", "TextKey", "BehaviorId", "EffectKind", "FormulaId", "AttackPatternId", "ValueOp", "ForeignKey"}:
        if not stable_id(text):
            raise LocatedError(sheet, table, row_number, header, "Stable id contains unsupported characters or whitespace")
    elif spec.kind == "StableIdList":
        for part in [item for item in text.split("|") if item]:
            if not stable_id(part):
                raise LocatedError(sheet, table, row_number, header, "StableIdList contains an invalid id")
    elif spec.kind == "Bool":
        if text not in {"true", "false"}:
            raise LocatedError(sheet, table, row_number, header, "Boolean must be lowercase true or false")
    elif spec.kind == "ValueOp":
        if text not in {"Add", "Multiply", "Override"}:
            raise LocatedError(sheet, table, row_number, header, "ValueOp must be Add, Multiply or Override")
    elif spec.kind in {"Float", "PercentDecimal", "Int"}:
        try:
            parsed = decimal_from_text(text)
        except SyncError as exc:
            raise LocatedError(sheet, table, row_number, header, str(exc)) from exc
        if spec.kind == "Int" and parsed != parsed.to_integral_value():
            raise LocatedError(sheet, table, row_number, header, "Value must be an integer")
        if spec.min_value is not None and parsed < spec.min_value:
            raise LocatedError(sheet, table, row_number, header, f"Value is below minimum {spec.min_value}")
        if spec.max_value is not None and parsed > spec.max_value:
            raise LocatedError(sheet, table, row_number, header, f"Value exceeds maximum {spec.max_value}")
    return text


def extract_table(sheet, table, table_name: str, table_id: str, specs: dict[str, ColumnSpec]):
    from openpyxl.utils.cell import range_boundaries

    min_col, min_row, max_col, max_row = range_boundaries(table.ref)
    headers = [str(sheet.cell(row=min_row, column=col).value or "") for col in range(min_col, max_col + 1)]
    expected_headers = list(specs)
    if headers != expected_headers:
        missing = [header for header in expected_headers if header not in headers]
        extra = [header for header in headers if header not in expected_headers]
        raise LocatedError(
            sheet.title,
            table_name,
            min_row,
            "Header",
            f"Columns do not match csv_schema for {table_id}; missing={missing} extra={extra}",
        )

    rows: list[list[str]] = [headers]
    source_rows: dict[int, tuple[str, str, int]] = {}
    for row_number in range(min_row + 1, max_row + 1):
        values: list[str] = []
        empty = True
        for col_index, header in enumerate(headers, start=min_col):
            value = cell_text(
                sheet.cell(row=row_number, column=col_index),
                sheet.title,
                table_name,
                header,
                specs[header],
                row_number,
            )
            values.append(value)
            empty = empty and value == ""
        if not empty:
            rows.append(values)
            source_rows[len(rows)] = (sheet.title, table_name, row_number)
    return rows, source_rows


def read_export_map(workbook) -> list[ExportOwner]:
    tables = workbook_tables(workbook)
    if "tblExportMap" not in tables:
        raise LocatedError("_ExportMap", "tblExportMap", 1, "TableName", "Missing export map table")
    sheet, table = tables["tblExportMap"]
    rows, _ = extract_export_map_rows(sheet, table)
    manifest_outputs = read_manifest_whitelist()
    seen_tables: set[str] = set()
    seen_outputs: set[str] = set()
    owners: list[ExportOwner] = []
    for excel_row, row in rows:
        table_name = row["TableName"]
        output_csv = row["OutputCsv"]
        output_path = Path(output_csv)
        if output_path.is_absolute() or output_path.name != output_csv or ".." in output_path.parts:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "OutputCsv", "OutputCsv must be a plain manifest filename")
        if table_name not in TABLE_TO_CSV:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "TableName", f"Unknown table {table_name}")
        if table_name not in tables:
            raise LocatedError(row["SheetName"], table_name, 1, "TableName", "Workbook table is missing")
        if TABLE_TO_CSV[table_name] != output_csv:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "OutputCsv", "Table output does not match locked mapping")
        if output_csv not in manifest_outputs:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "OutputCsv", "OutputCsv is not in manifest whitelist")
        if table_name in seen_tables:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "TableName", "Duplicate table owner")
        if output_csv in seen_outputs:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "OutputCsv", "Duplicate output owner")
        seen_tables.add(table_name)
        seen_outputs.add(output_csv)
        try:
            sort_key = int(row["SortKey"])
        except ValueError as exc:
            raise LocatedError(sheet.title, "tblExportMap", excel_row, "SortKey", "SortKey must be an integer") from exc
        owners.append(ExportOwner(row["SheetName"], table_name, output_csv, sort_key))
    workbook_export_tables = set(tables) & set(TABLE_TO_CSV)
    expected_outputs = {TABLE_TO_CSV[table_name] for table_name in workbook_export_tables}
    if seen_outputs != expected_outputs:
        raise LocatedError(
            sheet.title,
            "tblExportMap",
            1,
            "OutputCsv",
            f"Workbook export ownership mismatch missing={sorted(expected_outputs - seen_outputs)} extra={sorted(seen_outputs - expected_outputs)}",
        )
    return sorted(owners, key=lambda owner: (owner.sort_key, owner.output_csv))


def validate_workbook_protection(workbook, owners: list[ExportOwner]) -> None:
    from openpyxl.utils.cell import get_column_letter, range_boundaries

    tables = workbook_tables(workbook)
    editable_ranges: dict[str, list[tuple[int, int, int, int, str]]] = {}

    for owner in owners:
        if owner.table not in tables:
            raise LocatedError(owner.sheet, owner.table, 1, "TableName", "Workbook table is missing")

    for owner in owners:
        if owner.table not in AUTHORING_TABLES:
            continue
        sheet, table = tables[owner.table]
        min_col, min_row, max_col, max_row = range_boundaries(table.ref)
        if not sheet.protection.sheet:
            raise LocatedError(sheet.title, owner.table, min_row, "Protection", "Authoring sheet protection must remain enabled")
        if sheet.protection.insertRows or sheet.protection.deleteRows:
            raise LocatedError(
                sheet.title,
                owner.table,
                min_row,
                "Protection",
                "Authoring sheet must allow Table row insertion and deletion",
            )
        for col in range(min_col, max_col + 1):
            if not sheet.cell(row=min_row, column=col).protection.locked:
                raise LocatedError(sheet.title, owner.table, min_row, get_column_letter(col), "Table headers must remain locked")
        for row in range(min_row + 1, max_row + 1):
            for col in range(min_col, max_col + 1):
                if sheet.cell(row=row, column=col).protection.locked:
                    raise LocatedError(
                        sheet.title,
                        owner.table,
                        row,
                        get_column_letter(col),
                        "Production Table data cells must be unlocked for authoring",
                    )
        editable_ranges.setdefault(sheet.title, []).append((min_col, min_row + 1, max_col, max_row, owner.table))

    for sheet_name, ranges in editable_ranges.items():
        sheet = workbook[sheet_name]
        for row in sheet.iter_rows():
            for cell in row:
                if cell.protection.locked:
                    continue
                if not any(min_col <= cell.column <= max_col and min_row <= cell.row <= max_row for min_col, min_row, max_col, max_row, _ in ranges):
                    raise LocatedError(
                        sheet.title,
                        ranges[0][4],
                        cell.row,
                        cell.column_letter,
                        "Only production Table data cells may be unlocked",
                    )

    main_workbook = "tblCharacters" in tables
    required_protected_sheets = (*SYSTEM_SHEETS, *LOCKED_REFERENCE_SHEETS) if main_workbook else ("_ExportMap",)
    for sheet_name in required_protected_sheets:
        if sheet_name not in workbook.sheetnames:
            raise SyncError(f"Protected workbook sheet is missing: {sheet_name}")
        sheet = workbook[sheet_name]
        diagnostic_table = next(iter(sheet.tables), "tblProtection")
        if not sheet.protection.sheet:
            raise LocatedError(sheet.title, diagnostic_table, 1, "Protection", "Protected sheet must remain enabled")
        if not sheet.protection.insertRows or not sheet.protection.deleteRows:
            raise LocatedError(sheet.title, diagnostic_table, 1, "Protection", "Protected sheet must forbid row insertion and deletion")
        for row in sheet.iter_rows():
            for cell in row:
                if not cell.protection.locked:
                    raise LocatedError(sheet.title, diagnostic_table, cell.row, cell.column_letter, "Protected sheet cells must remain locked")


def validate_workbook_data_validations(workbook) -> None:
    from openpyxl.utils.cell import range_boundaries

    tables = workbook_tables(workbook)
    for table_name, column_names in AUTHORING_LIST_VALIDATION_COLUMNS.items():
        if table_name not in tables:
            continue
        sheet, table = tables[table_name]
        min_col, min_row, max_col, max_row = range_boundaries(table.ref)
        headers = [str(sheet.cell(row=min_row, column=col).value or "") for col in range(min_col, max_col + 1)]
        for column_name in column_names:
            if column_name not in headers:
                raise LocatedError(sheet.title, table_name, min_row, column_name, "Validation column is missing")
            column = min_col + headers.index(column_name)
            first_cell = sheet.cell(row=min_row + 1, column=column).coordinate
            last_cell = sheet.cell(row=max_row, column=column).coordinate
            matches = [
                validation
                for validation in sheet.data_validations.dataValidation
                if validation.type == "list" and first_cell in validation.cells and last_cell in validation.cells
            ]
            if not matches:
                raise LocatedError(
                    sheet.title,
                    table_name,
                    min_row + 1,
                    column_name,
                    "Finite or reference field must use an in-cell list validation over the complete Table data column",
                )
            strict_matches = [
                validation
                for validation in matches
                if validation.errorStyle in {"stop", None}
                and validation.showErrorMessage is True
                and validation.showDropDown in {False, None}
                and validation.allowBlank in {False, None}
            ]
            if not strict_matches:
                raise LocatedError(
                    sheet.title,
                    table_name,
                    min_row + 1,
                    column_name,
                    "List validation must show its dropdown and stop blank or unsupported values",
                )
            expected_formula = REFERENCE_LIST_VALIDATION_FORMULAS.get((table_name, column_name))
            if expected_formula and all(str(validation.formula1 or "").lstrip("=") != expected_formula for validation in strict_matches):
                raise LocatedError(
                    sheet.title,
                    table_name,
                    min_row + 1,
                    column_name,
                    f"Reference dropdown must use {expected_formula}",
                )


def extract_export_map_rows(sheet, table):
    from openpyxl.utils.cell import range_boundaries

    min_col, min_row, max_col, max_row = range_boundaries(table.ref)
    headers = [str(sheet.cell(row=min_row, column=col).value or "") for col in range(min_col, max_col + 1)]
    expected = ["SheetName", "TableName", "AdapterVersion", "OutputCsv", "SortKey"]
    if headers != expected:
        raise LocatedError(sheet.title, "tblExportMap", min_row, "Header", f"Expected {expected}")
    rows = []
    for row_number in range(min_row + 1, max_row + 1):
        values = {header: str(sheet.cell(row=row_number, column=min_col + idx).value or "") for idx, header in enumerate(headers)}
        if all(value == "" for value in values.values()):
            continue
        if values["AdapterVersion"] != "1":
            raise LocatedError(sheet.title, "tblExportMap", row_number, "AdapterVersion", "Only adapter version 1 is supported")
        rows.append((row_number, values))
    return rows, {}


def write_csv(path: Path, rows: list[list[str]]) -> bytes:
    import io

    buffer = io.StringIO(newline="")
    writer = csv.writer(buffer, lineterminator="\r\n")
    writer.writerows(rows)
    text = buffer.getvalue()
    data = text.encode("utf-8")
    path.write_bytes(data)
    return data


def validate_generated_package(package_dir: Path, source_map: dict[tuple[str, int], tuple[str, str, int]]) -> None:
    sys.path.insert(0, str(ROOT / "scripts"))
    validate_project = importlib.import_module("validate_project")
    try:
        validate_project.validate_csv_package(package_dir)
    except Exception as exc:
        message = str(exc)
        match = re.search(r"([^/\\:]+\.csv):(\d+)(?::([^:]+))?", message)
        if match:
            csv_name = match.group(1)
            line = int(match.group(2))
            column = match.group(3) or "Validation"
            if (csv_name, line) in source_map:
                sheet, table, row = source_map[(csv_name, line)]
                raise LocatedError(sheet, table, row, column, message) from exc
        raise SyncError(message) from exc


def generate_package(input_paths: Path | Iterable[Path], package_dir: Path):
    from openpyxl import load_workbook

    paths = [input_paths] if isinstance(input_paths, Path) else list(input_paths)
    schema_columns = load_schema()
    schema_specs = load_schema_specs()
    source_map: dict[tuple[str, int], tuple[str, str, int]] = {}
    csv_bytes: dict[str, bytes] = {}
    all_owners: list[ExportOwner] = []
    package_dir.mkdir(parents=True, exist_ok=True)

    for input_path in paths:
        workbook = load_workbook(input_path, data_only=False, read_only=False)
        owners = read_export_map(workbook)
        validate_workbook_protection(workbook, owners)
        validate_workbook_data_validations(workbook)
        tables = workbook_tables(workbook)
        for owner in owners:
            if owner.output_csv in csv_bytes:
                raise LocatedError(owner.sheet, owner.table, 1, "OutputCsv", "CSV is owned by more than one workbook")
            if owner.table not in tables:
                raise LocatedError(owner.sheet, owner.table, 1, "TableName", "Workbook table is missing")
            sheet, table = tables[owner.table]
            if sheet.title != owner.sheet:
                raise LocatedError(owner.sheet, owner.table, 1, "SheetName", f"Table is on sheet {sheet.title}")
            table_id = table_id_for_csv(owner.output_csv)
            rows, row_map = extract_table(sheet, table, owner.table, table_id, schema_specs[table_id])
            if rows[0] != schema_columns[table_id]:
                raise LocatedError(owner.sheet, owner.table, 1, "Header", "Table columns do not match schema order")
            csv_bytes[owner.output_csv] = write_csv(package_dir / owner.output_csv, rows)
            for csv_line, location in row_map.items():
                source_map[(owner.output_csv, csv_line)] = location
        all_owners.extend(owners)

    expected_outputs = set(TABLE_TO_CSV.values())
    actual_outputs = set(csv_bytes)
    if actual_outputs != expected_outputs:
        raise SyncError(
            f"Cross-workbook export ownership mismatch missing={sorted(expected_outputs - actual_outputs)} "
            f"extra={sorted(actual_outputs - expected_outputs)}"
        )
    for support_file in ("reecho_data_manifest.csv", "csv_schema.csv"):
        shutil.copy2(DATA_DIR / support_file, package_dir / support_file)
    validate_generated_package(package_dir, source_map)
    return all_owners, csv_bytes


def diff_against_content(csv_bytes: dict[str, bytes], data_dir: Path = DATA_DIR) -> list[str]:
    drift = []
    for name, generated in sorted(csv_bytes.items()):
        target = data_dir / name
        if not target.exists() or target.read_bytes() != generated:
            drift.append(name)
    return drift


def transaction_path(data_dir: Path) -> Path:
    return data_dir / TRANSACTION_FILE


def ensure_data_dir_path(data_dir: Path, name: str, whitelist: set[str], context: str) -> Path:
    candidate = Path(name)
    if candidate.is_absolute() or candidate.name != name or ".." in candidate.parts:
        raise SyncError(f"{context}: transaction file name must be a plain manifest filename")
    if name not in whitelist:
        raise SyncError(f"{context}: transaction file {name!r} is not in the manifest whitelist")
    data_root = data_dir.resolve()
    target = (data_root / name).resolve()
    if target.parent != data_root:
        raise SyncError(f"{context}: transaction target escapes data_dir")
    return target


def ensure_backup_dir(data_dir: Path, backup_name: str) -> Path:
    backup_path = Path(backup_name)
    if backup_path.is_absolute() or backup_path.name != backup_name or ".." in backup_path.parts:
        raise SyncError("transaction backup_dir must be a controlled relative directory name")
    if not backup_name.startswith(".reecho_csv_publish_backup_"):
        raise SyncError("transaction backup_dir is not a ReEcho CSV publish backup directory")
    data_root = data_dir.resolve()
    backup_dir = (data_root / backup_name).resolve()
    if backup_dir.parent != data_root:
        raise SyncError("transaction backup_dir escapes data_dir")
    return backup_dir


def recover_transaction(data_dir: Path, manifest_whitelist: set[str] | None = None) -> None:
    marker = transaction_path(data_dir)
    if not marker.exists():
        return
    whitelist = manifest_whitelist or read_manifest_whitelist(data_dir)
    try:
        state = json.loads(marker.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise SyncError("transaction marker is not valid JSON") from exc
    backup_dir = ensure_backup_dir(data_dir, str(state.get("backup_dir", "")))
    files = state.get("files", [])
    if not isinstance(files, list) or not files:
        raise SyncError("transaction marker must contain a non-empty files list")
    validated: list[tuple[Path, Path]] = []
    for item in files:
        if not isinstance(item, str):
            raise SyncError("transaction marker files must be relative manifest filenames")
        target = ensure_data_dir_path(data_dir, item, whitelist, "transaction recovery")
        backup = (backup_dir / item).resolve()
        if backup.parent != backup_dir.resolve():
            raise SyncError("transaction backup path escapes backup_dir")
        validated.append((backup, target))
    for backup, target in validated:
        if backup.exists():
            os.replace(backup, target)
    marker.unlink(missing_ok=True)
    if backup_dir.exists():
        shutil.rmtree(backup_dir, ignore_errors=True)


def rollback_transaction(state: dict[str, object], data_dir: Path, whitelist: set[str]) -> None:
    backup_dir = ensure_backup_dir(data_dir, str(state["backup_dir"]))
    for name in reversed(state["files"]):
        target = ensure_data_dir_path(data_dir, str(name), whitelist, "transaction rollback")
        backup = (backup_dir / str(name)).resolve()
        if backup.parent != backup_dir.resolve():
            raise SyncError("transaction backup path escapes backup_dir")
        if backup.exists():
            os.replace(backup, target)
    transaction_path(data_dir).unlink(missing_ok=True)
    shutil.rmtree(backup_dir, ignore_errors=True)


def commit_transaction(state: dict[str, object], data_dir: Path) -> None:
    backup_dir = ensure_backup_dir(data_dir, str(state["backup_dir"]))
    transaction_path(data_dir).unlink(missing_ok=True)
    shutil.rmtree(backup_dir, ignore_errors=True)


def publish(
    csv_bytes: dict[str, bytes],
    selected_outputs: Iterable[str],
    data_dir: Path = DATA_DIR,
    final_validator: Callable[[], None] | None = None,
    manifest_whitelist: set[str] | None = None,
) -> None:
    selected = list(selected_outputs)
    whitelist = manifest_whitelist or read_manifest_whitelist(data_dir)
    recover_transaction(data_dir, whitelist)
    for name in selected:
        if name not in csv_bytes:
            raise SyncError(f"Cannot publish {name!r}; no generated bytes were produced")
        ensure_data_dir_path(data_dir, name, whitelist, "transaction publish")
    backup_dir = data_dir / f".reecho_csv_publish_backup_{next(tempfile._get_candidate_names())}"
    backup_dir.mkdir()
    state: dict[str, object] = {"backup_dir": backup_dir.name, "files": []}
    for name in selected:
        target = ensure_data_dir_path(data_dir, name, whitelist, "transaction publish")
        backup = backup_dir / name
        shutil.copy2(target, backup)
        state["files"].append(name)
    marker = transaction_path(data_dir)
    marker.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")

    replaced = 0
    try:
        for name in selected:
            temp_path = data_dir / f".{name}.{next(tempfile._get_candidate_names())}.tmp"
            temp_path.write_bytes(csv_bytes[name])
            os.replace(temp_path, ensure_data_dir_path(data_dir, name, whitelist, "transaction publish"))
            replaced += 1
            fail_after = os.environ.get("REECHO_XLSX_FAIL_AFTER_REPLACE")
            if fail_after and replaced >= int(fail_after):
                raise OSError("Injected publish failure after replace")
        if final_validator is not None:
            final_validator()
    except Exception:
        rollback_transaction(state, data_dir, whitelist)
        raise
    commit_transaction(state, data_dir)


def run_project_validator() -> None:
    if os.environ.get("REECHO_XLSX_FAIL_FINAL_VALIDATOR"):
        raise SyncError("Injected final project validator failure")
    result = subprocess.run([sys.executable, str(ROOT / "scripts" / "validate_project.py")], cwd=ROOT, text=True)
    if result.returncode != 0:
        raise SyncError("scripts/validate_project.py failed after publish")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input",
        type=Path,
        action="append",
        help="Workbook input; repeat to provide a complete cross-workbook package. Defaults to both canonical workbooks.",
    )
    parser.add_argument("--output-dir", type=Path, help="Test output directory for non-canonical inputs.")
    parser.add_argument("--check", action="store_true", help="Validate and compare without writing production CSV.")
    parser.add_argument("--sheet", help="Publish only CSVs owned by one workbook sheet after full validation.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    check_dependencies()
    input_paths = [path.resolve() for path in (args.input or CANONICAL_WORKBOOKS)]
    canonical_paths = [path.resolve() for path in CANONICAL_WORKBOOKS]
    for input_path in input_paths:
        if not input_path.exists():
            raise SyncError(f"Workbook does not exist: {input_path}")
    non_canonical = input_paths != canonical_paths
    if non_canonical and not args.check and args.output_dir is None:
        raise SyncError("Non-canonical --input cannot publish to Content/Data; add --check or --output-dir <test-dir>.")

    with tempfile.TemporaryDirectory(prefix="reecho_xlsx_package_", dir=DATA_DIR.parent) as temp:
        package_dir = Path(temp)
        owners, csv_bytes = generate_package(input_paths, package_dir)
        if args.check:
            drift = diff_against_content(csv_bytes) if not non_canonical or args.output_dir is None else []
            if drift:
                raise SyncError("Generated CSV drift detected: " + ", ".join(drift))
            print("[PASS] XLSX export package validates and production CSV bytes match.")
            return 0

        if args.output_dir is not None:
            args.output_dir.mkdir(parents=True, exist_ok=True)
            for name, data in csv_bytes.items():
                (args.output_dir / name).write_bytes(data)
            print(f"[PASS] Wrote validated test CSV package to {args.output_dir}")
            return 0

        selected = [owner.output_csv for owner in owners if args.sheet is None or owner.sheet == args.sheet]
        if args.sheet is not None and not selected:
            raise SyncError(f"Sheet {args.sheet!r} owns no export CSV.")
        publish(csv_bytes, selected, final_validator=run_project_validator)
    print("[PASS] XLSX export package validated and published: " + ", ".join(selected))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SyncError as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        raise SystemExit(1)
