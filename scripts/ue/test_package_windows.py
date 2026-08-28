from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


MODULE_PATH = Path(__file__).with_name("package_windows.py")
SPEC = importlib.util.spec_from_file_location("package_windows", MODULE_PATH)
assert SPEC and SPEC.loader
package_windows = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = package_windows
SPEC.loader.exec_module(package_windows)


class PackageWindowsTests(unittest.TestCase):
    def test_mixed_uat_output_decodes_utf8_then_windows_fallback(self) -> None:
        self.assertEqual(
            package_windows.decode_process_output("构建成功\n".encode("utf-8"), "gbk"),
            "构建成功\n",
        )
        self.assertEqual(
            package_windows.decode_process_output("正在创建库\n".encode("gbk"), "gbk"),
            "正在创建库\n",
        )

    def test_default_mode_is_clean_development_test_package(self) -> None:
        args = package_windows.parse_args([])
        self.assertEqual(package_windows.configuration_for(args), "Development")
        self.assertFalse(args.remote_main)
        command = package_windows.make_uat_command(
            Path("C:/UE_5.8"),
            Path("C:/Project/ReEcho.uproject"),
            Path("C:/Output"),
            "/Game/Level00",
            "Development",
        )
        self.assertIn("-clientconfig=Development", command)
        self.assertIn("-clean", command)
        self.assertIn("-build", command)
        self.assertIn("-cook", command)
        self.assertIn("-archive", command)
        self.assertNotIn("-clientconfig=Shipping", command)

    def test_formal_selects_shipping(self) -> None:
        args = package_windows.parse_args(["--formal"])
        self.assertEqual(package_windows.configuration_for(args), "Shipping")

    def test_default_output_is_timestamped_on_desktop(self) -> None:
        source = package_windows.SourceContext(
            Path("C:/Project"),
            Path("C:/Project/ReEcho.uproject"),
            "1234567890abcdef",
            "origin/main",
            "origin-main",
        )
        with tempfile.TemporaryDirectory() as temporary:
            with mock.patch.dict(os.environ, {"REECHO_PACKAGE_DESKTOP": temporary}):
                output = package_windows.default_output_path(
                    source, "Development", "20260828-120000"
                )
        self.assertEqual(output.parent, Path(temporary).resolve())
        self.assertEqual(
            output.name,
            "ReEcho_origin-main_12345678_Win64_Development_Test_20260828-120000",
        )

    def test_manifest_drives_runtime_csv_list(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            data = root / "Content" / "Data"
            data.mkdir(parents=True)
            (data / "reecho_data_manifest.csv").write_text(
                "SchemaVersion,TableId,FileName,PrimaryKey\n"
                "1,Characters,characters.csv,Id\n"
                "1,Weapons,weapons.csv,Id\n",
                encoding="utf-8",
            )
            self.assertEqual(
                package_windows.runtime_csv_names(root),
                ["characters.csv", "reecho_data_manifest.csv", "weapons.csv"],
            )

    def test_only_game_content_directory_is_selected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            game_content = root / "Windows" / "ReEcho" / "Content"
            engine_content = root / "Windows" / "Engine" / "Content"
            plugin_content = root / "Windows" / "Plugins" / "Example" / "Content"
            for path in (game_content, engine_content, plugin_content):
                path.mkdir(parents=True)
            self.assertEqual(
                package_windows.packaged_content_directories(root),
                [game_content],
            )

    def test_smoke_log_contract_accepts_success_and_warns_on_ensure(self) -> None:
        text = "\n".join(
            [
                "LogInit: Display: Engine is initialized.",
                "[RuntimeAssetPreload] Completed. Success=true ResidentAssets=12",
                "Ensure condition failed: optional presentation fallback",
            ]
        )
        failures, warnings = package_windows.analyze_smoke_logs(text)
        self.assertEqual(failures, [])
        self.assertEqual(len(warnings), 1)

    def test_smoke_log_contract_rejects_missing_csv_and_preload_failure(self) -> None:
        text = "Engine is initialized\ncharacters.csv: File could not be read"
        failures, _ = package_windows.analyze_smoke_logs(text)
        self.assertTrue(any("RuntimeAssetPreload" in item for item in failures))
        self.assertTrue(any("CSV" in item for item in failures))

    def test_nonempty_explicit_output_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            (output / "existing.txt").write_text("keep", encoding="utf-8")
            with self.assertRaises(package_windows.PackageError):
                package_windows.prepare_output(output)


if __name__ == "__main__":
    unittest.main()
