# Plan XX - <discipline> - <short name>

## Locked goal

Describe the player-visible or pipeline outcome. Changing this section requires planner/human agreement.

## Locked acceptance

- [ ] Functional result with observable evidence.
- [ ] Relevant automation/build check passes.
- [ ] Human PIE check is stated when feel/readability matters.
- [ ] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit:
- Engine version and build availability:
- Existing automated test result:
- Claimed merge-hostile assets/resources:
- Stop condition if the baseline is already broken:

## Implementation outline

Implementation details may be refined by the executor without changing the locked goal or acceptance.

1. Inspect the smallest relevant code/data surface.
2. Make one coherent change at a time.
3. Validate immediately after each risky change.
4. Update execution notes and release resource claims.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | JSON/config/source invariants pass |
| Build | `scripts/ue/Build-Editor.ps1` | UHT/UBT exit code 0 |
| Automation | `scripts/ue/Run-Automation.ps1 -Filter ReEcho` | Automation report passes |
| Human | PIE task | Feel/readability result recorded by human |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation requested
