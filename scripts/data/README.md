# ReEcho XLSX to CSV authoring

策划配表请先阅读 [`Design/Data/ReEchoData使用说明.md`](../../Design/Data/ReEchoData使用说明.md)。本文只保留生成工具的快速命令。

Canonical authoring workbook:

```powershell
Design\Data\ReEchoData.xlsx
```

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
