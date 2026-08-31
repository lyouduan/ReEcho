#!/usr/bin/env python3
"""Regression coverage for the card behavior/target validation registrations."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parents[1]
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

import validate_project as validator


class CardRegistrationValidationTests(unittest.TestCase):
    def test_current_production_csv_accepts_implemented_card_registrations(self) -> None:
        # Validate the real rows, including both generic and card-specific gates.
        # This does not assert XLSX sync or gameplay correctness.
        validator.validate_csv_package(validator.DATA)

    def test_unknown_behavior_still_fails(self) -> None:
        validator.expect_fixture_failure("UnknownBehavior", "behavior id")

    def test_unknown_card_target_still_fails(self) -> None:
        validator.expect_fixture_failure("UnknownCardEffectTarget", "Target")


if __name__ == "__main__":
    unittest.main()
