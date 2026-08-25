#!/usr/bin/env python3
"""Focused regression tests for Encounter.8's three-wave Boss contract."""

from __future__ import annotations

import sys
import unittest
from copy import deepcopy
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parents[1]
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

import validate_project as validator


ENCOUNTER_WAVES = validator.ROOT / "Content" / "Data" / "encounter_waves.csv"


def valid_waves() -> list[dict[str, str]]:
    return [
        {"__line__": "23", "WaveIndex": "1", "TriggerSeconds": "0", "BossEnemyId": "M_SHEEP"},
        {"__line__": "24", "WaveIndex": "2", "TriggerSeconds": "10", "BossEnemyId": ""},
        {"__line__": "25", "WaveIndex": "3", "TriggerSeconds": "20", "BossEnemyId": ""},
    ]


class EncounterBossWaveContractTests(unittest.TestCase):
    def assert_invalid(self, waves: list[dict[str, str]], token: str) -> None:
        with self.assertRaisesRegex(validator.ValidationError, token):
            validator.validate_boss_encounter_waves(waves, ENCOUNTER_WAVES)

    def test_accepts_exact_three_wave_single_boss_contract(self) -> None:
        validator.validate_boss_encounter_waves(valid_waves(), ENCOUNTER_WAVES)

    def test_rejects_missing_or_extra_wave(self) -> None:
        self.assert_invalid(valid_waves()[:2], "exactly three waves")
        extra = valid_waves() + [
            {"__line__": "26", "WaveIndex": "4", "TriggerSeconds": "30", "BossEnemyId": ""}
        ]
        self.assert_invalid(extra, "exactly three waves")

    def test_rejects_wrong_wave_index(self) -> None:
        waves = deepcopy(valid_waves())
        waves[1]["WaveIndex"] = "3"
        self.assert_invalid(waves, r":24:WaveIndex:")

    def test_rejects_wrong_trigger(self) -> None:
        waves = deepcopy(valid_waves())
        waves[2]["TriggerSeconds"] = "21"
        self.assert_invalid(waves, r":25:TriggerSeconds:")

    def test_rejects_missing_or_wrong_first_wave_boss(self) -> None:
        for boss_id in ("", "M_FOX"):
            with self.subTest(boss_id=boss_id):
                waves = deepcopy(valid_waves())
                waves[0]["BossEnemyId"] = boss_id
                self.assert_invalid(waves, r":23:BossEnemyId:")

    def test_rejects_duplicate_boss_in_reinforcement_wave(self) -> None:
        waves = deepcopy(valid_waves())
        waves[1]["BossEnemyId"] = "M_SHEEP"
        self.assert_invalid(waves, r":24:BossEnemyId:.*reinforcement-only")


if __name__ == "__main__":
    unittest.main(verbosity=2)
