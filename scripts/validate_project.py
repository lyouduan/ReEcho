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

    required_shared = {"AI_ONBOARDING.md", "WORKFLOW.md", "PLANNER_RULES.md", "EXECUTOR_RULES.md", "LESSONS.md", "PROJECT_RULES.md", "PROJECT_STATE.md", "PLANNER_EXCHANGE.md"}
    absent_shared = sorted(name for name in required_shared if not (ROOT / "shared" / name).is_file())
    if absent_shared:
        fail(f"workflow deployment incomplete: {', '.join(absent_shared)}")
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
    state_text = (ROOT / "shared" / "PROJECT_STATE.md").read_text(encoding="utf-8")
    exchange_text = (ROOT / "shared" / "PLANNER_EXCHANGE.md").read_text(encoding="utf-8")
    planner_rules = (ROOT / "shared" / "PLANNER_RULES.md").read_text(encoding="utf-8")
    executor_rules = (ROOT / "shared" / "EXECUTOR_RULES.md").read_text(encoding="utf-8")
    project_rules = (ROOT / "shared" / "PROJECT_RULES.md").read_text(encoding="utf-8")
    workflow_text = (ROOT / "shared" / "WORKFLOW.md").read_text(encoding="utf-8")
    onboarding_text = (ROOT / "shared" / "AI_ONBOARDING.md").read_text(encoding="utf-8")
    plan_template = (ROOT / "plans" / "TEMPLATE.md").read_text(encoding="utf-8")
    readme_text = (ROOT / "README.md").read_text(encoding="utf-8")
    docs_workflow_text = (ROOT / "docs" / "AI_WORKFLOW.md").read_text(encoding="utf-8")
    architecture_text = (ROOT / "docs" / "ARCHITECTURE.md").read_text(encoding="utf-8")

    if "only mandatory reading-order authority" not in agents:
        fail("AGENTS.md must remain the sole startup-order authority")
    if "Ordinary tasks follow `AGENTS.md` directly" not in onboarding_text:
        fail("AI_ONBOARDING.md must not redefine ordinary-task startup order")
    if len(state_text.splitlines()) > 80:
        fail("PROJECT_STATE.md exceeded 80 lines; move history to plans/Git")
    if "34 `ReEcho.*` automation tests" not in state_text:
        fail("PROJECT_STATE.md must report the current thirty-four-test suite")
    if "## Recently closed" in exchange_text or "## Decisions" in exchange_text:
        fail("PLANNER_EXCHANGE.md must contain live coordination only, not history or permanent rules")
    announcement_block = exchange_text.split("## Planned and active work announcements", 1)[1].split("## Active ownership", 1)[0]
    for row in announcement_block.splitlines():
        if not row.startswith("|") or row.startswith("|---") or "| Plan |" in row:
            continue
        cells = [cell.strip().strip("`") for cell in row.strip("|").split("|")]
        if len(cells) != 7:
            fail("PLANNER_EXCHANGE.md announcement rows must use the canonical seven-column schema")
        if cells[2] not in {"Proposed", "Ready", "InProgress", "Review", "Closed", "Blocked"}:
            fail(f"PLANNER_EXCHANGE.md uses unknown task status: {cells[2]}")
        if cells[3] not in {"NotRequired", "PendingBeforeClose", "PendingFollowUp", "Passed"}:
            fail(f"PLANNER_EXCHANGE.md uses unknown human-validation state: {cells[3]}")
        if cells[2] == "Closed" and cells[3] != "PendingFollowUp":
            fail("closed Exchange rows are retained only for a live PendingFollowUp; otherwise remove them")
    active_block = exchange_text.split("## Active ownership", 1)[1].split("## Warnings / blocked items", 1)[0]
    if "plan/07" in active_block.lower() or "plan/08" in active_block.lower():
        fail("completed Plans 07/08 must not retain active ownership")
    for row in active_block.splitlines():
        if not row.startswith("|") or row.startswith("|---") or "| Owner |" in row:
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
        if stale_token in exchange_text or stale_token in state_text:
            fail(f"live workflow memory contains stale token: {stale_token}")
    if re.search(r"origin/main[^\n]{0,40}`[0-9a-f]{7,40}`", state_text, re.IGNORECASE):
        fail("PROJECT_STATE.md must not cache a self-staling current origin/main hash")
    required_template_fields = (
        "## Coordination",
        "Task status:",
        "Human validation:",
        "Local planning / implementation base:",
        "Impact mode:",
        "Writes:",
        "Stable Reads:",
        "Compatibility promise / downstream action:",
        "Explicit exclusions:",
    )
    missing_template_fields = [field for field in required_template_fields if field not in plan_template]
    if missing_template_fields:
        fail(f"Plan template lacks local coordination fields: {', '.join(missing_template_fields)}")
    if "git rev-parse --path-format=absolute --git-common-dir" not in executor_rules:
        fail("Executor lock guidance must use the Git common directory shared by worktrees")
    if "Executors update their assigned Plan's Execution notes" not in project_rules:
        fail("project rules must keep routine shared-state writes out of Executor branches")
    if "not a second rulebook" not in workflow_text:
        fail("WORKFLOW.md must remain explanatory rather than a duplicate rulebook")
    integration_audit_markers = (
        "## External-commit integration audit",
        "Physical/Git conflict",
        "Logical conflict",
        "Coupling",
        "even when Git can fast-forward",
        "Fetch again immediately before push",
    )
    missing_audit_markers = [marker for marker in integration_audit_markers if marker not in planner_rules]
    if missing_audit_markers:
        fail(f"Planner rules lack the external-commit integration audit gate: {', '.join(missing_audit_markers)}")
    main_only_markers = {
        "PROJECT_RULES.md": ("`origin/main` is the only permitted remote branch", "never push any remote ref"),
        "PLANNER_RULES.md": ("`origin/main` is the only permitted remote branch", "Plan-number conflicts"),
        "EXECUTOR_RULES.md": ("`origin/main` is the only remote branch", "Do not push the task branch"),
        "WORKFLOW.md": ("exactly one branch: `main`", "Planning drafts are not published merely to announce intent"),
    }
    main_only_texts = {
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
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
        "PROJECT_STATE.md": state_text,
        "PLANNER_EXCHANGE.md": exchange_text,
        "plans/TEMPLATE.md": plan_template,
    }
    for name, text_value in live_remote_side_ref_files.items():
        if "coord/" in text_value or "Planning broadcast" in text_value or "push only the task branch" in text_value:
            fail(f"{name} still advertises a remote planning/task side-ref workflow")
    if "Design/Data/ReEchoData.xlsx" not in readme_text or "generated into validated CSV" not in readme_text:
        fail("README.md must describe the current XLSX-to-CSV authority")
    if "not a workflow authority" not in docs_workflow_text or "`shared/` is the collaboration control plane" not in docs_workflow_text:
        fail("docs/AI_WORKFLOW.md must remain a human pointer to the shared authority")
    architecture_markers = (
        "Design/Data/ReEchoData.xlsx",
        "16 validated UTF-8 production CSV",
        "run-locked weapon definition",
        "Legacy JSON is migration-only",
    )
    missing_architecture_markers = [marker for marker in architecture_markers if marker not in architecture_text]
    if missing_architecture_markers:
        fail(f"docs/ARCHITECTURE.md is stale: {', '.join(missing_architecture_markers)}")


def validate_build_dependencies() -> None:
    build_cs = (ROOT / "Source" / "ReEcho" / "ReEcho.Build.cs").read_text(encoding="utf-8")
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
