"""Run inside Unreal Editor Python to import the generated Plan34 diagnostic tone."""
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = ROOT / "Design" / "Audio" / "Generated" / "ReEchoDiagTone.wav"
DESTINATION_PATH = "/Game/ReEcho/Audio/Diagnostics"
DESTINATION_NAME = "ReEchoDiagTone"

if not SOURCE.is_file():
    raise RuntimeError(f"Missing generated source: {SOURCE}. Run scripts/audio/generate_diagnostic_tone.py first.")

task = unreal.AssetImportTask()
task.filename = str(SOURCE)
task.destination_path = DESTINATION_PATH
task.destination_name = DESTINATION_NAME
task.automated = True
task.replace_existing = True
task.replace_existing_settings = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

asset_path = f"{DESTINATION_PATH}/{DESTINATION_NAME}.{DESTINATION_NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    unreal.log(f"Imported Plan34 diagnostic tone: {asset_path}")
else:
    raise RuntimeError(f"Unreal import did not create {asset_path}")
