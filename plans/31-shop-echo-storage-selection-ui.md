# Plan 31 - UI - shop echo storage and replay selection

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Unassigned; do not start while status is `Proposed`.
- Task status: `Proposed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: future local branch from a reviewed base containing accepted Plans 26, 29 and 30.
- Implementation branch: future `plan/31-shop-echo-selection-ui` in a separate clean worktree.
- Depends on / Blocks: blocked on Plan26 release/integration of `ReEchoInventoryShopWidget.*`, Plan29 storage APIs and Plan30 replay behavior. It blocks no disjoint backend work.
- Writes: `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`; `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`; `Source/ReEcho/Public/ReEchoGameMode.h`; `Source/ReEcho/Private/ReEchoGameMode.cpp`; Plan31-only UI/state tests where cheap; this Plan's Execution notes.
- Stable Reads: Plan29 read-only echo summaries and transactional store/skip/replace/select commands; Plan30 replay-limit semantics; current card-choice-to-shop transition.
- Impact mode: `Isolated` after Plan26 ownership release. Until then it has a real overlapping write and remains Proposed.
- Compatibility promise / downstream action: preserve Plan26 weapon presentation and existing inventory/shop purchases, currency, card-draw transition and close-to-next-encounter behavior. Never read or mutate RunSubsystem arrays directly.
- Explicit exclusions: no backend storage/save/replay redesign, no acquisition source in cards/shop, no CSV/XLSX, no weapon UI regression work, no `.uasset`/`.umap` or new art, and no Executor visual sign-off.

## Locked goal

Extend the existing shop screen with one coherent echo-management flow after each successful non-final encounter:

1. Present the just-completed pending recording and require an explicit `Store` or `Skip` decision.
2. If storing with free capacity, add it. If full, require choosing a stored echo to replace or cancel storing; never evict silently.
3. Show stored echoes using stable human-readable encounter metadata and stable GUID-backed selection, not array indices.
4. When specific replay is available, allow selection up to the current limit. Limit one is the specific-single UI; a larger limit is the same multi-select UI. When unavailable, clearly show that the next encounter will automatically replay the immediately previous encounter.
5. Closing the shop finalizes the pending decision and the next-encounter replay selection before the current `BeginNextEncounter` handoff.

The current prototype exposes storage every intermission and supports up to three stored/selected echoes. The eventual card/shop acquisition presentation is explicitly deferred; the UI must already respond correctly when the backend reports specific replay unavailable, single or multi.

## Locked acceptance

- [ ] Existing shop purchases, inventory presentation, Time Shards and Plan26 weapon label remain functional and visually separate from echo management.
- [ ] Pending recording displays at least encounter number and enough build/weapon identity to distinguish candidates without exposing GUIDs to the player.
- [ ] Store, skip and replace are explicit. Closing with an undecided pending recording opens a two-choice confirmation (`Skip and continue` / `Return to selection`); it never silently loses the recording.
- [ ] Stored slots accurately show used/capacity state. Replacement updates the view immediately and removes any now-stale replay selection transactionally.
- [ ] Replay selection enforces unavailable/one/many limits, prevents duplicates and clearly marks the selected set. Unavailable mode communicates automatic previous-encounter replay.
- [ ] Shop close passes through existing modal/input restoration and starts the next encounter exactly once. Reopening before close reflects authoritative RunSubsystem state.
- [ ] UI delegates pass stable GUIDs/commands; widget code never owns full recordings, writes save state or indexes directly into mutable RunSubsystem arrays.
- [ ] Executor stops after formatting, project validation, Editor build and cheap state/delegate tests, then supplies a short runnable handoff. The user performs layout, wording, click flow and end-to-end PIE validation.
- [ ] No generated products, machine-local paths or out-of-scope assets are committed.

## Step 0 gate

- Baseline branch/commit: accepted/integrated Plans 26, 29 and 30; record exact base after a fresh external-commit audit.
- Engine/build availability: human saves/closes Editor before build; installed UE 5.8 only.
- Existing focused-test result: Plan29/30 backend tests and Plan26 UI base checks pass.
- Active exclusive ownership or shared-contract approval: Plan26 must release `ReEchoInventoryShopWidget.*`; Plan28/30 GameMode ownership must also be released.
- Stop condition if the baseline is broken: do not overwrite Plan26, duplicate backend state in the widget, or invent the eventual card/shop acquisition source.

## Implementation outline

1. Read Plan26's final widget and Plan29's final public summaries/commands before designing the smallest additional panel/state flow.
2. Add pending store/skip/replace and stored-slot selection using stable GUID-backed view models/delegates.
3. Bridge commands in GameMode and gate existing shop close/next-encounter transition on a finalized storage decision.
4. Add only cheap deterministic state/delegate coverage; compile, then hand the user a concise shop and next-encounter test checklist.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project invariants pass |
| Format/build | `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | Intended formatting and UHT/UBT success |
| Focused automation | Plan31 state/delegate tests plus affected shop tests | No duplicate close/start, limits and command wiring pass |
| Whitespace/scope | `git diff --check` and path audit | Plan26 preserved; no backend duplication/assets |
| Human | User PIE: store/skip/free/full/replace, unavailable/single/multi selection, purchases and next encounter | User reports `Passed` or concrete rework |

## Execution notes

### Changed

### Evidence

### Remaining risks

### Human validation result/request
