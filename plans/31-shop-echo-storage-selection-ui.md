# Plan 31 - UI - shop echo storage and replay selection

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan31 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: current local `main` containing accepted Plan29 plus the locked Plan26/28/30 behavior contracts. Their implementations are integrated later by the Planner.
- Implementation branch: local `plan/31-shop-echo-selection-ui` in a separate clean worktree.
- Depends on / Blocks: may execute in parallel with Plans26/28/30 under explicit human approval. It consumes Plan29's accepted APIs and the locked Plan30 replay semantics; the Planner owns semantic integration of overlapping Widget/GameMode edits.
- Writes: `Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h`; `Source/ReEcho/Private/UI/ReEchoInventoryShopWidget.cpp`; `Source/ReEcho/Public/ReEchoGameMode.h`; `Source/ReEcho/Private/ReEchoGameMode.cpp`; Plan31-only UI/state tests where cheap; this Plan's Execution notes.
- Stable Reads: Plan29 read-only echo summaries and transactional store/skip/replace/select commands; Plan30 replay-limit semantics; current card-choice-to-shop transition.
- Impact mode: `SharedContract` coordinated overlap. Plan26/31 both edit InventoryShopWidget and Plans28/30/31 may edit GameMode in separate local worktrees; this is an explicit human-approved integration tradeoff, not an automatic-merge claim.
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

- Baseline branch/commit: exact current local `main` containing accepted Plan29; record it after the remote-difference gate. Do not consume or alter other Plans' WIP branches.
- Engine/build availability: human saves/closes Editor before build; installed UE 5.8 only.
- Existing focused-test result: Plan29 backend tests pass. Independently validate current shop behavior before editing; Plan26/30 WIP evidence is not available on this branch.
- Active exclusive ownership or shared-contract approval: the human explicitly approved parallel overlap with Plans26/28/30. Record every overlapping Widget/GameMode function changed for Planner integration.
- Stop condition if the baseline is broken: do not copy another worktree, duplicate backend state in the widget, remove locked Plan26/28/30 behavior, or invent the eventual card/shop acquisition source.

## Implementation outline

1. Read Plan26's locked weapon-presentation contract and Plan29's accepted public summaries/commands before designing the smallest additional panel/state flow; do not read or copy Plan26 WIP implementation.
2. Add pending store/skip/replace and stored-slot selection using stable GUID-backed view models/delegates.
3. Bridge commands in GameMode and gate existing shop close/next-encounter transition on a finalized storage decision. Record overlapping functions so the Planner can combine Plan28/30/31 behavior by contract.
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

Widget (`Source/ReEcho/Public/UI/ReEchoInventoryShopWidget.h` / `.cpp`):
- New public methods: `RefreshEchoState(UReEchoRunSubsystem*)`, `RequestStoreEcho`, `RequestSkipEcho`,
  `RequestReplaceEcho`, `RequestCancelReplaceEcho`, `ToggleReplaySelection`, `RequestClose`,
  `IsPendingEchoDecided`, `GetEchoSummary`, `GetEchoPendingDecision`, `GetCachedRunSubsystem`.
- New private state (summary + stable GUIDs only, never full recordings): `FReEchoEchoStorageSummary EchoSummary`,
  `TArray<FGuid> EchoSelection`, `TArray<FGuid> EchoSlotGuids`, `EReEchoShopEchoPendingDecision EchoPendingDecision`,
  `UReEchoRunSubsystem* CachedRunSubsystem`.
- New UI members: `EchoPanel` (UVerticalBox), `CloseConfirmWidget` (UVerticalBox), `EchoCapacityText`,
  `EchoReplayModeText`, `EchoPendingInfoText`, `EchoStoreButton`, `EchoSkipButton`, `EchoReplaceInstructionText`,
  `EchoCancelReplaceButton`, `EchoReplaceButtons[MaxStorageCapacity]`, `EchoSelectButtons[MaxStorageCapacity]`,
  `EchoSlotTexts[MaxStorageCapacity]`, `EchoSelectLabels[MaxStorageCapacity]`, `EchoSelectionText`.
- New handlers: `HandleStoreClicked`, `HandleSkipClicked`, `HandleCancelReplaceClicked`,
  `HandleReplaceSlot0/1/2Clicked`, `HandleSelectSlot0/1/2Clicked`, `HandleConfirmSkipContinueClicked`,
  `HandleConfirmReturnClicked`, `HandleReplaceSlotClicked(int32)`, `HandleSelectSlotClicked(int32)`, `BuildEchoPanel`.
- `HandleCloseClicked()` now calls `RequestClose()` (close gating) instead of broadcasting `OnClosed` directly.
- Delegates `OnClosed` / `OnPurchaseRequested` unchanged; purchases/inventory/currency preserved.

GameMode (`Source/ReEcho/Private/ReEchoGameMode.cpp` only; `ReEchoGameMode.h` untouched):
- `ShowInventoryShopMenu(...)`: after `InventoryShopWidget->AddToViewport(95)`, added
  `InventoryShopWidget->RefreshEchoState(RunSubsystem);` — the Plan31 bridge injecting the authoritative
  RunSubsystem into the widget. No other GameMode function changed.
- `HandleInventoryShopClosed`, `HandleShopPurchaseRequested`, `BeginNextEncounter`, `ShowTraitCardChoice`,
  `HandleTraitCardSelected`, and the close/input-restore flow are reused unchanged; each close triggers exactly
  one `BeginNextEncounter` (one-shot timer, `bContinueRunAfterShop` reset).

### Store / Skip / Replace / Selection state flow
- Shop open → `RefreshEchoState` reads `GetEchoStorageSummary()` and reconciles `EchoSelection` to `SelectedReplayIds`.
- Pending undecided → `EchoPendingInfoText` shows 遭遇编号 + 角色/武器 FName (no GUID to player);
  Store/Skip visible.
- `RequestStoreEcho`: free slot → `StorePendingRecording()` (Stored) + re-read; full → `Replacing` (show replace
  buttons + Cancel), pending stays undecided.
- `RequestReplaceEcho(TargetId)`: `StorePendingRecordingReplacing(TargetId)` (Stored) + re-read. No silent evict.
- `RequestCancelReplaceEcho`: back to `Undecided` (Return to selection).
- `RequestSkipEcho`: `SkipPendingRecordingStorage()` (Skipped) + re-read (rolling latest echo preserved).
- `ToggleReplaySelection(Id)`: limit 0 → ignored; limit 1 → single-select (replaces previous); limit >1 →
  multi up to limit, duplicates prevented; commits via `SetSelectedReplayIds`, then re-reads authoritative summary.
- Close → `RequestClose`: if `bHasPendingRecording`, show `CloseConfirmWidget`
  (`Skip and continue` / `Return to selection`); else `OnClosed.Broadcast()`.
  `Skip and continue` skips pending then broadcasts; `Return to selection` hides confirm and stays.

### Evidence
- `python scripts/validate_project.py`: passed.
- `scripts/ue/Build-Editor.cmd -Configuration Development`: BUILD_EXIT=0 (UHT/UBT clean, no warnings-as-errors).
- Automation `ReEcho.Shop.EchoSelection.Plan31`: `Result={Success}`.
- Existing `ReEcho.*` tests (Shop.PurchaseUpdatesInventory, Traits.*, Weapons.*) all `Result={Success}`;
  no `Result={Failure}` in the run.
- `git diff --check`: clean. Path audit: only the allowed files changed; no RunSubsystem/save/replay/asset touched.
- `clang-format`: binary not present in this environment (only `.clang-format` configs ship with UE 5.8);
  code follows the repo `.clang-format` style manually. No CI enforcement available locally.

### Remaining risks / overlaps
- Plan26 overlap: Plan26 also edits `InventoryShopWidget` for a read-only current-weapon label. Plan31 added a new
  `EchoPanel` and new functions only; it did NOT modify any Plan26-reserved slot/field or its planned label.
  Planner must merge: keep Plan26's weapon label intact and place `EchoPanel` without clobbering it. No overlapping
  Widget function was modified by Plan31.
- Plan28 overlap: Plan28 owns P input / pause UI and Player; Plan31 does not touch those. GameMode change is additive
  (one new call inside `ShowInventoryShopMenu`).
- Plan30 overlap: Plan30 owns specific-replay spawn inside `BeginNextEncounter`. Plan31 only sets selection via
  RunSubsystem `SetSelectedReplayIds` and reuses the existing close → `BeginNextEncounter` path; it does NOT read or
  implement Plan30 spawn logic (depends on the locked Plan30 contract).
- The echo panel is added at a fixed canvas anchor; layout/wording/click-flow/end-to-end PIE are for the user
  (executor performs no visual sign-off).

### Human validation result/request
- Status: `PendingBeforeClose`. User to run the PIE checklist (store / skip / full-replace-cancel / locked-specific
  replay / single / multi / purchases / close-once) and then report `Passed` or concrete rework.

### Planner integration review (rework required)
- The Plan30 and Plan31 commits are integrated on local `main`, but Plan31 remains `InProgress` and is not accepted.
- Show echo management only in the shop flow; the shared inventory-only screen must not expose intermission decisions.
- Treat RunSubsystem command results as authoritative. Store, skip and replace failures must not be presented as success.
- The confirm-and-continue path may broadcast `OnClosed` only after skip succeeds and the refreshed summary has no pending recording.
- Add focused non-visual tests for close gating, command-failure behavior and shop-versus-inventory visibility/state.
- Improve replacement-mode controls and player-facing wording without asking the executor to perform visual validation.
