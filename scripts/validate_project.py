#!/usr/bin/env python3
"""Fast static validation that does not require Unreal Engine."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Content" / "Data"


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def load_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"{path.relative_to(ROOT)}: {exc}")


def main() -> int:
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
        fail(f"missing data files: {', '.join(missing)}")

    documents = {path.name: load_json(path) for path in DATA.glob("*.json")}
    project = load_json(ROOT / "ReEcho.uproject")
    modules = {module.get("Name") for module in project.get("Modules", [])}
    if "ReEcho" not in modules:
        fail("ReEcho.uproject does not declare the ReEcho runtime module")

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
    if "sixteen `ReEcho.*` automation tests pass" not in state_text:
        fail("PROJECT_STATE.md must report the current sixteen-test baseline")
    active_block = exchange_text.split("## Active ownership", 1)[1].split("## Recently closed", 1)[0]
    if "plan/07" in active_block.lower() or "plan/08" in active_block.lower():
        fail("completed Plans 07/08 must not retain active ownership")
    duplicate_heading = "提交前 Markdown 同步（ReEcho 项目规则）"
    if duplicate_heading in planner_rules or duplicate_heading in executor_rules:
        fail("role rules duplicate the project-level Markdown commit rule")

    print(f"[PASS] JSON files={len(documents)} effective_cards={len(effective_cards)} encounters={len(encounters)}")
    print("[PASS] workflow memory, token guards, and Unreal project descriptor present")
    print("Evidence level: static verified only (no UHT/UBT/PIE claim)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
