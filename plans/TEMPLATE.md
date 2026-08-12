# Plan XX - <discipline> - <short name>

## Coordination

- Planner owner:
- Executor owner:
- Plan authored by (AI side): `Gavyn-side AI | ReEcho teammate-side AI`.
- Implementation authored by (AI side): `Unassigned | Gavyn-side AI | ReEcho teammate-side AI`.
- Task status: `Proposed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `NotRequired` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base:
- Implementation branch:
- Depends on / Blocks:
- Writes:
- Stable Reads:
- Impact mode: `Isolated` (`Isolated | ReadOnly | SharedContract | Exclusive`).
- Compatibility promise / downstream action:
- Explicit exclusions:

## Locked goal

Describe the player-visible or pipeline outcome. Changing this section requires Planner/human agreement.

## Locked acceptance

- [ ] Functional result has observable evidence.
- [ ] Required automation/build checks pass.
- [ ] Human validation is requested only when feel, readability, visual quality or usability matters.
- [ ] No generated UE build products or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit:
- Engine/build availability:
- Existing focused-test result:
- Active exclusive ownership or shared-contract approval:
- Stop condition if the baseline is broken:

## Implementation outline

Implementation details may be refined without changing locked goal, acceptance or published contracts.

1. Inspect the smallest relevant code/data surface.
2. Make one coherent change at a time.
3. Validate immediately after each risky change.
4. Update Execution notes; record ownership/scope/contract changes locally before crossing the changed boundary.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project/source invariants pass |
| Build, when C++ changes | `scripts/ue/Build-Editor.cmd` | UHT/UBT exit code 0 |
| Automation, when runtime behavior changes | `scripts/ue/Run-Automation.cmd -Filter ReEcho` | Affected report passes |
| Human, when required | Named PIE/usability task | Human result or explicit follow-up deferral recorded |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request
