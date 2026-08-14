# ReEcho XLSX to CSV authoring

策划配表请先阅读 [`Design/Data/ReEchoData使用说明.md`](../../Design/Data/ReEchoData使用说明.md)。首次拉仓库和正式验收另见 [`Design/Data/ReEchoData策划验收清单.md`](../../Design/Data/ReEchoData策划验收清单.md)。本文只保留生成工具的快速命令。

Canonical authoring workbooks:

```powershell
Design\Data\ReEchoData.xlsx
Design\Data\ReEchoEnemyData.xlsx
```

怪物策划只编辑独立工作簿；其说明与验收清单见 [`ReEchoEnemyData使用说明.md`](../../Design/Data/ReEchoEnemyData使用说明.md) 和 [`ReEchoEnemyData策划验收清单.md`](../../Design/Data/ReEchoEnemyData策划验收清单.md)。统一同步入口会联合验证两个工作簿的独立 ExportMap。

Install the locked XLSX dependency once in the Python environment used for repository tooling:

```powershell
python -m pip install -r scripts\data\requirements.txt
```

Fixed commands:

```powershell
python scripts\data\sync_xlsx_to_csv.py
python scripts\data\sync_xlsx_to_csv.py --check
python scripts\data\sync_xlsx_to_csv.py --sheet "武器体系W"
```

Unreal runtime reads only generated UTF-8 CSV under `Content/Data`; it never reads XLSX and never requires Excel, COM, Office, or Codex runtimes.
