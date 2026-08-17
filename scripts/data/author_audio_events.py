#!/usr/bin/env python3
"""Author the standalone Plan34 audio catalog workbook.

This script only writes Design/Data/ReEchoAudioEvents.xlsx. It never touches
ReEchoData.xlsx or ReEchoEnemyData.xlsx.
"""
from pathlib import Path

import openpyxl
from openpyxl.styles import Font, PatternFill, Protection
from openpyxl.utils import get_column_letter
from openpyxl.worksheet.datavalidation import DataValidation
from openpyxl.worksheet.table import Table, TableColumn, TableStyleInfo

ROOT = Path(__file__).resolve().parents[2]
WORKBOOK = ROOT / "Design" / "Data" / "ReEchoAudioEvents.xlsx"
COLUMNS = [
    "EventId", "AssetPath", "Bus", "EventType", "Spatial3D", "BaseVolume",
    "PitchMin", "PitchMax", "CooldownSeconds", "MaxConcurrency", "Priority",
    "PausePolicy", "AttenuationMin", "AttenuationMax",
]

EVENTS = [
    ("Music.Menu", "Music", "Loop"),
    ("Music.Encounter", "Music", "Loop"),
    ("Music.Boss", "Music", "Loop"),
    ("Music.Shop", "Music", "Loop"),
    ("Music.Death", "Music", "Loop"),
    ("Music.Victory", "Music", "Loop"),
    ("Ambience.Arena", "Ambience", "Loop"),
    ("Ambience.Rain", "Ambience", "Loop"),
    ("UI.Hover", "UiSfx", "OneShot"),
    ("UI.Confirm", "UiSfx", "OneShot"),
    ("UI.Cancel", "UiSfx", "OneShot"),
    ("UI.Error", "UiSfx", "OneShot"),
    ("UI.Purchase", "UiSfx", "OneShot"),
    ("UI.CardSelect", "UiSfx", "OneShot"),
    ("Combat.Attack", "CombatSfx", "OneShot"),
    ("Combat.Hit", "CombatSfx", "OneShot"),
    ("Combat.Block", "CombatSfx", "OneShot"),
    ("Combat.Hurt", "CombatSfx", "OneShot"),
    ("Combat.Kill", "CombatSfx", "OneShot"),
    ("Combat.Death", "CombatSfx", "OneShot"),
    ("Enemy.Spawn", "CombatSfx", "OneShot"),
    ("Enemy.Attack", "CombatSfx", "OneShot"),
    ("Enemy.Death", "CombatSfx", "OneShot"),
    ("Boss.Spawn", "CombatSfx", "OneShot"),
    ("Boss.Attack", "CombatSfx", "OneShot"),
    ("Boss.Death", "CombatSfx", "OneShot"),
    ("Echo.Spawn", "CombatSfx", "OneShot"),
    ("Echo.Attack", "CombatSfx", "OneShot"),
    ("Echo.End", "CombatSfx", "OneShot"),
    ("CameraMove", "UiSfx", "OneShot"),
    ("Revive", "Music", "OneShot"),
]

EVENT_ASSET_PATHS = {
    "Music.Menu": "/Game/ReEcho/Audio/Music/Music_Menu.Music_Menu",
    "Music.Encounter": "/Game/ReEcho/Audio/Music/The_Iron_Waltz.The_Iron_Waltz",
    "Music.Boss": "/Game/ReEcho/Audio/Music/Music_Boss.Music_Boss",
    "Music.Shop": "/Game/ReEcho/Audio/Music/Music_Shop.Music_Shop",
    "Music.Death": "/Game/ReEcho/Audio/Music/Music_Death.Music_Death",
    "Music.Victory": "/Game/ReEcho/Audio/Music/Music_Victory.Music_Victory",
    "Ambience.Arena": "/Game/ReEcho/Audio/Ambience/Ambience_Arena.Ambience_Arena",
    "Ambience.Rain": "/Game/ReEcho/Audio/Ambience/Ambience_Rain.Ambience_Rain",
    "UI.Hover": "/Game/ReEcho/Audio/UI/UI_Hover.UI_Hover",
    "UI.Confirm": "/Game/ReEcho/Audio/UI/UI_Confirm.UI_Confirm",
    "UI.Cancel": "/Game/ReEcho/Audio/UI/UI_Cancel.UI_Cancel",
    "UI.Error": "/Game/ReEcho/Audio/UI/UI_Error.UI_Error",
    "UI.Purchase": "/Game/ReEcho/Audio/UI/UI_Purchase.UI_Purchase",
    "UI.CardSelect": "/Game/ReEcho/Audio/UI/UI_CardSelect.UI_CardSelect",
    "Combat.Attack": "/Game/ReEcho/Audio/Combat/Combat_Attack.Combat_Attack",
    "Combat.Hit": "/Game/ReEcho/Audio/Combat/Combat_Hit.Combat_Hit",
    "Combat.Block": "/Game/ReEcho/Audio/Combat/Combat_Block.Combat_Block",
    "Combat.Hurt": "/Game/ReEcho/Audio/Combat/Combat_Hurt.Combat_Hurt",
    "Combat.Kill": "/Game/ReEcho/Audio/Combat/Combat_Kill.Combat_Kill",
    "Combat.Death": "/Game/ReEcho/Audio/Combat/Combat_Death.Combat_Death",
    "Enemy.Spawn": "/Game/ReEcho/Audio/Enemy/Enemy_Spawn.Enemy_Spawn",
    "Enemy.Attack": "/Game/ReEcho/Audio/Enemy/Enemy_Attack.Enemy_Attack",
    "Enemy.Death": "/Game/ReEcho/Audio/Enemy/Enemy_Death.Enemy_Death",
    "Boss.Spawn": "/Game/ReEcho/Audio/Boss/Boss_Spawn.Boss_Spawn",
    "Boss.Attack": "/Game/ReEcho/Audio/Boss/Boss_Attack.Boss_Attack",
    "Boss.Death": "/Game/ReEcho/Audio/Boss/Boss_Death.Boss_Death",
    "Echo.Spawn": "/Game/ReEcho/Audio/Echo/Echo_Spawn.Echo_Spawn",
    "Echo.Attack": "/Game/ReEcho/Audio/Echo/Echo_Attack.Echo_Attack",
    "Echo.End": "/Game/ReEcho/Audio/Echo/Echo_End.Echo_End",
    "CameraMove": "/Game/ReEcho/Audio/Flow/CameraMove.CameraMove",
    "Revive": "/Game/ReEcho/Audio/Flow/Revive.Revive",
}


def add_table(ws, name: str, headers: list[str], rows: list[list[object]], style: str) -> None:
    for column, header in enumerate(headers, 1):
        cell = ws.cell(1, column, header)
        cell.font = Font(bold=True, color="FFFFFFFF")
        cell.fill = PatternFill("solid", fgColor="FF4F81BD")
        cell.protection = Protection(locked=True)
    for row_index, row in enumerate(rows, 2):
        for column, value in enumerate(row, 1):
            ws.cell(row_index, column, value).protection = Protection(locked=False)
    ref = f"A1:{get_column_letter(len(headers))}{len(rows) + 1}"
    table = Table(displayName=name, ref=ref)
    table.tableColumns = [TableColumn(id=index + 1, name=header) for index, header in enumerate(headers)]
    table.tableStyleInfo = TableStyleInfo(name=style, showRowStripes=True, showFirstColumn=False,
                                          showLastColumn=False, showColumnStripes=False)
    ws.add_table(table)


def main() -> int:
    wb = openpyxl.Workbook()
    wb.remove(wb.active)

    export = wb.create_sheet("_ExportMap")
    add_table(export, "tblExportMap", ["SheetName", "TableName", "AdapterVersion", "OutputCsv", "SortKey"],
              [["AudioEvents", "tblAudioEvents", 1, "audio_events.csv", 30]], "TableStyleMedium9")
    export.protection.sheet = True
    export.protection.insertRows = True
    export.protection.deleteRows = True
    for row in export.iter_rows():
        for cell in row:
            cell.protection = Protection(locked=True)

    rows: list[list[object]] = []
    for event_id, bus, event_type in EVENTS:
        spatial = bus == "CombatSfx"
        asset = EVENT_ASSET_PATHS[event_id]
        rows.append([
            event_id, asset, bus, event_type, str(spatial).lower(), 1.0, 0.95 if spatial else 1.0,
            1.05 if spatial else 1.0, 0.05 if event_type == "OneShot" else 0.0,
            8 if event_type == "OneShot" else 1, 20 if bus == "CombatSfx" else 10,
            "ContinueOnPause" if bus in {"Music", "Ambience", "UiSfx"} else "PauseWithGame",
            200.0 if spatial else 0.0, 2000.0 if spatial else 0.0,
        ])

    audio = wb.create_sheet("AudioEvents")
    add_table(audio, "tblAudioEvents", COLUMNS, rows, "TableStyleMedium2")
    audio.protection.sheet = True
    audio.protection.insertRows = False
    audio.protection.deleteRows = False
    validation_values = {
        "Bus": "Music,Ambience,CombatSfx,UiSfx",
        "EventType": "OneShot,Loop",
        "Spatial3D": "true,false",
        "PausePolicy": "PauseWithGame,ContinueOnPause",
    }
    for column_name, values in validation_values.items():
        column = COLUMNS.index(column_name) + 1
        validation = DataValidation(type="list", formula1=f'"{values}"', allow_blank=False)
        validation.errorStyle = "stop"
        validation.showErrorMessage = True
        validation.showDropDown = False
        audio.add_data_validation(validation)
        validation.add(f"{get_column_letter(column)}2:{get_column_letter(column)}{len(rows) + 1}")
    for column, width in enumerate([24, 62, 14, 12, 10, 12, 10, 10, 18, 16, 10, 20, 16, 16], 1):
        audio.column_dimensions[get_column_letter(column)].width = width

    WORKBOOK.parent.mkdir(parents=True, exist_ok=True)
    wb.save(WORKBOOK)
    print(f"authored standalone audio workbook: {WORKBOOK}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
