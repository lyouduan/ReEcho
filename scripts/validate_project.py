#!/usr/bin/env python3
"""Fast static validation that does not require Unreal Engine."""

from __future__ import annotations

import csv
import io
import json
import math
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Content" / "Data"
CSV_SCHEMA_VERSION = 1
REGISTERED_BEHAVIOR_IDS = {
    "None",
    "RuntimeSmoke.LogValue",
    "Character.SageBonusChoice",
    "Character.PoetReactionGrowth",
    "Character.BraveForge",
    "Card.StatModifier",
    "Card.InstantRecovery",
    "Status.ElementImmunity",
    "Status.Burn",
    "Reaction.Burn",
    "Reaction.Vaporize",
    "Reaction.Growth",
    "Reaction.Conduct",
    "Reaction.Enhance",
    "Weapon.AttackStep",
    "Weapon.DashStrike",
    "Part.CoreDamageChannel",
    "Part.StatModifier",
    "Part.AttackPatternReplacement",
    "Part.OnKillHealPercent",
}
REGISTERED_EFFECT_KINDS = {
    "ScalarModifier",
    "StatModifier",
    "InstantRecovery",
    "ElementReaction",
    "WeaponDamageChannel",
    "AttackPatternReplacement",
    "ParameterizedBehavior",
    "UniqueBehavior",
}
REGISTERED_FORMULA_IDS = {
    "None",
    "Element.ElementAttackDot",
    "Element.ElementAttackSquared",
    "Element.AttachInRadius",
    "Element.ChainElementAttack",
    "Element.EnhanceNextReaction",
    "Weapon.PhysicalOrElementalCoefficient",
}
REGISTERED_ATTACK_PATTERN_IDS = {
    "None",
    "Pattern.DaggerCombo",
    "Pattern.DaggerDashOnly",
    "Pattern.LongSwordCombo",
    "Pattern.ScytheSweep",
    "Pattern.WhipCombo",
    "Pattern.BowShot",
    "Pattern.GunShot",
    "Pattern.StaffProjectile",
    "Pattern.MoonStaffWave",
    "Pattern.ElementalProjectile",
}
VALUE_OPS = {"Add", "Multiply", "Override"}
CARD_TARGETS = {
    "HpMax",
    "HpPoint",
    "PhysicalAttack",
    "ElementalAttack",
    "Block",
    "AttackSpeed",
    "MovementSpeed",
    "EchoEfficiency",
}
CARD_EFFECT_BEHAVIOR_PAIRS = {
    "StatModifier": "Card.StatModifier",
    "InstantRecovery": "Card.InstantRecovery",
}
ELEMENT_IDS = {"Flame", "Lightning", "Grass", "Water"}
ELEMENT_ROLES = {"Trigger", "Attachment"}
STATUS_BEHAVIOR_PAIRS = {
    "Z_Elemental_Immunity": "Status.ElementImmunity",
    "Z_Burn": "Status.Burn",
}
REACTION_BEHAVIOR_FORMULA_PAIRS = {
    "Reaction.Burn": "Element.ElementAttackDot",
    "Reaction.Vaporize": "Element.ElementAttackSquared",
    "Reaction.Growth": "Element.AttachInRadius",
    "Reaction.Conduct": "Element.ChainElementAttack",
    "Reaction.Enhance": "Element.EnhanceNextReaction",
}
WEAPON_TYPE_IDS = {"Dagger", "LongSword", "Scythe", "Whip", "Bow", "Gun", "Staff"}
INPUT_SLOTS = {"None", "1", "2", "3"}
WEAPON_EFFECT_TARGETS = {
    "DamageChannel",
    "AttackSpeed",
    "AttackIntervalSeconds",
    "PhysicalCoefficient",
    "ElementalCoefficient",
    "AttackPattern",
    "OnKill",
}
PART_EFFECT_BEHAVIOR_PAIRS = {
    "WeaponDamageChannel": "Part.CoreDamageChannel",
    "StatModifier": "Part.StatModifier",
    "AttackPatternReplacement": "Part.AttackPatternReplacement",
    "UniqueBehavior": "Part.OnKillHealPercent",
}


class ValidationError(Exception):
    pass


@dataclass(frozen=True)
class CsvColumnSpec:
    kind: str
    required: bool = True
    min_value: float | None = None
    max_value: float | None = None
    reference_table: str | None = None


CSV_TABLES: dict[str, dict[str, CsvColumnSpec]] = {
    "RuntimeSmoke": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayNameKey": CsvColumnSpec("TextKey"),
        "Enabled": CsvColumnSpec("Bool"),
        "TestScalar": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "TestPercent": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "DistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=1000000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "ModifierOp": CsvColumnSpec("ValueOp"),
    },
    "RuntimeSmokeEffects": {
        "Id": CsvColumnSpec("StableId"),
        "RuntimeRowId": CsvColumnSpec("ForeignKey", reference_table="RuntimeSmoke"),
        "ParamName": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
    },
    "Characters": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "RoleId": CsvColumnSpec("StableId"),
        "PromotionPriority": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DefaultWeaponId": CsvColumnSpec("StableId"),
        "AppearanceId": CsvColumnSpec("StableId"),
        "PassiveBehaviorId": CsvColumnSpec("BehaviorId"),
        "PassiveValue": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "HpMax": CsvColumnSpec("Float", min_value=1.0, max_value=100000.0),
        "PhysicalAttack": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ElementalAttack": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "AttackSpeed": CsvColumnSpec("Float", min_value=0.1, max_value=100.0),
        "MovementSpeed": CsvColumnSpec("Float", min_value=0.1, max_value=100.0),
        "CriticalRate": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "CriticalEffect": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "EchoEfficiency": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "ReactionEfficiency": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "EverySecondAttackBonus": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RandomElementProjectiles": CsvColumnSpec("Bool"),
    },
    "CharacterAliases": {
        "Id": CsvColumnSpec("StableId"),
        "CanonicalCharacterId": CsvColumnSpec("ForeignKey", reference_table="Characters"),
        "Reason": CsvColumnSpec("Text"),
    },
    "Cards": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "Tier": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "Tags": CsvColumnSpec("StableIdList"),
        "PromotionRoleId": CsvColumnSpec("StableId"),
        "OfferGroup": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "Offerable": CsvColumnSpec("Bool"),
        "StackPolicy": CsvColumnSpec("StableId"),
        "ConflictPolicy": CsvColumnSpec("StableId"),
        "ReviewStatus": CsvColumnSpec("StableId"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "CardEffects": {
        "Id": CsvColumnSpec("StableId"),
        "CardId": CsvColumnSpec("ForeignKey", reference_table="Cards"),
        "Order": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Trigger": CsvColumnSpec("StableId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "Target": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "ParamName": CsvColumnSpec("StableId"),
        "ParamValue": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
    },
    "Elements": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "Role": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "DisplayNameKey": CsvColumnSpec("TextKey"),
        "ColorHex": CsvColumnSpec("Text"),
        "VisualKey": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Statuses": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "StackPolicy": CsvColumnSpec("StableId"),
        "RefreshPolicy": CsvColumnSpec("StableId"),
        "MutexGroup": CsvColumnSpec("StableId"),
        "Tags": CsvColumnSpec("StableIdList"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Reactions": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "TriggerElementId": CsvColumnSpec("ForeignKey", reference_table="Elements"),
        "AttachmentElementId": CsvColumnSpec("ForeignKey", reference_table="Elements"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "DamageMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "DamageIncrease": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "StatusId": CsvColumnSpec("ForeignKey", reference_table="Statuses"),
        "StatusDurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "EnhancementMultiplier": CsvColumnSpec("Float", min_value=1.0, max_value=100.0),
        "CanCrit": CsvColumnSpec("Bool"),
        "AffectedByEchoEfficiency": CsvColumnSpec("Bool"),
        "ClearsAttachment": CsvColumnSpec("Bool"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "WeaponTypes": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "BaseAttackPatternId": CsvColumnSpec("AttackPatternId"),
        "SlotProfileId": CsvColumnSpec("StableId"),
        "BaseIntervalSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "BaseRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "BaseArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "BaseProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "BaseConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "BaseExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ChainWindowSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Weapons": {
        "Id": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("ForeignKey", reference_table="WeaponTypes"),
        "DisplayName": CsvColumnSpec("Text"),
        "VisualKey": CsvColumnSpec("StableId"),
        "InputSlot": CsvColumnSpec("StableId"),
        "StartSelectable": CsvColumnSpec("Bool"),
        "LoadoutOrder": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "AttackIntervalSeconds": CsvColumnSpec("Float", min_value=0.01, max_value=60.0),
        "PhysicalCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "ElementalCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "DataRevision": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "AttackSteps": {
        "Id": CsvColumnSpec("StableId"),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "StepIndex": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "PhysicalCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "ElementalCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MovementCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "Invulnerable": CsvColumnSpec("Bool"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "ConditionId": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "SlotTypes": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "SlotProfiles": {
        "Id": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("StableId"),
        "SlotTypeId": CsvColumnSpec("ForeignKey", reference_table="SlotTypes"),
        "SlotCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Required": CsvColumnSpec("Bool"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Parts": {
        "Id": CsvColumnSpec("StableId"),
        "PartId": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("StableId"),
        "SlotTypeId": CsvColumnSpec("ForeignKey", reference_table="SlotTypes"),
        "DisplayName": CsvColumnSpec("Text", required=False),
        "Description": CsvColumnSpec("Text", required=False),
        "Rarity": CsvColumnSpec("StableId"),
        "Tags": CsvColumnSpec("StableIdList"),
        "Enabled": CsvColumnSpec("Bool"),
        "ReviewStatus": CsvColumnSpec("StableId"),
        "ImplementationStatus": CsvColumnSpec("StableId"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
    },
    "PartEffects": {
        "Id": CsvColumnSpec("StableId"),
        "PartId": CsvColumnSpec("ForeignKey", reference_table="Parts"),
        "Order": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Trigger": CsvColumnSpec("StableId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "Target": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "ParamName": CsvColumnSpec("StableId"),
        "ParamValue": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "CooldownSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "StackPolicy": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
    },
}


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def fail(message: str) -> None:
    raise ValidationError(message)


def load_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"{rel(path)}: {exc}")


def read_utf8_no_bom(path: Path) -> str:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        fail(f"{rel(path)}: {exc}")
    if raw.startswith(b"\xef\xbb\xbf"):
        fail(f"{rel(path)}: CSV must be UTF-8 without BOM")
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        fail(f"{rel(path)}: invalid UTF-8: {exc}")


def load_csv(path: Path) -> list[dict[str, str]]:
    text = read_utf8_no_bom(path)
    try:
        reader = csv.DictReader(io.StringIO(text), strict=True)
        if not reader.fieldnames:
            fail(f"{rel(path)}: CSV file is empty")
        headers = [header.strip() for header in reader.fieldnames]
        if len(headers) != len(set(headers)) or any(not header for header in headers):
            fail(f"{rel(path)}: header is empty or duplicated")
        rows: list[dict[str, str]] = []
        for row in reader:
            if row.get(None):
                fail(f"{rel(path)}:{reader.line_num}: row has more cells than headers")
            normalized = {header: (row.get(header) or "").strip() for header in reader.fieldnames}
            if any(value.startswith(("=", "@")) for value in normalized.values()):
                fail(f"{rel(path)}:{reader.line_num}: CSV cells must not contain spreadsheet formulas")
            normalized["__line__"] = str(reader.line_num)
            rows.append(normalized)
        return rows
    except csv.Error as exc:
        fail(f"{rel(path)}: {exc}")


def stable_id(value: str) -> bool:
    allowed = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.")
    return bool(value) and value == value.strip() and all(ch in allowed for ch in value)


def parse_float(path: Path, row: dict[str, str], column: str, spec: CsvColumnSpec) -> float:
    value = row[column]
    line = row["__line__"]
    try:
        parsed = float(value)
    except ValueError:
        fail(f"{rel(path)}:{line}:{column}: value must be a finite number")
    if not math.isfinite(parsed):
        fail(f"{rel(path)}:{line}:{column}: value must be a finite number")
    if spec.min_value is not None and parsed < spec.min_value:
        fail(f"{rel(path)}:{line}:{column}: value {parsed} is below {spec.min_value}")
    if spec.max_value is not None and parsed > spec.max_value:
        fail(f"{rel(path)}:{line}:{column}: value {parsed} exceeds {spec.max_value}")
    return parsed


def validate_csv_schema() -> None:
    schema_path = DATA / "csv_schema.csv"
    rows = load_csv(schema_path)
    required_columns = {
        "SchemaVersion",
        "TableId",
        "ColumnName",
        "Type",
        "Required",
        "Min",
        "Max",
        "ReferencesTable",
        "AllowedValues",
        "Description",
    }
    if set(rows[0].keys()) - {"__line__"} != required_columns:
        fail(f"{rel(schema_path)}: schema columns do not match the contract")

    declared = {(row["TableId"], row["ColumnName"]) for row in rows}
    for row in rows:
        line = row["__line__"]
        if row["SchemaVersion"] != str(CSV_SCHEMA_VERSION):
            fail(f"{rel(schema_path)}:{line}: unsupported schema version {row['SchemaVersion']}")
        if row["Required"] not in {"true", "false"}:
            fail(f"{rel(schema_path)}:{line}: Required must be true or false")
    for table_id, specs in CSV_TABLES.items():
        for column in specs:
            if (table_id, column) not in declared:
                fail(f"{rel(schema_path)}: missing contract row for {table_id}.{column}")


def validate_manifest(data_dir: Path) -> dict[str, Path]:
    manifest_path = data_dir / "reecho_data_manifest.csv"
    rows = load_csv(manifest_path)
    expected_headers = {"SchemaVersion", "TableId", "FileName", "PrimaryKey", "__line__"}
    if set(rows[0].keys()) != expected_headers:
        fail(f"{rel(manifest_path)}: manifest columns do not match the contract")

    entries: dict[str, Path] = {}
    for row in rows:
        line = row["__line__"]
        if row["SchemaVersion"] != str(CSV_SCHEMA_VERSION):
            fail(f"{rel(manifest_path)}:{line}: unsupported SchemaVersion {row['SchemaVersion']}")
        table_id = row["TableId"]
        if table_id not in CSV_TABLES:
            fail(f"{rel(manifest_path)}:{line}: unknown TableId {table_id!r}")
        if table_id in entries:
            fail(f"{rel(manifest_path)}:{line}: duplicate TableId {table_id!r}")
        if row["PrimaryKey"] != "Id":
            fail(f"{rel(manifest_path)}:{line}: PrimaryKey must be Id")
        file_name = row["FileName"]
        if "/" in file_name or "\\" in file_name:
            fail(f"{rel(manifest_path)}:{line}: FileName must stay inside its data directory")
        entries[table_id] = data_dir / file_name

    missing_tables = sorted(set(CSV_TABLES) - set(entries))
    if missing_tables:
        fail(f"{rel(manifest_path)}: missing tables: {', '.join(missing_tables)}")
    return entries


def validate_table(path: Path, table_id: str, references: dict[str, set[str]]) -> set[str]:
    specs = CSV_TABLES[table_id]
    rows = load_csv(path)
    headers = set(rows[0].keys()) - {"__line__"} if rows else set(specs)
    if headers != set(specs):
        fail(f"{rel(path)}: expected columns {sorted(specs)}, got {sorted(headers)}")

    ids: set[str] = set()
    for row in rows:
        line = row["__line__"]
        for column, spec in specs.items():
            value = row[column]
            if spec.required and not value:
                fail(f"{rel(path)}:{line}:{column}: required value is empty")
            if not value:
                continue
            if spec.kind in {"StableId", "BehaviorId", "EffectKind", "FormulaId", "ForeignKey"} and not stable_id(value):
                fail(f"{rel(path)}:{line}:{column}: invalid stable id")
            if spec.kind == "StableIdList":
                for part in value.split("|"):
                    if not stable_id(part):
                        fail(f"{rel(path)}:{line}:{column}: invalid stable id list entry")
            if spec.kind == "Bool" and value not in {"true", "false"}:
                fail(f"{rel(path)}:{line}:{column}: boolean must be lowercase true or false")
            if spec.kind in {"Float", "PercentDecimal", "Int"}:
                parse_float(path, row, column, spec)
            if spec.kind == "Int" and float(value) != round(float(value)):
                fail(f"{rel(path)}:{line}:{column}: value must be an integer")
            if spec.kind == "ValueOp" and value not in VALUE_OPS:
                fail(f"{rel(path)}:{line}:{column}: value operation must be Add, Multiply or Override")
            if spec.kind == "BehaviorId" and value not in REGISTERED_BEHAVIOR_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ behavior id {value!r}")
            if spec.kind == "EffectKind" and value not in REGISTERED_EFFECT_KINDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ effect kind {value!r}")
            if spec.kind == "FormulaId" and value not in REGISTERED_FORMULA_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ formula id {value!r}")
            if spec.kind == "AttackPatternId" and value not in REGISTERED_ATTACK_PATTERN_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ attack pattern id {value!r}")
            if spec.kind == "ForeignKey" and value == "None" and table_id == "Reactions" and column == "StatusId":
                continue
            if spec.kind == "ForeignKey" and value not in references[spec.reference_table or ""]:
                fail(f"{rel(path)}:{line}:{column}: unknown reference {value!r}")

        row_id = row["Id"]
        if row_id in ids:
            fail(f"{rel(path)}:{line}: duplicate id {row_id!r}")
        ids.add(row_id)
    if not ids and table_id == "RuntimeSmoke":
        fail(f"{rel(path)}: RuntimeSmoke must contain at least one row")
    return ids


def validate_csv_package(data_dir: Path) -> None:
    entries = validate_manifest(data_dir)
    references: dict[str, set[str]] = {}
    references["RuntimeSmoke"] = validate_table(entries["RuntimeSmoke"], "RuntimeSmoke", references)
    references["RuntimeSmokeEffects"] = validate_table(entries["RuntimeSmokeEffects"], "RuntimeSmokeEffects", references)
    references["Characters"] = validate_table(entries["Characters"], "Characters", references)
    references["CharacterAliases"] = validate_table(entries["CharacterAliases"], "CharacterAliases", references)
    references["Cards"] = validate_table(entries["Cards"], "Cards", references)
    references["CardEffects"] = validate_table(entries["CardEffects"], "CardEffects", references)
    references["Elements"] = validate_table(entries["Elements"], "Elements", references)
    references["Statuses"] = validate_table(entries["Statuses"], "Statuses", references)
    references["Reactions"] = validate_table(entries["Reactions"], "Reactions", references)
    references["WeaponTypes"] = validate_table(entries["WeaponTypes"], "WeaponTypes", references)
    references["Weapons"] = validate_table(entries["Weapons"], "Weapons", references)
    references["AttackSteps"] = validate_table(entries["AttackSteps"], "AttackSteps", references)
    references["SlotTypes"] = validate_table(entries["SlotTypes"], "SlotTypes", references)
    references["SlotProfiles"] = validate_table(entries["SlotProfiles"], "SlotProfiles", references)
    references["Parts"] = validate_table(entries["Parts"], "Parts", references)
    references["PartEffects"] = validate_table(entries["PartEffects"], "PartEffects", references)
    validate_character_build_domain(data_dir, entries)
    validate_element_reaction_domain(data_dir, entries)
    validate_weapon_domain(data_dir, entries)


def assemble_fixture_package(fixture_dir: Path, temp_root: Path) -> Path:
    assembled = temp_root / fixture_dir.name
    assembled.mkdir(parents=True, exist_ok=True)
    for source in DATA.glob("*.csv"):
        shutil.copy2(source, assembled / source.name)
    for override in fixture_dir.glob("*.csv"):
        shutil.copy2(override, assembled / override.name)
    return assembled


def validate_character_build_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    characters = load_csv(entries["Characters"])
    cards = load_csv(entries["Cards"])
    effects = load_csv(entries["CardEffects"])
    enabled_characters = {row["Id"] for row in characters if row["Enabled"] == "true"}
    if {"J_CAT", "J_SPADE", "J_DIAMOND", "J_CLOVER", "J_HEART"} - enabled_characters:
        fail(f"{rel(entries['Characters'])}: current playable character ids must remain enabled")
    for row in characters:
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Characters'])}:{row['__line__']}: disabled character requires DisabledReason")

    enabled_cards = {row["Id"] for row in cards if row["Enabled"] == "true"}
    offerable_traits = [row["Id"] for row in cards if row["OfferGroup"] == "Trait" and row["Offerable"] == "true"]
    if offerable_traits != ["G_1_01", "G_1_02", "G_1_03", "G_1_04", "G_1_05", "G_1_08"]:
        fail(f"{rel(entries['Cards'])}: current offerable trait pool changed: {offerable_traits}")
    for row in cards:
        if row["Enabled"] == "true" and row["ReviewStatus"] != "Approved":
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: enabled card must be Approved")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: disabled card requires DisabledReason")
        if row["Offerable"] == "true" and row["Enabled"] != "true":
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: offerable card must be enabled")

    effects_by_card: dict[str, list[dict[str, str]]] = {}
    seen_orders: set[tuple[str, str]] = set()
    for row in effects:
        if row["Trigger"] != "OnApply":
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: unsupported card effect trigger {row['Trigger']!r}")
        if row["Target"] not in CARD_TARGETS:
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: unknown card effect target {row['Target']!r}")
        expected_behavior = CARD_EFFECT_BEHAVIOR_PAIRS.get(row["EffectKind"])
        if not expected_behavior or row["BehaviorId"] != expected_behavior:
            fail(
                f"{rel(entries['CardEffects'])}:{row['__line__']}: invalid EffectKind/BehaviorId pair "
                f"{row['EffectKind']!r}/{row['BehaviorId']!r}"
            )
        key = (row["CardId"], row["Order"])
        if key in seen_orders:
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: duplicate CardId/Order {key}")
        seen_orders.add(key)
        effects_by_card.setdefault(row["CardId"], []).append(row)
    for card_id in enabled_cards:
        if card_id not in effects_by_card:
            fail(f"{rel(entries['CardEffects'])}: enabled card {card_id!r} has no effect rows")


def validate_element_reaction_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    elements = load_csv(entries["Elements"])
    statuses = load_csv(entries["Statuses"])
    reactions = load_csv(entries["Reactions"])

    enabled_elements = {row["Id"] for row in elements if row["Enabled"] == "true"}
    if enabled_elements != ELEMENT_IDS:
        fail(f"{rel(entries['Elements'])}: enabled element ids changed: {sorted(enabled_elements)}")
    for row in elements:
        if row["Id"] not in ELEMENT_IDS:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: unsupported element id {row['Id']!r}")
        if row["Role"] not in ELEMENT_ROLES:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: role must be Trigger or Attachment")
        if not row["ColorHex"].startswith("#") or len(row["ColorHex"]) != 7:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: ColorHex must be #RRGGBB")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: disabled element requires DisabledReason")

    status_ids = {row["Id"] for row in statuses}
    if {"Z_Elemental_Immunity", "Z_Burn"} - status_ids:
        fail(f"{rel(entries['Statuses'])}: elemental immunity and burn statuses are required")
    for row in statuses:
        expected_behavior = STATUS_BEHAVIOR_PAIRS.get(row["Id"], "None")
        if row["BehaviorId"] != expected_behavior:
            fail(
                f"{rel(entries['Statuses'])}:{row['__line__']}: invalid StatusId/BehaviorId pair "
                f"{row['Id']!r}/{row['BehaviorId']!r}"
            )
        if row["Enabled"] == "true" and row["BehaviorId"] == "None":
            fail(f"{rel(entries['Statuses'])}:{row['__line__']}: enabled status needs behavior")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Statuses'])}:{row['__line__']}: disabled status requires DisabledReason")

    expected_pairs = {
        ("Flame", "Grass"),
        ("Flame", "Water"),
        ("Lightning", "Grass"),
        ("Lightning", "Water"),
        ("Grass", "Water"),
        ("Water", "Grass"),
    }
    seen_pairs: set[tuple[str, str]] = set()
    enabled_reactions: set[str] = set()
    for row in reactions:
        pair = (row["TriggerElementId"], row["AttachmentElementId"])
        if pair in seen_pairs:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: duplicate ordered pair {pair}")
        seen_pairs.add(pair)
        if row["BehaviorId"] not in REACTION_BEHAVIOR_FORMULA_PAIRS:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: unsupported reaction behavior {row['BehaviorId']!r}")
        expected_formula = REACTION_BEHAVIOR_FORMULA_PAIRS[row["BehaviorId"]]
        if row["FormulaId"] != expected_formula:
            fail(
                f"{rel(entries['Reactions'])}:{row['__line__']}: invalid ReactionBehaviorId/FormulaId pair "
                f"{row['BehaviorId']!r}/{row['FormulaId']!r}"
            )
        if row["StatusId"] != "None" and row["StatusId"] not in status_ids:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: unknown status reference {row['StatusId']!r}")
        if row["Enabled"] == "true":
            enabled_reactions.add(row["Id"])
        if row["CanCrit"] == "true":
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: CanCrit is not supported yet")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: disabled reaction requires DisabledReason")
    if seen_pairs != expected_pairs:
        fail(f"{rel(entries['Reactions'])}: ordered reaction pairs changed: {sorted(seen_pairs)}")
    if len(enabled_reactions) != 6:
        fail(f"{rel(entries['Reactions'])}: expected six enabled reactions, got {sorted(enabled_reactions)}")


def validate_weapon_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    weapon_types = load_csv(entries["WeaponTypes"])
    weapons = load_csv(entries["Weapons"])
    attack_steps = load_csv(entries["AttackSteps"])
    slot_types = load_csv(entries["SlotTypes"])
    slot_profiles = load_csv(entries["SlotProfiles"])
    parts = load_csv(entries["Parts"])
    part_effects = load_csv(entries["PartEffects"])

    enabled_type_ids = {row["Id"] for row in weapon_types if row["Enabled"] == "true"}
    if enabled_type_ids != WEAPON_TYPE_IDS:
        fail(f"{rel(entries['WeaponTypes'])}: enabled weapon type ids changed: {sorted(enabled_type_ids)}")
    for row in weapon_types:
        if row["Id"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['WeaponTypes'])}:{row['__line__']}: unsupported WeaponTypeId {row['Id']!r}")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['WeaponTypes'])}:{row['__line__']}: disabled weapon type requires DisabledReason")

    enabled_weapons = {row["Id"]: row for row in weapons if row["Enabled"] == "true"}
    required_weapons = {"W_J_01", "W_J_02", "W_J_03", "W_J_04"}
    if required_weapons - set(enabled_weapons):
        fail(f"{rel(entries['Weapons'])}: current required weapons must remain enabled")
    start_selectable = sorted(
        [row for row in weapons if row["Enabled"] == "true" and row["StartSelectable"] == "true"],
        key=lambda row: int(row["LoadoutOrder"]),
    )
    if [row["Id"] for row in start_selectable] != ["W_J_02", "W_J_01", "W_J_03"]:
        fail(f"{rel(entries['Weapons'])}: legacy start weapon order changed")
    input_map = {row["InputSlot"]: row["Id"] for row in weapons if row["InputSlot"] != "None" and row["Enabled"] == "true"}
    if input_map != {"1": "W_J_02", "2": "W_J_01", "3": "W_J_03"}:
        fail(f"{rel(entries['Weapons'])}: legacy input slot mapping changed: {input_map}")
    if enabled_weapons["W_J_04"]["WeaponTypeId"] != "Scythe" or enabled_weapons["W_J_04"]["AttackPatternId"] != "Pattern.ScytheSweep":
        fail(f"{rel(entries['Weapons'])}: W_J_04 must use the Scythe pattern")
    reachable_daggers = [
        row
        for row in enabled_weapons.values()
        if row["WeaponTypeId"] == "Dagger" and row["InputSlot"] == "None" and row["StartSelectable"] == "false"
    ]
    if not reachable_daggers:
        fail(f"{rel(entries['Weapons'])}: enabled non-start Dagger weapon is required for Dagger-only parts")
    staff_projectile_weapons = [
        row
        for row in enabled_weapons.values()
        if row["AttackPatternId"] == "Pattern.StaffProjectile" and float(row["ExplosionRadiusCm"]) > 0
    ]
    if not staff_projectile_weapons:
        fail(f"{rel(entries['Weapons'])}: enabled StaffProjectile weapon with ExplosionRadiusCm is required")
    for row in weapons:
        if row["InputSlot"] not in INPUT_SLOTS:
            fail(f"{rel(entries['Weapons'])}:{row['__line__']}: InputSlot must be None, 1, 2 or 3")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Weapons'])}:{row['__line__']}: disabled weapon requires DisabledReason")

    characters = load_csv(entries["Characters"])
    for row in characters:
        if row["Enabled"] == "true" and row["DefaultWeaponId"] not in enabled_weapons:
            fail(
                f"{rel(entries['Characters'])}:{row['__line__']}: DefaultWeaponId "
                f"{row['DefaultWeaponId']!r} does not reference an enabled weapon"
            )

    seen_steps: set[tuple[str, str]] = set()
    patterns_with_steps: set[str] = set()
    for row in attack_steps:
        key = (row["AttackPatternId"], row["StepIndex"])
        if key in seen_steps:
            fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: duplicate AttackPatternId/StepIndex {key}")
        seen_steps.add(key)
        if row["Enabled"] == "true":
            patterns_with_steps.add(row["AttackPatternId"])
        elif not row["DisabledReason"]:
            fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: disabled attack step requires DisabledReason")
    missing_patterns = {row["AttackPatternId"] for row in weapons if row["Enabled"] == "true"} - patterns_with_steps
    if missing_patterns:
        fail(f"{rel(entries['AttackSteps'])}: enabled weapons missing attack steps: {sorted(missing_patterns)}")

    slot_type_ids = {row["Id"] for row in slot_types}
    for row in slot_profiles:
        if row["WeaponTypeId"] != "Any" and row["WeaponTypeId"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: unknown WeaponTypeId {row['WeaponTypeId']!r}")
        if row["SlotTypeId"] not in slot_type_ids:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: unknown SlotTypeId {row['SlotTypeId']!r}")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: disabled slot profile requires DisabledReason")

    if len(parts) != 78:
        fail(f"{rel(entries['Parts'])}: weapon slot audit must contain 78 source rows")
    named_rows = [row for row in parts if row["DisplayName"]]
    unnamed_disabled = [row for row in parts if not row["DisplayName"] and row["Enabled"] == "false" and row["PartId"] == "None"]
    if len(named_rows) != 16 or len(unnamed_disabled) != 62:
        fail(f"{rel(entries['Parts'])}: expected 16 named rows and 62 unnamed disabled audit rows")
    enabled_parts = {row["Id"]: row for row in parts if row["Enabled"] == "true"}
    enabled_cores = [row for row in enabled_parts.values() if row["SlotTypeId"] == "Core"]
    if len(enabled_cores) < 6:
        fail(f"{rel(entries['Parts'])}: at least six generic cores must be enabled")
    if "P_DAGGER_STRENGTH_GRIP" not in enabled_parts:
        fail(f"{rel(entries['Parts'])}: strength grip must be enabled")
    for row in parts:
        if row["WeaponTypeId"] != "Any" and row["WeaponTypeId"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: unknown WeaponTypeId {row['WeaponTypeId']!r}")
        if row["Enabled"] == "true":
            if row["PartId"] == "None" or not row["DisplayName"]:
                fail(f"{rel(entries['Parts'])}:{row['__line__']}: enabled part needs stable PartId and name")
            if row["ReviewStatus"] != "Approved" or row["ImplementationStatus"] != "Implemented":
                fail(f"{rel(entries['Parts'])}:{row['__line__']}: enabled part must be approved and implemented")
        elif not row["DisabledReason"]:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: disabled part requires DisabledReason")

    effects_by_part: dict[str, list[dict[str, str]]] = {}
    seen_effect_orders: set[tuple[str, str]] = set()
    has_pattern_replacement = False
    has_unique_behavior = False
    has_value_ops: set[str] = set()
    for row in part_effects:
        if row["Target"] not in WEAPON_EFFECT_TARGETS:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: unsupported weapon effect target {row['Target']!r}")
        expected_behavior = PART_EFFECT_BEHAVIOR_PAIRS.get(row["EffectKind"])
        if not expected_behavior or row["BehaviorId"] != expected_behavior:
            fail(
                f"{rel(entries['PartEffects'])}:{row['__line__']}: invalid EffectKind/BehaviorId pair "
                f"{row['EffectKind']!r}/{row['BehaviorId']!r}"
            )
        if row["Enabled"] == "true" and row["PartId"] not in enabled_parts:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: enabled effect references disabled part")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: disabled part effect requires DisabledReason")
        key = (row["PartId"], row["Order"])
        if key in seen_effect_orders:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: duplicate PartId/Order {key}")
        seen_effect_orders.add(key)
        effects_by_part.setdefault(row["PartId"], []).append(row)
        if row["Enabled"] == "true":
            has_value_ops.add(row["ValueOp"])
            has_pattern_replacement |= row["EffectKind"] == "AttackPatternReplacement"
            has_unique_behavior |= row["EffectKind"] == "UniqueBehavior"
    for part_id in enabled_parts:
        if part_id not in effects_by_part:
            fail(f"{rel(entries['PartEffects'])}: enabled part {part_id!r} has no effect rows")
    if not has_pattern_replacement:
        fail(f"{rel(entries['PartEffects'])}: at least one enabled AttackPatternReplacement effect is required")
    if not has_unique_behavior:
        fail(f"{rel(entries['PartEffects'])}: at least one enabled UniqueBehavior effect is required")
    if has_value_ops != {"Add", "Multiply", "Override"}:
        fail(f"{rel(entries['PartEffects'])}: enabled part effects must cover Add/Multiply/Override")


def expect_fixture_failure(name: str, token: str) -> None:
    fixture = DATA / "TestFixtures" / "CsvRuntime" / name
    with tempfile.TemporaryDirectory(prefix="reecho_csv_fixture_") as temp:
        assembled = assemble_fixture_package(fixture, Path(temp))
        try:
            validate_csv_package(assembled)
        except ValidationError as exc:
            if token not in str(exc):
                fail(f"{rel(fixture)}: expected {token!r}, got {exc}")
            return
        fail(f"{rel(fixture)}: expected fixture to fail with {token}")


def validate_legacy_json() -> tuple[int, int, int]:
    required = {
        "global_balance.json",
        "characters.json",
        "weapons.json",
        "cards.json",
        "enemies.json",
        "encounters.json",
        "elements.json",
        "reactions.json",
        "statuses.json",
    }
    missing = sorted(name for name in required if not (DATA / name).is_file())
    if missing:
        fail(f"missing legacy JSON files: {', '.join(missing)}")

    documents = {path.name: load_json(path) for path in DATA.glob("*.json")}
    balance = documents["global_balance.json"]
    expected = {"encounterDuration": 30.0, "fixedStepHz": 60.0, "recordingHz": 20.0}
    for key, value in expected.items():
        if balance.get(key) != value:
            fail(f"global_balance.{key} must be {value}, got {balance.get(key)!r}")

    for filename in ("characters.json", "weapons.json", "cards.json", "enemies.json", "elements.json", "reactions.json", "statuses.json"):
        rows = documents[filename]
        ids = [row.get("id") for row in rows]
        if None in ids or len(ids) != len(set(ids)):
            fail(f"{filename} has a missing or duplicate id")

    cards = documents["cards.json"]
    effective_cards = [card for card in cards if not card.get("reserved", False)]
    reserved = {card["id"] for card in cards if card.get("reserved", False)}
    if len(effective_cards) != 27 or reserved != {"G_2_08", "G_2_09"}:
        fail(f"expected 27 effective cards and reserved G_2_08/G_2_09; got {len(effective_cards)} and {sorted(reserved)}")

    if len(documents["characters.json"]) != 4 or len(documents["weapons.json"]) != 4:
        fail("the demo baseline requires exactly four characters and four weapons")
    encounters = documents["encounters.json"]
    if [row.get("index") for row in encounters] != list(range(1, 7)):
        fail("encounters must be indexed consecutively from 1 through 6")
    return len(documents), len(effective_cards), len(encounters)


def validate_workflow() -> None:
    project = load_json(ROOT / "ReEcho.uproject")
    modules = {module.get("Name") for module in project.get("Modules", [])}
    if "ReEcho" not in modules:
        fail("ReEcho.uproject does not declare the ReEcho runtime module")

    required_shared = {
        "AI_ONBOARDING.md",
        "WORKFLOW.md",
        "PROGRAMMER_RULES.md",
        "DESIGNER_RULES.md",
        "ARTIST_RULES.md",
        "SECRETARY_RULES.md",
        "GIT_RULES.md",
        "PLANNER_RULES.md",
        "EXECUTOR_RULES.md",
        "LESSONS.md",
        "PROJECT_RULES.md",
        "PLANNER_EXCHANGE.md",
        "ARCHITECTURE.md",
        "CODEBASE_MAP.md",
    }
    absent_shared = sorted(name for name in required_shared if not (ROOT / "shared" / name).is_file())
    if absent_shared:
        fail(f"workflow deployment incomplete: {', '.join(absent_shared)}")
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
    exchange_text = (ROOT / "shared" / "PLANNER_EXCHANGE.md").read_text(encoding="utf-8")
    planner_rules = (ROOT / "shared" / "PLANNER_RULES.md").read_text(encoding="utf-8")
    executor_rules = (ROOT / "shared" / "EXECUTOR_RULES.md").read_text(encoding="utf-8")
    project_rules = (ROOT / "shared" / "PROJECT_RULES.md").read_text(encoding="utf-8")
    programmer_rules = (ROOT / "shared" / "PROGRAMMER_RULES.md").read_text(encoding="utf-8")
    designer_rules = (ROOT / "shared" / "DESIGNER_RULES.md").read_text(encoding="utf-8")
    artist_rules = (ROOT / "shared" / "ARTIST_RULES.md").read_text(encoding="utf-8")
    secretary_rules = (ROOT / "shared" / "SECRETARY_RULES.md").read_text(encoding="utf-8")
    git_rules = (ROOT / "shared" / "GIT_RULES.md").read_text(encoding="utf-8")
    workflow_text = (ROOT / "shared" / "WORKFLOW.md").read_text(encoding="utf-8")
    onboarding_text = (ROOT / "shared" / "AI_ONBOARDING.md").read_text(encoding="utf-8")
    plan_template = (ROOT / "plans" / "TEMPLATE.md").read_text(encoding="utf-8")
    readme_text = (ROOT / "README.md").read_text(encoding="utf-8")
    docs_workflow_text = (ROOT / "docs" / "AI_WORKFLOW.md").read_text(encoding="utf-8")
    architecture_root = ROOT / "shared" / "CODEBASE_MAP"
    architecture_path = architecture_root / "ARCHITECTURE.md"
    codebase_index_path = architecture_root / "README.md"
    module_template_path = architecture_root / "MODULE_TEMPLATE.md"
    for required_path in (architecture_path, codebase_index_path, module_template_path):
        if not required_path.is_file():
            fail(f"architecture library incomplete: {rel(required_path)}")
    architecture_text = architecture_path.read_text(encoding="utf-8")
    codebase_index_text = codebase_index_path.read_text(encoding="utf-8")
    module_template_text = module_template_path.read_text(encoding="utf-8")

    if "only mandatory reading-order authority" not in agents:
        fail("AGENTS.md must remain the sole startup-order authority")
    role_gate_markers = (
        "## First-contact professional-role gate",
        "你在本项目中的角色是程序、策划还是美术？",
        "Do not infer the role",
        "shared/PROGRAMMER_RULES.md",
        "shared/DESIGNER_RULES.md",
        "shared/ARTIST_RULES.md",
        "是否采用规划者-执行者模式？",
    )
    missing_role_gate_markers = [marker for marker in role_gate_markers if marker not in agents]
    if missing_role_gate_markers:
        fail(f"AGENTS.md lacks the first-contact professional-role gate: {', '.join(missing_role_gate_markers)}")
    remote_rule_authority_markers = (
        "## 远端规则权威与权限升级",
        "最新远端权威规则高于最初 prompt",
        "向**程序侧项目秘书**请求确认",
        "不得自行猜测权限",
        "不替代程序、策划、美术或用户作产品取舍",
    )
    missing_remote_rule_authority_markers = [
        marker for marker in remote_rule_authority_markers if marker not in project_rules
    ]
    if missing_remote_rule_authority_markers:
        fail(
            "PROJECT_RULES.md lacks remote-rule authority or permission escalation: "
            + ", ".join(missing_remote_rule_authority_markers)
        )
    if "remote-rule authority and permission-escalation gate in `shared/PROJECT_RULES.md`" not in agents:
        fail("AGENTS.md must route prompt/rule conflicts to PROJECT_RULES.md")
    risk_authority_markers = (
        "## 风险操作的人类确认与执行",
        "风险操作授权的唯一权威",
        "覆盖或丢弃未提交内容",
        "跳过规则或 Plan 要求的构建、测试、审查或发布门禁",
        "建议先问程序",
        "确认后应直接执行",
        "经人工确认跳过/未验证",
        "不得写成通过",
        "人工确认不能授权伪造证据",
    )
    missing_risk_authority_markers = [marker for marker in risk_authority_markers if marker not in project_rules]
    if missing_risk_authority_markers:
        fail(
            "PROJECT_RULES.md lacks human-confirmed risk execution markers: "
            + ", ".join(missing_risk_authority_markers)
        )
    if "risky-operation prohibition or exception is governed" not in agents:
        fail("AGENTS.md must route every risky-operation rule to PROJECT_RULES.md")
    role_rule_markers = {
        "PROGRAMMER_RULES.md": (
            "程序用户路线",
            "Planner 负责规划",
            "Executor 负责在指定本地 Plan/分支上具体实现",
            "## Planner-Executor 模式",
            "选择 Planner-Executor 模式的唯一权威",
            "远端协作安全检查",
            "shared/GIT_RULES.md",
        ),
        "DESIGNER_RULES.md": ("策划用户路线", "ReEchoData.xlsx", "禁止手改生成的", "shared/GIT_RULES.md"),
        "ARTIST_RULES.md": ("美术用户路线", "Active Exclusive", "禁止手改 `.uasset`", "shared/GIT_RULES.md"),
    }
    role_rule_texts = {
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
    }
    for name, markers in role_rule_markers.items():
        missing = [marker for marker in markers if marker not in role_rule_texts[name]]
        if missing:
            fail(f"{name} lacks professional-route boundaries: {', '.join(missing)}")
    risk_route_marker = "风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md`"
    risk_route_texts = {
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "SECRETARY_RULES.md": secretary_rules,
        "GIT_RULES.md": git_rules,
    }
    missing_risk_routes = [name for name, text_value in risk_route_texts.items() if risk_route_marker not in text_value]
    if missing_risk_routes:
        fail("rule files must route risky-operation confirmation to PROJECT_RULES.md: " + ", ".join(missing_risk_routes))
    secretary_markers = (
        "项目秘书仓库职责的唯一权威",
        "不是第四种用户专业角色",
        "可维护全仓库规则",
        "## 秘书任务无需 Plan",
        "不需要 Plan、Plan 编号、Plan 文件",
        "秘书不为自己创建行",
        "物理/Git 冲突、逻辑冲突和耦合",
        "负责受理各角色 AI 提交的仓库规则冲突、职责路由和操作权限咨询",
        "推送 `origin/main`",
        "遵循 `shared/GIT_RULES.md` 中集中定义的身份和边界规则",
    )
    missing_secretary_markers = [marker for marker in secretary_markers if marker not in secretary_rules]
    if missing_secretary_markers:
        fail(f"SECRETARY_RULES.md lacks secretary authority markers: {', '.join(missing_secretary_markers)}")
    if "shared/SECRETARY_RULES.md" not in agents or "Project Secretary duty is a repository coordination overlay" not in agents:
        fail("AGENTS.md must route explicit Project Secretary duty to SECRETARY_RULES.md")
    if "项目秘书边界位于 `SECRETARY_RULES.md`" not in project_rules:
        fail("PROJECT_RULES.md must point to SECRETARY_RULES.md without redefining secretary duty")
    git_rule_markers = (
        "提交身份和发布完整性规则的唯一权威",
        "[PROGRAMMER]",
        "[DESIGNER]",
        "[ARTIST]",
        "[SECRETARY]",
        "普通程序路线每次推送 `origin/main` 前",
        "Build-Editor.cmd -Configuration Development -FullRebuild",
        "ReEchoEditor.prebuilt.json",
        "仅含源码的程序候选",
        "经人工确认跳过/未验证",
        "建议先问程序",
    )
    missing_git_rule_markers = [marker for marker in git_rule_markers if marker not in git_rules]
    if missing_git_rule_markers:
        fail(f"GIT_RULES.md lacks commit/publication authority markers: {', '.join(missing_git_rule_markers)}")
    if "shared/GIT_RULES.md" not in agents or "Commit or publication work" not in agents:
        fail("AGENTS.md must route commit and publication work to GIT_RULES.md")
    if "普通任务直接遵循 `AGENTS.md`" not in onboarding_text:
        fail("AI_ONBOARDING.md must not redefine ordinary-task startup order")
    if "所有风险操作遵循 `shared/PROJECT_RULES.md` 的统一确认机制" not in onboarding_text:
        fail("AI_ONBOARDING.md must route risky operations to PROJECT_RULES.md")
    if "## 最近关闭" in exchange_text or "## 决定" in exchange_text:
        fail("PLANNER_EXCHANGE.md must contain live coordination only, not history or permanent rules")
    announcement_block = exchange_text.split("## 已规划和活跃工作公告", 1)[1].split("## 活跃所有权", 1)[0]
    for row in announcement_block.splitlines():
        if not row.startswith("|") or row.startswith("|---") or "| Plan |" in row:
            continue
        cells = [cell.strip().strip("`") for cell in row.strip("|").split("|")]
        if len(cells) != 8:
            fail("PLANNER_EXCHANGE.md announcement rows must use the canonical eight-column schema")
        if not cells[1] or "/" not in cells[1]:
            fail("PLANNER_EXCHANGE.md announcement rows must identify Plan and implementation AI sides")
        if cells[3] not in {"Proposed", "Ready", "InProgress", "Review", "Closed", "Blocked"}:
            fail(f"PLANNER_EXCHANGE.md uses unknown task status: {cells[3]}")
        if cells[4] not in {"NotRequired", "PendingBeforeClose", "PendingFollowUp", "Passed"}:
            fail(f"PLANNER_EXCHANGE.md uses unknown human-validation state: {cells[4]}")
        if cells[3] == "Closed" and cells[4] != "PendingFollowUp":
            fail("closed Exchange rows are retained only for a live PendingFollowUp; otherwise remove them")
    active_block = exchange_text.split("## 活跃所有权", 1)[1].split("## 警告 / 阻塞项", 1)[0]
    if "plan/07" in active_block.lower() or "plan/08" in active_block.lower():
        fail("completed Plans 07/08 must not retain active ownership")
    for row in active_block.splitlines():
        if not row.startswith("|") or row.startswith("|---") or "| 所有者 |" in row:
            continue
        cells = [cell.strip() for cell in row.strip("|").split("|")]
        if len(cells) != 6:
            fail("PLANNER_EXCHANGE.md ownership rows must use the canonical six-column schema")
        modes = set(re.findall(r"`(Isolated|ReadOnly|SharedContract|Exclusive)`", cells[2]))
        if not modes:
            fail(f"PLANNER_EXCHANGE.md ownership row has no recognized impact mode: {cells[2]}")
        if cells[3].strip("`") not in {"Reserved", "Active", "Released"}:
            fail(f"PLANNER_EXCHANGE.md uses unknown ownership state: {cells[3]}")
        if cells[3].strip("`") == "Released":
            fail("released ownership rows must be removed from the live Exchange")
    for name, text in {
        "WORKFLOW.md": workflow_text,
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
    }.items():
        if "<待定>" in text or "<RESOURCE_LOCK>" in text:
            fail(f"{name} still contains a generic workflow placeholder")
    for stale_token in ("ReadyForHandoff", "InProgress/Rework", "origin/main` at `df6e60a"):
        if stale_token in exchange_text:
            fail(f"live workflow memory contains stale token: {stale_token}")
    required_template_fields = (
        "## 协调",
        "Plan 编写方（AI 侧）：",
        "实现编写方（AI 侧）：",
        "任务状态：",
        "人工验收：",
        "本地规划 / 实现基线：",
        "影响模式：",
        "Writes:",
        "Stable Reads:",
        "兼容承诺 / 下游操作：",
        "明确排除：",
    )
    missing_template_fields = [field for field in required_template_fields if field not in plan_template]
    if missing_template_fields:
        fail(f"Plan template lacks local coordination fields: {', '.join(missing_template_fields)}")
    if "git rev-parse --path-format=absolute --git-common-dir" not in executor_rules:
        fail("Executor lock guidance must use the Git common directory shared by worktrees")
    if "Executor 更新其指定 Plan 的执行记录" not in project_rules:
        fail("project rules must keep routine shared-state writes out of Executor branches")
    if "不是第二本规则书" not in workflow_text:
        fail("WORKFLOW.md must remain explanatory rather than a duplicate rulebook")
    integration_audit_markers = (
        "## 外部提交集成审计",
        "物理/Git 冲突",
        "逻辑冲突",
        "耦合",
        "即使 Git 可以快进",
        "推送前立即再次 fetch",
    )
    missing_audit_markers = [marker for marker in integration_audit_markers if marker not in planner_rules]
    if missing_audit_markers:
        fail(f"Planner rules lack the external-commit integration audit gate: {', '.join(missing_audit_markers)}")
    plan_publication_markers = (
        "## 执行前编号并发布 Plan",
        "分配 Plan 编号前",
        "识别最大 Plan 编号",
        "立即将编号 Plan 发布到 `origin/main`",
        "不得为该 Plan 启动 Executor、实现分支或准备发布的专业工作",
        "若推送被拒或其他已发布 Plan 占用编号",
        "新编号 Plan 文件及其匹配实时 Exchange",
    )
    missing_plan_publication_markers = [marker for marker in plan_publication_markers if marker not in planner_rules]
    if missing_plan_publication_markers:
        fail(f"Planner rules lack remote-first Plan numbering/publication: {', '.join(missing_plan_publication_markers)}")
    plan_language_markers = (
        "## Plan 语言",
        "新建 Plan 及对现有 Plan 的实质性内容更新必须使用中文正文",
        "不得仅为翻译而批量重写已关闭历史 Plan",
    )
    missing_plan_language_markers = [marker for marker in plan_language_markers if marker not in planner_rules]
    if missing_plan_language_markers:
        fail(f"Planner rules lack the Chinese Plan language contract: {', '.join(missing_plan_language_markers)}")
    if "# Plan XX - <专业> - <简短名称>" not in plan_template or "## 锁定目标" not in plan_template:
        fail("Plan template must provide the Chinese authoring surface")
    if "每个正式编号 Plan 都依据 `PLANNER_RULES.md` 发布到 `origin/main`" not in project_rules:
        fail("PROJECT_RULES.md must route numbered Plans through the remote-first Planner rule")
    if "编号 Plan 在实现开始前发布到 `main`" not in workflow_text:
        fail("WORKFLOW.md must explain remote-first numbered Plan publication")
    compact_prompt_markers = (
        "启动提示只是中立路由外壳",
        "不得赋予新身份",
        "不改变你现有的身份或职责",
        "不得重复 Plan 的锁定目标",
    )
    missing_compact_prompt_markers = [marker for marker in compact_prompt_markers if marker not in planner_rules]
    if missing_compact_prompt_markers:
        fail(f"Planner rules lack the neutral Plan-driven prompt contract: {', '.join(missing_compact_prompt_markers)}")
    main_only_markers = {
        "PROJECT_RULES.md": ("`origin/main` 是唯一允许的远端分支", "不推送任何远端引用"),
        "PLANNER_RULES.md": ("`origin/main` 是唯一允许的远端分支", "Plan 编号冲突"),
        "EXECUTOR_RULES.md": ("`origin/main` 是唯一远端分支", "不得推送任务分支"),
        "SECRETARY_RULES.md": ("默认禁止创建或推送 `origin/main` 之外的远端分支", "默认只将本地 `main` 非强制推送至 `origin/main`"),
        "WORKFLOW.md": ("远端仓库只有一个分支：`main`", "编号 Plan 在实现开始前发布到 `main`"),
    }
    main_only_texts = {
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "SECRETARY_RULES.md": secretary_rules,
        "WORKFLOW.md": workflow_text,
    }
    for name, markers in main_only_markers.items():
        missing = [marker for marker in markers if marker not in main_only_texts[name]]
        if missing:
            fail(f"{name} lacks main-only remote workflow markers: {', '.join(missing)}")
    live_remote_side_ref_files = {
        "AGENTS.md": agents,
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "WORKFLOW.md": workflow_text,
        "PLANNER_EXCHANGE.md": exchange_text,
        "plans/TEMPLATE.md": plan_template,
    }
    for name, text_value in live_remote_side_ref_files.items():
        if "coord/" in text_value or "Planning broadcast" in text_value or "push only the task branch" in text_value:
            fail(f"{name} still advertises a remote planning/task side-ref workflow")
    non_authoritative_tag_sources = {
        "AGENTS.md": agents,
        "PROJECT_RULES.md": project_rules,
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "WORKFLOW.md": workflow_text,
        "AI_ONBOARDING.md": onboarding_text,
        "README.md": readme_text,
        "docs/AI_WORKFLOW.md": docs_workflow_text,
    }
    commit_tags = ("[PROGRAMMER]", "[DESIGNER]", "[ARTIST]", "[SECRETARY]")
    duplicate_tag_sources = [
        name
        for name, text_value in non_authoritative_tag_sources.items()
        if any(tag in text_value for tag in commit_tags)
    ]
    if duplicate_tag_sources:
        fail(f"AI commit-tag rules must live only in GIT_RULES.md, duplicated in: {', '.join(duplicate_tag_sources)}")
    if "Design/Data/ReEchoData.xlsx" not in readme_text or "generated into validated CSV" not in readme_text:
        fail("README.md must describe the current XLSX-to-CSV authority")
    if "Project Secretary route" not in readme_text or "SECRETARY_RULES.md" not in readme_text:
        fail("README.md must index the Project Secretary route")
    if "not a workflow authority" not in docs_workflow_text or "`shared/` is the collaboration control plane" not in docs_workflow_text:
        fail("docs/AI_WORKFLOW.md must remain a human pointer to the shared authority")
    if "Project Secretary coordination duty" not in docs_workflow_text or "SECRETARY_RULES.md" not in docs_workflow_text:
        fail("docs/AI_WORKFLOW.md must point to the Project Secretary authority")
    architecture_markers = (
        "## 当前 Runtime Module",
        "## 跨模块状态流",
        "## 跨模块不变量",
        "## 当前候选状态",
        "Design/Data/ReEchoData.xlsx",
        "旧 JSON 仅用于迁移",
    )
    missing_architecture_markers = [marker for marker in architecture_markers if marker not in architecture_text]
    if missing_architecture_markers:
        fail(f"shared/CODEBASE_MAP/ARCHITECTURE.md is stale: {', '.join(missing_architecture_markers)}")
    index_markers = (
        "## Runtime Module 索引",
        "## 模块与内部领域索引",
        "## 文档职责",
        "## 同步规则",
        "MODULE_TEMPLATE.md",
    )
    missing_index_markers = [marker for marker in index_markers if marker not in codebase_index_text]
    if missing_index_markers:
        fail(f"shared/CODEBASE_MAP/README.md is stale: {', '.join(missing_index_markers)}")
    required_module_sections = (
        "## 模块状态",
        "## 存在原因",
        "## 职责与排除项",
        "## 权威状态",
        "## 输入、输出与公共契约",
        "## 依赖方向",
        "## 运行时流程",
        "## 代码位置与阅读路线",
        "## 扩展",
        "## 验证",
        "## 不变量与常见错误",
    )
    missing_template_sections = [section for section in required_module_sections if section not in module_template_text]
    if missing_template_sections:
        fail(f"shared/CODEBASE_MAP/MODULE_TEMPLATE.md lacks module sections: {', '.join(missing_template_sections)}")
    runtime_modules = sorted(
        module.get("Name")
        for module in project.get("Modules", [])
        if module.get("Type") == "Runtime" and module.get("Name")
    )
    module_document_texts = {}
    for module_name in runtime_modules:
        architecture_id = f"MOD-{module_name}"
        if architecture_id not in architecture_text:
            fail(f"shared/CODEBASE_MAP/ARCHITECTURE.md lacks runtime module id: {architecture_id}")
        if architecture_id not in codebase_index_text:
            fail(f"shared/CODEBASE_MAP/README.md lacks runtime module index: {architecture_id}")
        module_document_path = architecture_root / "modules" / f"{architecture_id}.md"
        if not module_document_path.is_file():
            fail(f"runtime module lacks detailed design document: {rel(module_document_path)}")
        module_document_text = module_document_path.read_text(encoding="utf-8")
        module_document_texts[architecture_id] = module_document_text
        if architecture_id not in module_document_text or f"Source/{module_name}/" not in module_document_text:
            fail(f"{rel(module_document_path)} lacks its id or code location")
        missing_sections = [section for section in required_module_sections if section not in module_document_text]
        if missing_sections:
            fail(f"{rel(module_document_path)} lacks detailed design sections: {', '.join(missing_sections)}")
    architecture_area_ids = (
        "AREA-Core",
        "AREA-Data",
        "AREA-AbilityCombat",
        "AREA-Weapons",
        "AREA-Enemies",
        "AREA-Encounter",
        "AREA-Run",
        "AREA-Recording",
        "AREA-Player",
        "AREA-Presentation",
        "AREA-UI",
        "AREA-Tests",
    )
    expected_module_ids = {f"MOD-{module_name}" for module_name in runtime_modules}
    architecture_module_ids = set(re.findall(r"`(MOD-[A-Za-z][A-Za-z0-9]*)`", architecture_text))
    if architecture_module_ids != expected_module_ids:
        fail(
            "runtime descriptor and architecture module ids differ: "
            f"descriptor={sorted(expected_module_ids)}, architecture={sorted(architecture_module_ids)}"
        )
    indexed_ids = set(re.findall(r"`((?:MOD|AREA)-[A-Za-z][A-Za-z0-9]*)`", codebase_index_text))
    documented_ids = set()
    for module_document_text in module_document_texts.values():
        documented_ids.update(re.findall(r"`((?:MOD|AREA)-[A-Za-z][A-Za-z0-9]*)`", module_document_text))
    for architecture_id in architecture_area_ids:
        if architecture_id not in codebase_index_text:
            fail(f"shared/CODEBASE_MAP/README.md lacks internal area index: {architecture_id}")
        if architecture_id not in documented_ids:
            fail(f"module design documents lack internal area design: {architecture_id}")
    if indexed_ids != documented_ids:
        fail(
            "architecture index and module documents use different stable ids: "
            f"index_only={sorted(indexed_ids - documented_ids)}, docs_only={sorted(documented_ids - indexed_ids)}"
        )
    architecture_maintenance_markers = (
        "## 架构影响与设计决策",
        "受影响架构标识",
        "对应模块文档",
        "相关文档同步范围",
        "`<文档>` 已更新",
        "`<文档>` 已审阅、无需修改",
        "### 架构文档审阅结果",
    )
    missing_architecture_maintenance_markers = [
        marker for marker in architecture_maintenance_markers if marker not in plan_template
    ]
    if missing_architecture_maintenance_markers:
        fail(
            "plans/TEMPLATE.md lacks architecture decision/closure records: "
            + ", ".join(missing_architecture_maintenance_markers)
        )
    if "未记录完整审阅结果不得关闭 Plan" not in project_rules:
        fail("PROJECT_RULES.md must gate Plan closure on architecture review")
    module_document_gate_markers = {
        "PROJECT_RULES.md": (
            "每个程序任务在实现前必须把受影响的 `MOD-*` / `AREA-*`",
            "程序修改任一 Runtime Module 时",
            "直接修改模块及受契约影响模块",
        ),
        "PROGRAMMER_RULES.md": ("修改任何 Runtime Module 前", "modules/MOD-*.md"),
        "PLANNER_RULES.md": (
            "程序 Plan 在发布前必须完成“架构影响与设计决策”",
            "每份相关文档必须在 Plan 记录",
        ),
        "EXECUTOR_RULES.md": ("每个被修改 Runtime Module 的 `modules/MOD-*.md` 位于 Writes",),
        "CODEBASE_MAP/README.md": ("每个程序 Plan 在实施前必须列出受影响的 `MOD-*` / `AREA-*`",),
    }
    module_document_gate_texts = {
        "PROJECT_RULES.md": project_rules,
        "PROGRAMMER_RULES.md": programmer_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "CODEBASE_MAP/README.md": codebase_index_text,
    }
    for name, markers in module_document_gate_markers.items():
        missing = [marker for marker in markers if marker not in module_document_gate_texts[name]]
        if missing:
            fail(f"{name} lacks programmer module-document gates: {', '.join(missing)}")
    if "不得设为 `Closed`" not in planner_rules:
        fail("PLANNER_RULES.md must enforce architecture review during Plan closure")


def validate_build_dependencies() -> None:
    build_cs = (ROOT / "Source" / "ReEcho" / "ReEcho.Build.cs").read_text(encoding="utf-8")
    if '"ReEchoEnemies"' not in build_cs:
        fail("ReEcho host module must depend on ReEchoEnemies for EnemyHost composition")
    for file_name in (
        "reecho_data_manifest.csv",
        "csv_schema.csv",
        "runtime_smoke.csv",
        "runtime_smoke_effects.csv",
        "characters.csv",
        "character_aliases.csv",
        "cards.csv",
        "card_effects.csv",
        "elements.csv",
        "statuses.csv",
        "reactions.csv",
        "weapon_types.csv",
        "weapons.csv",
        "attack_steps.csv",
        "slot_types.csv",
        "slot_profiles.csv",
        "parts.csv",
        "part_effects.csv",
    ):
        if f"Content/Data/{file_name}" not in build_cs:
            fail(f"ReEcho.Build.cs does not stage production CSV {file_name}")


def validate_combat_module_boundaries() -> None:
    combat_root = ROOT / "Source" / "ReEchoCombat"
    build_path = combat_root / "ReEchoCombat.Build.cs"
    if not build_path.is_file():
        fail("missing ReEchoCombat runtime module")
    build_text = build_path.read_text(encoding="utf-8")
    if '"ReEcho"' in build_text or '"ReEchoAudio"' in build_text:
        fail("ReEchoCombat must not depend on the main or audio runtime modules")

    forbidden_include_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
        "Weapons/",
    )
    include_pattern = re.compile(r'^\s*#include\s+[<\"]([^>\"]+)[>\"]', re.MULTILINE)
    for path in combat_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(forbidden_include_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/audio module include: {include}")

    descriptor = load_json(ROOT / "ReEcho.uproject")
    combat_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoCombat"]
    if len(combat_modules) != 1 or combat_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoCombat Runtime module")

    weapons_root = ROOT / "Source" / "ReEchoWeapons"
    weapons_build_path = weapons_root / "ReEchoWeapons.Build.cs"
    if not weapons_build_path.is_file():
        fail("missing ReEchoWeapons runtime module")
    weapons_build_text = weapons_build_path.read_text(encoding="utf-8")
    if '"ReEchoCombat"' not in weapons_build_text:
        fail("ReEchoWeapons must depend on the public ReEchoCombat contracts")
    if '"ReEcho"' in weapons_build_text or '"ReEchoAudio"' in weapons_build_text:
        fail("ReEchoWeapons must not depend on the main or audio runtime modules")

    weapons_forbidden_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
    )
    weapons_forbidden_logic_tokens = (
        "ApplyFinalDamage",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "ReEchoGameplayEffects::ApplyDamage",
        "AReEchoEnemyActor",
        "AReEchoPlayerPawn",
    )
    for path in weapons_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(weapons_forbidden_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/audio module include: {include}")
        for token in weapons_forbidden_logic_tokens:
            if token in text_value:
                fail(f"{rel(path)} bypasses Combat adjudication with forbidden token: {token}")

    weapons_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoWeapons"]
    if len(weapons_modules) != 1 or weapons_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoWeapons Runtime module")

    enemies_root = ROOT / "Source" / "ReEchoEnemies"
    enemies_build_path = enemies_root / "ReEchoEnemies.Build.cs"
    if not enemies_build_path.is_file():
        fail("missing ReEchoEnemies runtime module")
    enemies_build_text = enemies_build_path.read_text(encoding="utf-8")
    if '"ReEchoCombat"' not in enemies_build_text:
        fail("ReEchoEnemies must depend on the public ReEchoCombat contracts")
    for forbidden_module in ("ReEcho", "ReEchoWeapons", "ReEchoAudio", "UMG", "Paper2D"):
        if f'"{forbidden_module}"' in enemies_build_text:
            fail(f"ReEchoEnemies must not depend on {forbidden_module}")

    enemies_forbidden_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
        "Weapons/",
    )
    enemies_forbidden_logic_tokens = (
        "FindComponentByClass",
        "TActorIterator",
        "AReEchoGameMode",
        "AReEchoEnemyActor",
        "AReEchoPlayerPawn",
        "ApplyFinalDamage",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "ReEchoGameplayEffects::ApplyDamage",
        "/Game/",
    )
    for path in enemies_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(enemies_forbidden_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/presentation module include: {include}")
        for token in enemies_forbidden_logic_tokens:
            if token in text_value:
                fail(f"{rel(path)} violates the explicit Enemy logic boundary with token: {token}")

    enemies_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoEnemies"]
    if len(enemies_modules) != 1 or enemies_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoEnemies Runtime module")

    enemy_host_header_path = ROOT / "Source" / "ReEcho" / "Public" / "Graybox" / "ReEchoEnemyActor.h"
    enemy_host_cpp_path = ROOT / "Source" / "ReEcho" / "Private" / "Graybox" / "ReEchoEnemyActor.cpp"
    enemy_host_header = enemy_host_header_path.read_text(encoding="utf-8")
    enemy_host_cpp = enemy_host_cpp_path.read_text(encoding="utf-8")
    legacy_enemy_host_state = (
        "float MoveSpeed =",
        "float ContactDamage =",
        "float AttackInterval =",
        "float AttackCooldown =",
        "float FuseRemaining =",
        "bool bBomberFuseActive",
        "float HitReactionRemaining =",
        "FVector KnockbackVelocity =",
        "float AttackVisualRemaining =",
        "float DeathVisualRemaining =",
        "int64 AttackSequence =",
    )
    for token in legacy_enemy_host_state:
        if token in enemy_host_header:
            fail(f"{rel(enemy_host_header_path)} retains duplicate Enemy logic/presentation state: {token}")
    if '"/Game/' in enemy_host_cpp:
        fail(f"{rel(enemy_host_cpp_path)} must not own enemy presentation asset paths")

    enemy_presentation_root = ROOT / "Source" / "ReEcho" / "Private" / "Presentation" / "Enemy"
    presentation_gameplay_tokens = (
        "ReEchoHitResolver",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "NotifyHurt(",
        "NotifyDeath(",
        "RestoreSnapshot(",
        "AddActorWorldOffset",
        "SetActorLocation",
        "SetActorEnableCollision",
        "SetLifeSpan",
    )
    for path in enemy_presentation_root.rglob("*.cpp"):
        text_value = path.read_text(encoding="utf-8")
        for token in presentation_gameplay_tokens:
            if token in text_value:
                fail(f"{rel(path)} writes gameplay from enemy presentation with token: {token}")

    game_mode_path = ROOT / "Source" / "ReEcho" / "Private" / "ReEchoGameMode.cpp"
    if "TActorIterator<AReEchoEnemyActor>" in game_mode_path.read_text(encoding="utf-8"):
        fail("AReEchoGameMode must use the single EnemyRoster instead of scanning concrete enemy actors")


def validate_prebuilt_editor() -> None:
    tool = ROOT / "scripts" / "ue" / "prebuilt_editor.py"
    if not tool.is_file():
        fail("missing prebuilt Editor bundle tool")
    build_script = ROOT / "scripts" / "ue" / "Build-Editor.ps1"
    build_text = build_script.read_text(encoding="utf-8") if build_script.is_file() else ""
    if "[switch]$FullRebuild" not in build_text or "$buildArguments += '-Rebuild'" not in build_text:
        fail("Build-Editor.ps1 must expose the full-rebuild publication gate")
    result = subprocess.run(
        [sys.executable, str(tool), "check"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        fail("prebuilt Editor bundle check failed:\n" + result.stdout.strip())

    manifest_path = ROOT / "Binaries" / "Win64" / "ReEchoEditor.prebuilt.json"
    manifest = load_json(manifest_path)
    allowed = {
        "Binaries/Win64/ReEchoEditor.prebuilt.json",
        *(f"Binaries/Win64/{name}" for name in manifest.get("binaries", {})),
    }
    tracked_result = subprocess.run(
        ["git", "ls-files", "--cached", "Binaries"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if tracked_result.returncode == 0:
        tracked = {line.strip().replace("\\", "/") for line in tracked_result.stdout.splitlines() if line.strip()}
        unexpected = sorted(tracked - allowed)
        missing = sorted(allowed - tracked)
        if unexpected:
            fail(f"tracked generated products exceed prebuilt allowlist: {', '.join(unexpected)}")
        if missing:
            fail(f"prebuilt Editor files are not staged/tracked: {', '.join(missing)}")


def validate_xlsx_authoring_sync() -> None:
    sync_script = ROOT / "scripts" / "data" / "sync_xlsx_to_csv.py"
    if not sync_script.is_file():
        fail("missing XLSX authoring sync script")
    result = subprocess.run(
        [sys.executable, str(sync_script), "--check"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        fail("XLSX authoring sync check failed:\n" + result.stdout.strip())


def main() -> int:
    json_count, effective_cards, encounter_count = validate_legacy_json()
    validate_csv_schema()
    validate_csv_package(DATA)
    with tempfile.TemporaryDirectory(prefix="reecho_csv_valid_alt_") as temp:
        validate_csv_package(assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "ValidAlt", Path(temp)))
    with tempfile.TemporaryDirectory(prefix="reecho_csv_reaction_changed_") as temp:
        validate_csv_package(
            assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "ReactionValueChanged", Path(temp))
        )
    with tempfile.TemporaryDirectory(prefix="reecho_csv_echo_efficiency_") as temp:
        validate_csv_package(
            assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "EchoEfficiencyEnabled", Path(temp))
        )
    for name, token in {
        "DuplicateId": "duplicate id",
        "MissingRequired": "required value",
        "UnknownReference": "unknown reference",
        "UnknownWeaponReference": "DefaultWeaponId",
        "UnknownBehavior": "behavior id",
        "UnknownEffectKind": "effect kind",
        "IllegalRange": "exceeds",
        "UnsupportedVersion": "SchemaVersion",
        "DuplicateCardEffectOrder": "duplicate CardId/Order",
        "UnsupportedCardEffectTrigger": "Trigger",
        "UnknownCardEffectTarget": "Target",
        "InvalidCardEffectBehaviorPair": "EffectKind/BehaviorId",
        "UnknownFormulaId": "FormulaId",
        "DuplicateReactionPair": "duplicate ordered pair",
        "UnsupportedCanCrit": "CanCrit",
        "DuplicateWeaponInputSlot": "legacy input slot mapping",
        "InvalidWeaponPartEffectBehaviorPair": "EffectKind/BehaviorId",
    }.items():
        expect_fixture_failure(name, token)
    validate_build_dependencies()
    validate_combat_module_boundaries()
    validate_prebuilt_editor()
    validate_xlsx_authoring_sync()
    validate_workflow()

    print(f"[PASS] legacy migration-only JSON files={json_count} effective_cards={effective_cards} encounters={encounter_count}")
    print("[PASS] CSV schema, production character/build/element/weapon tables, fixtures, IDs, references, behavior/effect/formula/attack-pattern allowlists, UTF-8 and staging deps")
    print("[PASS] XLSX authoring workbook check matches generated production CSV bytes")
    print("[PASS] workflow memory, token guards, and Unreal project descriptor present")
    print("Evidence level: static verified only (no UHT/UBT/PIE claim)")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValidationError as exc:
        print(f"[FAIL] {exc}")
        sys.exit(1)
