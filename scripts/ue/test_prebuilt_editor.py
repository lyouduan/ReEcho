#!/usr/bin/env python3
"""Focused tests for the curated Unreal Editor bundle manifest."""

from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("prebuilt_editor.py")
SPEC = importlib.util.spec_from_file_location("prebuilt_editor", SCRIPT)
assert SPEC and SPEC.loader
PREBUILT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PREBUILT)


class PrebuiltEditorTests(unittest.TestCase):
    def make_project(self, root: Path) -> None:
        (root / "Source" / "ReEcho").mkdir(parents=True)
        (root / "Binaries" / "Win64").mkdir(parents=True)
        (root / "ReEcho.uproject").write_text(
            json.dumps({"EngineAssociation": "5.8", "Modules": [{"Name": "ReEcho"}]}),
            encoding="utf-8",
        )
        (root / "Source" / "ReEcho.Target.cs").write_text("target-v1\n", encoding="utf-8")
        (root / "Source" / "ReEcho" / "ReEcho.Build.cs").write_text("rules-v1\n", encoding="utf-8")
        (root / "Binaries" / "Win64" / "UnrealEditor.modules").write_text(
            json.dumps({"BuildId": "55116800", "Modules": {"ReEcho": "UnrealEditor-ReEcho.dll"}}),
            encoding="utf-8",
        )
        (root / "Binaries" / "Win64" / "UnrealEditor-ReEcho.dll").write_bytes(b"module-v1")

    def test_update_then_check(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            expected = PREBUILT.update(root)
            self.assertEqual(expected, PREBUILT.check(root))
            modules_text = (root / "Binaries" / "Win64" / "UnrealEditor.modules").read_text(encoding="utf-8")
            self.assertNotIn(" \n", modules_text)

    def test_source_change_is_stale(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            PREBUILT.update(root)
            (root / "Source" / "ReEcho.Target.cs").write_text("target-v2\n", encoding="utf-8")
            with self.assertRaisesRegex(PREBUILT.PrebuiltError, "stale source fingerprint"):
                PREBUILT.check(root)

    def test_binary_change_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            PREBUILT.update(root)
            (root / "Binaries" / "Win64" / "UnrealEditor-ReEcho.dll").write_bytes(b"tampered")
            with self.assertRaisesRegex(PREBUILT.PrebuiltError, "binary hash mismatch"):
                PREBUILT.check(root)


if __name__ == "__main__":
    unittest.main()
