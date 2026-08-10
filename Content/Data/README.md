# ReEcho data source

CSV is the target designer-editable runtime source. The legacy JSON files in this directory are migration-only review material until their domains are moved by later plans; do not add a second editable truth in JSON, C++, DeveloperSettings, Actors or Widgets.

## CSV contract v1

- Encoding: UTF-8 without BOM.
- Delimiter: comma. Quote cells with `"` when they contain a comma, quote or newline; escape a literal quote as `""`.
- Empty values: only allowed for fields marked optional in `csv_schema.csv`. Disabled character/card rows must still carry a `DisabledReason`.
- Booleans: lowercase `true` or `false`.
- Percentages: decimal values, so `0.20` means 20 percent.
- Units: distance columns include `Cm`; time columns include `Seconds`. Do not mix meters with Unreal centimeters, or milliseconds with seconds.
- IDs: stable IDs may contain letters, digits, `_`, `-` and `.` only. IDs cannot be blank or padded with whitespace.
- References: child tables must reference parent IDs exactly. `runtime_smoke_effects.csv.RuntimeRowId` references `runtime_smoke.csv.Id`; `character_aliases.csv.CanonicalCharacterId` references `characters.csv.Id`; `card_effects.csv.CardId` references `cards.csv.Id`.
- Value operations: only `Add`, `Multiply` and `Override` are valid.
- Logic hooks: CSV may name registered C++ `BehaviorId` and `EffectKind` values, but it does not execute expressions, scripts or formulas.

## Files

- `reecho_data_manifest.csv`: schema version and production CSV discovery.
- `csv_schema.csv`: human-readable and statically validated column contract.
- `runtime_smoke.csv`: minimal production runtime table used to prove CSV loading, packaging and value changes.
- `runtime_smoke_effects.csv`: one-to-many child table for typed numeric parameters.
- `characters.csv`: canonical playable/runtime character ids, display names, base stats, default weapons, appearance ids and registered passive behavior ids.
- `character_aliases.csv`: explicit legacy/workbook id mapping such as `J_01` to `J_SPADE`.
- `cards.csv`: canonical card/forge offer rows, source provenance, tags, promotion role bucket, offer group, review state and disabled reasons.
- `card_effects.csv`: ordered child effects for enabled cards and forge choices using typed targets, `EffectKind`, `ValueOp` and registered `BehaviorId`.
- `TestFixtures/CsvRuntime/`: positive and negative automation fixtures; not production data.

Run `python scripts/validate_project.py` before opening Unreal. It validates CSV schema, IDs, foreign keys, enum and behavior allowlists, disabled source rows, the six-card draw pool, UTF-8 and the expected negative fixtures.
