import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).with_name("import_combat_vfx.py")
SPEC = importlib.util.spec_from_file_location("import_combat_vfx", SCRIPT_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class CombatVfxImportTests(unittest.TestCase):
    def test_reference_extraction_supports_ascii_and_utf16(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Root.uasset"
            path.write_bytes(
                b"prefix/Game/VFX/Test/Ascii.Asset\x00"
                + "/Game/Mat/Utf16.Material".encode("utf-16-le")
            )
            self.assertEqual(
                MODULE.extract_package_references(path),
                {"/Game/VFX/Test/Ascii", "/Game/Mat/Utf16"},
            )

    def test_copy_refuses_to_overwrite_a_different_asset(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source_root = root / "source"
            content_root = root / "project" / "Content"
            relative_path = Path("VFX/Test.uasset")
            source_path = source_root / relative_path
            target_path = content_root / relative_path
            source_path.parent.mkdir(parents=True)
            target_path.parent.mkdir(parents=True)
            source_path.write_bytes(b"source")
            target_path.write_bytes(b"different")
            row = MODULE.ManifestRow(
                package="/Game/VFX/Test",
                relative_path=relative_path.as_posix(),
                sha256=MODULE.sha256_file(source_path),
                size=source_path.stat().st_size,
                required_by="/Game/VFX/Test",
            )
            with self.assertRaisesRegex(RuntimeError, "refusing to overwrite"):
                MODULE.copy_rows(source_root, content_root, [row])

    def test_copy_accepts_a_named_project_adaptation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source_root = root / "source"
            content_root = root / "project" / "Content"
            relative_path = Path("VFX/Adapted.uasset")
            source_path = source_root / relative_path
            target_path = content_root / relative_path
            source_path.parent.mkdir(parents=True)
            target_path.parent.mkdir(parents=True)
            source_path.write_bytes(b"source")
            target_path.write_bytes(b"editor-adapted")
            row = MODULE.ManifestRow(
                package="/Game/VFX/Adapted",
                relative_path=relative_path.as_posix(),
                sha256=MODULE.sha256_file(source_path),
                size=source_path.stat().st_size,
                required_by="/Game/VFX/Adapted",
            )
            MODULE.PROJECT_ADAPTATIONS[row.relative_path] = (
                MODULE.sha256_file(target_path),
                target_path.stat().st_size,
                "test adaptation",
            )
            try:
                MODULE.copy_rows(source_root, content_root, [row])
            finally:
                MODULE.PROJECT_ADAPTATIONS.pop(row.relative_path)


if __name__ == "__main__":
    unittest.main()
