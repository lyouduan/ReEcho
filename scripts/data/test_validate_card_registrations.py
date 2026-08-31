"""Keep static card registration in sync without weakening unknown-ID rejection."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import validate_project as validator


class CardRegistrationTests(unittest.TestCase):
    def test_existing_runtime_behavior_and_target_are_registered(self):
        self.assertIn("Card.EasterShardThreshold", validator.REGISTERED_BEHAVIOR_IDS)
        self.assertIn("CritNegateAmplification", validator.CARD_TARGETS)

    def test_production_csv_package_is_valid(self):
        validator.validate_csv_package(validator.DATA)

    def test_unknown_behavior_is_still_rejected(self):
        validator.expect_fixture_failure("UnknownBehavior", "behavior id")

    def test_unknown_card_target_is_still_rejected(self):
        validator.expect_fixture_failure("UnknownCardEffectTarget", "Target")


if __name__ == "__main__":
    unittest.main(verbosity=2)
