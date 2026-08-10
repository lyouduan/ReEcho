#!/usr/bin/env python3
"""Fast static validation that does not require Unreal Engine."""

from __future__ import annotations

import csv
import io
import json
import math
import sys
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
}
REGISTERED_EFFECT_KINDS = {"ScalarModifier", "StatModifier", "InstantRecovery"}
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
            if spec.kind in {"StableId", "BehaviorId", "EffectKind", "ForeignKey"} and not stable_id(value):
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
    validate_character_build_domain(data_dir, entries)


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


def expect_fixture_failure(name: str, token: str) -> None:
    fixture = DATA / "TestFixtures" / "CsvRuntime" / name
    try:
        validate_csv_package(fixture)
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

    if "only mandatory reading-order authority" not in agents:
        fail("AGENTS.md must remain the sole startup-order authority")
    if len(state_text.splitlines()) > 80:
        fail("PROJECT_STATE.md exceeded 80 lines; move history to plans/Git")
    if "twenty-four `ReEcho.*` automation tests pass" not in state_text:
        fail("PROJECT_STATE.md must report the current twenty-four-test baseline")
    active_block = exchange_text.split("## Active ownership", 1)[1].split("## Recently closed", 1)[0]
    if "plan/07" in active_block.lower() or "plan/08" in active_block.lower():
        fail("completed Plans 07/08 must not retain active ownership")
    duplicate_heading = "提交前 Markdown 同步（ReEcho 项目规则）"
    if duplicate_heading in planner_rules or duplicate_heading in executor_rules:
        fail("role rules duplicate the project-level Markdown commit rule")


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
    ):
        if f"Content/Data/{file_name}" not in build_cs:
            fail(f"ReEcho.Build.cs does not stage production CSV {file_name}")


def main() -> int:
    json_count, effective_cards, encounter_count = validate_legacy_json()
    validate_csv_schema()
    validate_csv_package(DATA)
    validate_csv_package(DATA / "TestFixtures" / "CsvRuntime" / "ValidAlt")
    for name, token in {
        "DuplicateId": "duplicate id",
        "MissingRequired": "required value",
        "UnknownReference": "unknown reference",
        "UnknownBehavior": "behavior id",
        "UnknownEffectKind": "effect kind",
        "IllegalRange": "exceeds",
        "UnsupportedVersion": "SchemaVersion",
        "DuplicateCardEffectOrder": "duplicate CardId/Order",
        "UnsupportedCardEffectTrigger": "Trigger",
        "UnknownCardEffectTarget": "Target",
        "InvalidCardEffectBehaviorPair": "EffectKind/BehaviorId",
    }.items():
        expect_fixture_failure(name, token)
    validate_build_dependencies()
    validate_workflow()

    print(f"[PASS] legacy migration-only JSON files={json_count} effective_cards={effective_cards} encounters={encounter_count}")
    print("[PASS] CSV schema, production character/build tables, fixtures, IDs, references, behavior/effect allowlists, UTF-8 and staging deps")
    print("[PASS] workflow memory, token guards, and Unreal project descriptor present")
    print("Evidence level: static verified only (no UHT/UBT/PIE claim)")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValidationError as exc:
        print(f"[FAIL] {exc}")
        sys.exit(1)
