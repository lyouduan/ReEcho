#!/usr/bin/env python3

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from . import sync_difficulty_xlsx_to_csv as sync


class DifficultyWorkbookSyncTests(unittest.TestCase):
    def test_initial_packages_have_identical_structure_and_bytes(self) -> None:
        packages: dict[str, dict[str, bytes]] = {}
        for difficulty in sync.DIFFICULTIES:
            with tempfile.TemporaryDirectory(prefix=f"reecho_{difficulty.lower()}_test_") as temp:
                packages[difficulty] = sync.generate_workbook(sync.workbook_path(difficulty), Path(temp))
        self.assertEqual(set(packages["Party"]), set(packages["Standard"]))
        self.assertEqual(set(packages["Nightmare"]), set(packages["Standard"]))
        self.assertEqual(packages["Party"], packages["Standard"])
        self.assertEqual(packages["Nightmare"], packages["Standard"])

    def test_published_packages_validate_as_complete_runtime_overlays(self) -> None:
        for difficulty in sync.DIFFICULTIES:
            sync.validate_as_full_package(sync.DIFFICULTY_DIR / difficulty)


if __name__ == "__main__":
    unittest.main()
