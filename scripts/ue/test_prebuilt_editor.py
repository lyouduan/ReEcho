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
        (root / "Binaries" / "Win64" / "ReEchoEditor.target").write_text(
            json.dumps(
                {
                    "TargetName": "ReEchoEditor",
                    "Platform": "Win64",
                    "Configuration": "Development",
                    "Project": "../../ReEcho.uproject",
                    "Version": {"BuildId": "55116800"},
                }
            ),
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
            target_text = (root / "Binaries" / "Win64" / "ReEchoEditor.target").read_text(encoding="utf-8")
            self.assertNotIn(" \n", target_text)

    def test_source_change_is_stale(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            PREBUILT.update(root)
            (root / "Source" / "ReEcho.Target.cs").write_text("target-v2\n", encoding="utf-8")
            with self.assertRaisesRegex(PREBUILT.PrebuiltError, "stale source fingerprint"):
                PREBUILT.check(root)

    def test_source_line_endings_do_not_stale_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            source = root / "Source" / "ReEcho.Target.cs"
            source.write_bytes(b"first\nsecond\n")
            PREBUILT.update(root)
            source.write_bytes(b"first\r\nsecond\r\n")
            PREBUILT.check(root)

    def test_binary_change_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            PREBUILT.update(root)
            (root / "Binaries" / "Win64" / "UnrealEditor-ReEcho.dll").write_bytes(b"tampered")
            with self.assertRaisesRegex(PREBUILT.PrebuiltError, "binary hash mismatch"):
                PREBUILT.check(root)

    def test_target_build_id_must_match_modules(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            self.make_project(root)
            target_path = root / "Binaries" / "Win64" / "ReEchoEditor.target"
            target = json.loads(target_path.read_text(encoding="utf-8"))
            target["Version"]["BuildId"] = "different"
            target_path.write_text(json.dumps(target), encoding="utf-8")
            with self.assertRaisesRegex(PREBUILT.PrebuiltError, "does not match UnrealEditor.modules"):
                PREBUILT.update(root)


if __name__ == "__main__":
    unittest.main()
