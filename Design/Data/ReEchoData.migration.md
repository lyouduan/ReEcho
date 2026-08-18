# Plan25 XLSX Migration Report

Canonical workbook: `Design/Data/ReEchoData.xlsx`

Source reference workbook: `../回响肉鸽数值与构筑体系.xlsx`

## Authority Decision

The canonical workbook keeps the original Chinese sheet names and stable source provenance, but each production sheet now starts with its machine-exported Tables. The former side-by-side source blocks were removed after their prose was migrated by stable ID into the relevant `Description` columns. Every machine-exported table was reverse-seeded from the accepted production CSV, not from stale workbook values.

CSV remains the runtime package consumed by Unreal. The runtime does not read XLSX and has no Excel, COM, Office or Codex-private dependency.

## Export Coverage

`_ExportMap` owns exactly the 16 manifest production CSV files:

- `角色体系J`: `tblCharacters`, `tblCharacterAliases`
- `构筑体系G`: `tblCards`, `tblCardEffects`
- `元素体系Y`: `tblElements`, `tblReactions`
- `状态Z`: `tblStatuses`
- `武器体系W`: `tblWeaponTypes`, `tblWeapons`, `tblAttackSteps`
- `武器插槽C`: `tblSlotTypes`, `tblSlotProfiles`, `tblParts`, `tblPartEffects`
- `_SystemData`: `tblRuntimeSmoke`, `tblRuntimeSmokeEffects`

`属性S` remains a protected reference dictionary. `武器体系（废案）` was later removed from the canonical workbook after its ReferenceOnly role was retired. `怪物体系M` and `经济系统` remain ReferenceOnly and are not exported.

## Audited Source Differences

- Characters: source workbook IDs `J_01`-`J_04` are preserved as explicit aliases to canonical runtime IDs `J_SPADE`, `J_DIAMOND`, `J_CLOVER`, `J_HEART`. Runtime-only `J_CAT` remains present and enabled because it is accepted production data.
- Cards: source workbook contains broader design rows than the current six-card trait pool and forge rows. Accepted CSV enabled/review batches are preserved; unimplemented/design-review/reserved rows stay disabled with reasons.
- Characters/elements/reactions/statuses/weapon types: source prose is retained in the primary Table `Description` column. Runtime formulas and behavior still come only from registered typed fields; descriptions are not parsed to infer gameplay logic. The six ordered Plan23 reactions remain unchanged.
- Weapons/slots: Plan24 accepted stable `WeaponId`, legacy hotkeys, start-selectable order, `W_J_04`, `W_J_05`, `W_J_06`, attack steps and part batches are preserved. Part effects and source introductions are consolidated in `tblParts.Description`; the slot sheet keeps all 78 audited source rows, including 62 unnamed disabled rows with `PartId=None`.

## Verification Snapshot

- Initial `python scripts/data/sync_xlsx_to_csv.py --check` validates the workbook and matches all current production CSV bytes.
- Repeated generation from the same workbook is byte deterministic.
- The generator rejects formulas, unknown tables, path escapes, missing columns, bad types, foreign-key drift and unknown handlers before production publish.
- Transaction publish uses same-disk temp files, backups, a transaction marker and `os.replace`; automated fault injection proves rollback preserves production bytes.
