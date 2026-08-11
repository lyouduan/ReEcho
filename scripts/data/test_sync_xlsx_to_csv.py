#!/usr/bin/env python3
"""Regression tests for the Plan25 XLSX authoring sync."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from openpyxl import load_workbook

import sync_xlsx_to_csv as sync


ROOT = sync.ROOT
DATA = sync.DATA_DIR
SCRIPT = ROOT / "scripts" / "data" / "sync_xlsx_to_csv.py"
CANONICAL = sync.CANONICAL_XLSX


class SyncXlsxToCsvTests(unittest.TestCase):
    def run_sync(self, *args: str, env: dict[str, str] | None = None, expect_success: bool = True) -> subprocess.CompletedProcess[str]:
        merged_env = os.environ.copy()
        if env:
            merged_env.update(env)
        result = subprocess.run(
            [sys.executable, str(SCRIPT), *args],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=merged_env,
        )
        if expect_success and result.returncode != 0:
            self.fail(result.stdout)
        if not expect_success and result.returncode == 0:
            self.fail("command unexpectedly succeeded")
        return result

    def copy_workbook(self, temp_dir: Path) -> Path:
        target = temp_dir / "fixture.xlsx"
        shutil.copy2(CANONICAL, target)
        return target

    def mutate_workbook(self, mutator) -> Path:
        temp_dir = Path(tempfile.mkdtemp(prefix="reecho_xlsx_fixture_"))
        workbook_path = self.copy_workbook(temp_dir)
        wb = load_workbook(workbook_path)
        mutator(wb)
        wb.save(workbook_path)
        self.addCleanup(lambda: shutil.rmtree(temp_dir, ignore_errors=True))
        return workbook_path

    def assert_invalid_workbook(self, mutator, token: str) -> None:
        workbook_path = self.mutate_workbook(mutator)
        result = self.run_sync("--input", str(workbook_path), "--check", expect_success=False)
        self.assertIn(token, result.stdout)
        self.assertRegex(result.stdout, r":tbl[A-Za-z]+:row \d+:column ")

    def production_bytes(self) -> dict[str, bytes]:
        return {name: (DATA / name).read_bytes() for name in sync.TABLE_TO_CSV.values()}

    def restore_production(self, before: dict[str, bytes]) -> None:
        for name, data in before.items():
            (DATA / name).write_bytes(data)
        marker = DATA / sync.TRANSACTION_FILE
        marker.unlink(missing_ok=True)
        for path in DATA.glob(".reecho_csv_publish_backup_*"):
            if path.is_dir():
                shutil.rmtree(path, ignore_errors=True)

    def test_export_map_covers_manifest_without_duplicate_ownership(self) -> None:
        wb = load_workbook(CANONICAL, read_only=False)
        owners = sync.read_export_map(wb)
        outputs = [owner.output_csv for owner in owners]
        self.assertEqual(sorted(outputs), sorted(sync.TABLE_TO_CSV.values()))
        self.assertEqual(len(outputs), len(set(outputs)))
        by_sheet: dict[str, list[str]] = {}
        for owner in owners:
            by_sheet.setdefault(owner.sheet, []).append(owner.output_csv)
        self.assertEqual(sorted(by_sheet["角色体系J"]), ["character_aliases.csv", "characters.csv"])
        self.assertEqual(sorted(by_sheet["构筑体系G"]), ["card_effects.csv", "cards.csv"])
        self.assertEqual(sorted(by_sheet["武器体系W"]), ["attack_steps.csv", "weapon_types.csv", "weapons.csv"])
        self.assertEqual(sorted(by_sheet["武器插槽C"]), ["part_effects.csv", "parts.csv", "slot_profiles.csv", "slot_types.csv"])

    def test_output_is_deterministic_and_matches_accepted_csv_bytes(self) -> None:
        with tempfile.TemporaryDirectory(prefix="reecho_xlsx_out_a_") as first, tempfile.TemporaryDirectory(prefix="reecho_xlsx_out_b_") as second:
            self.run_sync("--output-dir", first)
            self.run_sync("--output-dir", second)
            for name in sync.TABLE_TO_CSV.values():
                first_bytes = (Path(first) / name).read_bytes()
                self.assertEqual(first_bytes, (Path(second) / name).read_bytes(), name)
                self.assertEqual(first_bytes, (DATA / name).read_bytes(), name)

    def test_check_is_read_only_and_reports_drift(self) -> None:
        before = self.production_bytes()
        try:
            target = DATA / "runtime_smoke.csv"
            drifted = before["runtime_smoke.csv"].replace(b"42.5", b"43.5", 1)
            target.write_bytes(drifted)
            result = self.run_sync("--check", expect_success=False)
            self.assertIn("Generated CSV drift detected", result.stdout)
            self.assertEqual(target.read_bytes(), drifted)
        finally:
            self.restore_production(before)

    def test_sheet_publish_validates_global_snapshot_and_writes_group(self) -> None:
        before = self.production_bytes()
        try:
            self.run_sync("--sheet", "武器体系W")
            after = self.production_bytes()
            self.assertEqual(before, after)
        finally:
            self.restore_production(before)

    def test_noncanonical_input_cannot_write_production(self) -> None:
        with tempfile.TemporaryDirectory(prefix="reecho_xlsx_noncanonical_") as temp:
            workbook_path = self.copy_workbook(Path(temp))
            result = self.run_sync("--input", str(workbook_path), expect_success=False)
            self.assertIn("Non-canonical --input cannot publish", result.stdout)

    def test_invalid_workbook_cases_fail_with_location(self) -> None:
        self.assert_invalid_workbook(lambda wb: wb["角色体系J"].tables.pop("tblCharacters"), "Workbook table is missing")
        self.assert_invalid_workbook(lambda wb: setattr(wb["角色体系J"]["Q3"], "value", "HpMaxBroken"), "Columns do not match")
        self.assert_invalid_workbook(lambda wb: setattr(wb["角色体系J"]["Q4"], "value", "not-a-number"), "finite number")
        self.assert_invalid_workbook(lambda wb: setattr(wb["角色体系J"]["N4"], "value", "W_UNKNOWN"), "DefaultWeaponId")
        self.assert_invalid_workbook(lambda wb: setattr(wb["角色体系J"]["P4"], "value", "Unknown.Handler"), "behavior id")
        self.assert_invalid_workbook(lambda wb: setattr(wb["_SystemData"]["D4"], "value", "=1+1"), "Formula cells are not allowed")
        self.assert_invalid_workbook(lambda wb: setattr(wb["_ExportMap"]["D2"], "value", "../characters.csv"), "plain manifest filename")

    def test_publish_failure_rolls_back_all_changed_bytes(self) -> None:
        before = self.production_bytes()
        try:
            result = self.run_sync(env={"REECHO_XLSX_FAIL_AFTER_REPLACE": "1"}, expect_success=False)
            self.assertIn("Injected publish failure", result.stdout)
            self.assertEqual(before, self.production_bytes())
            self.assertFalse((DATA / sync.TRANSACTION_FILE).exists())
        finally:
            self.restore_production(before)

    def test_legacy_transaction_is_recovered_before_publish(self) -> None:
        before = self.production_bytes()
        backup_dir = DATA / ".reecho_csv_publish_backup_test"
        backup_dir.mkdir(exist_ok=True)
        try:
            backup = backup_dir / "runtime_smoke.csv"
            backup.write_bytes(before["runtime_smoke.csv"])
            (DATA / "runtime_smoke.csv").write_bytes(b"corrupted\r\n")
            marker = DATA / sync.TRANSACTION_FILE
            marker.write_text(
                json.dumps(
                    {
                        "backup_dir": str(backup_dir),
                        "files": [{"target": str(DATA / "runtime_smoke.csv"), "backup": str(backup)}],
                    }
                ),
                encoding="utf-8",
            )
            self.run_sync()
            self.assertEqual(before, self.production_bytes())
            self.assertFalse(marker.exists())
            self.assertFalse(backup_dir.exists())
        finally:
            self.restore_production(before)


if __name__ == "__main__":
    unittest.main(verbosity=2)
