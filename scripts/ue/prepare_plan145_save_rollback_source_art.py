"""Stage the approved Plan145 save-rollback source art in the project."""

from pathlib import Path
import shutil


SOURCE_DIR = Path(r"C:\Users\gavynqiu\Documents\miniGame\正式-UI视觉\正式-UI视觉\存档回溯、设置、关于")
DESTINATION_DIR = (
    Path(__file__).resolve().parents[2]
    / "Content"
    / "SourceArt"
    / "UI"
    / "SaveRollback"
    / "Plan145"
)

FILES = {
    "游戏设置框.png": "SaveRollbackFrame.png",
    "设置框底板浅.png": "SaveRollbackSurface.png",
    "标题-存档回溯.png": "SaveRollbackTitle.png",
    "存档底板浅.png": "SaveSlotOccupied.png",
    "存档底板深.png": "SaveSlotEmpty.png",
    "预览图占位.png": "SaveSlotPlaceholder.png",
    "备注栏.png": "SaveSlotNoteStrip.png",
    "关闭.png": "SaveRollbackClose.png",
}


DESTINATION_DIR.mkdir(parents=True, exist_ok=True)
for source_name, destination_name in FILES.items():
    source = SOURCE_DIR / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing approved Plan145 art: {source}")
    shutil.copy2(source, DESTINATION_DIR / destination_name)
    print(f"Staged {source_name} -> {destination_name}")

