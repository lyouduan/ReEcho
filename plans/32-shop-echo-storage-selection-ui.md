# Plan 32 - UI - shop echo storage and replay selection

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Plan32 Executor.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `InProgress` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `PendingBeforeClose` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: old UI behavior reference `73a8ed6`; acquisition behavior reference `e2b1047`; final implementation must adapt onto current `origin/main` containing Plans28/30/31/37.
- Implementation branch: continue local `plan/32-shop-echo-selection-ui`; do not merge either historical reference commit wholesale.
- Depends on / Blocks: may execute in parallel with Plans26/28/30 under explicit human approval. It consumes Plan30's accepted APIs and the locked Plan31 replay semantics; the Planner owns semantic integration of overlapping Widget/GameMode edits.
- Writes: typed inventory/shop opening context; `UReEchoEchoManagementWidget`; GUID-backed snapshot/entry types; `WBP_ReEchoEchoManagementPanel` and `WBP_ReEchoStoredEchoEntry` embedded under `WBP_ReEchoInventoryShopScreen`; narrow intermission orchestration; shop catalog and transactional purchase handling for the one-time specific-replay unlock; Plan32/shop tests and notes. The latest UI Manager/Flow Coordinator remains authoritative.
- Stable Reads: Plan30 read-only echo summaries and transactional store/skip/replace/select commands; Plan31 replay-limit semantics; current card-choice-to-shop transition.
- Impact mode: `SharedContract` coordinated overlap. Plan26/31 both edit InventoryShopWidget and Plans28/30/31 may edit GameMode in separate local worktrees; this is an explicit human-approved integration tradeoff, not an automatic-merge claim.
- Compatibility promise / downstream action: preserve Plan26 weapon presentation and existing inventory/shop purchases, currency, card-draw transition and close-to-next-encounter behavior. Never read or mutate RunSubsystem arrays directly.
- Explicit exclusions: no backend storage/save/replay redesign, no card-based acquisition, no CSV/XLSX, no weapon UI regression work, no new top-level Screen, no restoration of the old code-built shop layout, no new art and no Executor visual sign-off.

## Locked goal

Extend the existing shop screen with one coherent echo-management flow after each successful non-final encounter:

1. Present the just-completed pending recording and require an explicit `Store` or `Skip` decision.
2. If storing with free capacity, add it. If full, require choosing a stored echo to replace or cancel storing; never evict silently.
3. Show stored echoes using stable human-readable encounter metadata and stable GUID-backed selection, not array indices.
4. When specific replay is available, allow selection up to the current limit. Limit one is the specific-single UI; a larger limit is the same multi-select UI. When unavailable, clearly show that the next encounter will automatically replay the immediately previous encounter.
5. Closing the shop finalizes the pending decision and the next-encounter replay selection before the current `BeginNextEncounter` handoff.

The current prototype exposes storage every intermission and supports up to three stored/selected echoes. Add the previously approved one-time shop item `SHOP_REPLAY_UNLOCK`: it costs 30 Time Shards and changes `SpecificReplayLimit` from 0 to the existing maximum of 3. Insufficient funds or repeat purchase must reject atomically without spending currency or changing the limit. A successful purchase is saved immediately and refreshes both shop ownership/currency and the post-trait echo-management selection state. Card-based acquisition remains deferred.

The shared inventory/shop screen uses a typed `Inventory`, `ManualShop` or `PostTraitIntermission` context. Echo management is visible only for `PostTraitIntermission`; it never appears in the M-key manual shop or pure inventory view.

## Locked acceptance

- [ ] Existing shop purchases, inventory presentation, Time Shards and Plan26 weapon label remain functional and visually separate from echo management.
- [ ] Pending recording displays at least encounter number and enough build/weapon identity to distinguish candidates without exposing GUIDs to the player.
- [ ] Store, skip and replace are explicit. Closing with an undecided pending recording opens a two-choice confirmation (`Skip and continue` / `Return to selection`); it never silently loses the recording.
- [ ] Stored slots accurately show used/capacity state. Replacement updates the view immediately and removes any now-stale replay selection transactionally.
- [ ] Replay selection enforces unavailable/one/many limits, prevents duplicates and clearly marks the selected set. Unavailable mode communicates automatic previous-encounter replay.
- [ ] The shop catalog exposes one `SHOP_REPLAY_UNLOCK` item costing 30 Time Shards. A successful first purchase deducts exactly 30, records ownership, changes the specific-replay limit from 0 to 3, saves immediately and refreshes the current shop/echo panel without closing it.
- [ ] Insufficient funds and repeat purchase reject atomically: no currency loss, duplicate inventory entry, limit change or false success. Save/Continue preserves both ownership and the unlocked limit. Ordinary shop items still purchase normally.
- [ ] Store/replace/skip/select checks the authoritative transaction result, saves immediately only after success, refreshes the snapshot and never reports success after a rejected command.
- [ ] Close eligibility is derived from authoritative pending/selection state, not from a confirmation control's existence. Skip-and-continue closes only after successful Skip and refreshed pending-clear evidence.
- [ ] Shop close passes through existing modal/input restoration and starts the next encounter exactly once through an idempotent/in-progress transition. Reopening before close reflects authoritative RunSubsystem state.
- [ ] UI delegates pass stable GUIDs/commands; widget code never owns full recordings, writes save state or indexes directly into mutable RunSubsystem arrays.
- [ ] Executor stops after formatting, project validation, Editor build and cheap state/delegate tests, then supplies a short runnable handoff. The user performs layout, wording, click flow and end-to-end PIE validation.
- [ ] No generated products, machine-local paths or out-of-scope assets are committed.

## Step 0 gate

- Baseline branch/commit: exact current local `main` containing accepted Plan30; record it after the remote-difference gate. Do not consume or alter other Plans' WIP branches.
- Engine/build availability: human saves/closes Editor before build; installed UE 5.8 only.
- Existing focused-test result: Plan30 backend tests pass. Independently validate current shop behavior before editing; Plan26/30 WIP evidence is not available on this branch.
- Active exclusive ownership or shared-contract approval: the human explicitly approved parallel overlap with Plans26/28/30. Record every overlapping Widget/GameMode function changed for Planner integration.
- Stop condition if the baseline is broken: do not copy another worktree, duplicate backend state in the widget, remove locked Plan26/28/30 behavior, or cherry-pick the historical `e2b1047` widget callback rewrite. Reuse only its locked purchase semantics through the current indexed shop UI and authoritative RunSubsystem transaction path.

## Implementation outline

1. Extend the shared shop screen with a typed opening context and embed a dedicated echo-management child Widget; do not add a second top-level screen.
2. Add pending store/skip/replace and stored-slot selection using stable GUID-backed snapshots and dynamic entries.
3. Route typed commands through a narrow intermission coordinator or equivalent GameMode seam, check results, save immediately after success, refresh state and gate closing from authoritative data.
4. Add the one-time 30-shard specific-replay unlock to the current generic shop catalog/purchase transaction. On success, persist and refresh the current shop plus echo summary; do not special-case old fixed button callbacks.
5. Make close-to-next-encounter one-shot, then add deterministic transaction/delegate/purchase coverage and hand visual/usability validation to the user.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py` | Project invariants pass |
| Format/build | `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | Intended formatting and UHT/UBT success |
| Focused automation | Plan32 state/delegate tests plus affected shop/save tests | No duplicate close/start; limits, command wiring, atomic purchase and Continue persistence pass |
| Whitespace/scope | `git diff --check` and path audit | Plan26 preserved; no backend duplication/assets |
| Human | User PIE: store/skip/free/full/replace, unavailable/single/multi selection, purchases and next encounter | User reports `Passed` or concrete rework |

## Execution notes

### Planner review finding (2026-08-12, rework required)

#### Latest user scope correction: restore shop acquisition

- The previous exclusion of acquisition is superseded by the user's direct report that the Plan31 shop unlock must be present. Implement the locked `SHOP_REPLAY_UNLOCK` semantics above as part of Plan32, because Plan32 already owns the shop/echo selection surface.
- Use `e2b1047` only as a behavior reference. Do not cherry-pick it or restore its obsolete fixed `HandleOffer0..7` widget callbacks; the current indexed shop buttons already support an additional catalog entry.
- The final Plan32 candidate must be ported/adapted onto current `origin/main`; the old Plan32 branch contains valid local work but cannot be merged wholesale if it would remove Plan28, Plan37 or current rule/publication changes.

- The UMG-adapted candidate at `73a8ed6` preserves UI Flow ownership and implements the correct typed post-trait context, authoritative GameMode transactions and one-shot close orchestration. It is the only implementation reference for the rework; do not merge the old code-built `plan/31-shop-echo-selection-ui` branch.
- The locked `WBP_ReEchoEchoManagementPanel` and `WBP_ReEchoStoredEchoEntry` assets do not exist. Both native widgets still construct their full presentation in `BuildWidgetTree`, so the echo-management presentation is not editable through the UMG assets promised by this Plan and partially restores code-owned layout inside a WBP page.
- The integrated candidate also dropped the old focused transaction/close tests. `ReEcho.UI.IntermissionContexts` checks only the three opening modes; it does not prove rejected Store/Skip/Replace results, stable-GUID selection limits, pending close confirmation, skip failure, save behavior or close-once behavior.
- Required rework: start from current published `origin/main`, port only the Plan32 typed UMG/intermission behavior from `73a8ed6`, create the two promised child WBP assets with native fallback kept minimal, and restore cheap non-visual coverage for the authoritative command and close gates. Do not include Plan28 code or the old fixed-button callback rewrite. The current locked `SHOP_REPLAY_UNLOCK` acquisition requirement above supersedes the historical acquisition deferral.
- Return to `Review` only after static validation, Editor build, Blueprint compilation, focused/affected automation and `git diff --check` pass. PIE and visual acceptance remain with the user.

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
  `InventoryShopWidget->RefreshEchoState(RunSubsystem);` — the Plan32 bridge injecting the authoritative
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
- Automation `ReEcho.Shop.EchoSelection.Plan32`: `Result={Success}`.
- Existing `ReEcho.*` tests (Shop.PurchaseUpdatesInventory, Traits.*, Weapons.*) all `Result={Success}`;
  no `Result={Failure}` in the run.
- `git diff --check`: clean. Path audit: only the allowed files changed; no RunSubsystem/save/replay/asset touched.
- `clang-format`: binary not present in this environment (only `.clang-format` configs ship with UE 5.8);
  code follows the repo `.clang-format` style manually. No CI enforcement available locally.

### Remaining risks / overlaps
- Plan26 overlap: Plan26 also edits `InventoryShopWidget` for a read-only current-weapon label. Plan32 added a new
  `EchoPanel` and new functions only; it did NOT modify any Plan26-reserved slot/field or its planned label.
  Planner must merge: keep Plan26's weapon label intact and place `EchoPanel` without clobbering it. No overlapping
  Widget function was modified by Plan32.
- Plan28 overlap: Plan28 owns P input / pause UI and Player; Plan32 does not touch those. GameMode change is additive
  (one new call inside `ShowInventoryShopMenu`).
- Plan31 overlap: Plan31 owns specific-replay spawn inside `BeginNextEncounter`. Plan32 only sets selection via
  RunSubsystem `SetSelectedReplayIds` and reuses the existing close → `BeginNextEncounter` path; it does NOT read or
  implement Plan31 spawn logic (depends on the locked Plan31 contract).
- The echo panel is added at a fixed canvas anchor; layout/wording/click-flow/end-to-end PIE are for the user
  (executor performs no visual sign-off).

### Human validation result/request
- Status: `PendingBeforeClose`. User to run the PIE checklist (store / skip / full-replace-cancel / locked-specific
  replay / single / multi / purchases / close-once) and then report `Passed` or concrete rework.

### Planner integration review (rework completed)
- Integration rework pass executed on the merged Plan31+31 baseline (`plan/32-shop-echo-selection-ui`, HEAD after the local main merge). All planner-review items closed:
- **Shop-only echo management**: `GameMode::ShowInventoryShopMenu` now calls `Widget->SetEchoManagementEnabled(bShowShop)` explicitly; the widget never guesses the mode. `RefreshEchoState` returns early (collapses `EchoPanel`, skips the close gate) when management is disabled, so the plain inventory screen never shows intermission decisions or the pending-recording gate.
- **Backend result is the only success basis**: `RequestStoreEcho` / `RequestSkipEcho` / `RequestReplaceEcho` now branch on the returned `EReEchoEchoStorageResult`. `Success` is required to mark `Stored`/`Skipped`; on `StorageFull` `RequestStoreEcho` enters `Replacing` (no silent evict); any other failure keeps the decision `Undecided`, re-reads the authoritative summary and shows a short failure line (`EchoStatusText`). `ToggleReplaySelection` still commits only when `SetSelectedReplayIds` returns `Success`.
- **Close gate fixed**: `RequestClose` only opens the confirm overlay when management is enabled AND a pending recording exists. `HandleConfirmSkipContinueClicked` first executes the skip; it broadcasts `OnClosed` only if the skip returned `Success` and the re-read summary has no pending recording. On failure it keeps the shop open and shows the failure state. `Return to selection` only hides the confirm box (no skip, no close). A `bEchoCloseBroadcasted` guard ensures `OnClosed` fires at most once per close operation.
- **Replace mode + wording**: `EchoSlotGuids` is rebuilt before the replace buttons are shown; entering `Replacing` hides the normal Store/Skip buttons; cancel returns to the normal decision. Internal field names (e.g. `SpecificReplayLimit`) are never shown; player wording uses "尚未获得指定回放能力，下一场自动回放上一场" / "最多选择 N 个回响在下一场回放" / "容量已满，请选择要替换的已存储回响". No GUIDs shown.
- **Unity Build fix**: `CreateStartedRun` in `ReEchoEchoReplayRuntimeTests.cpp` renamed to `CreateStartedReplayRun` to avoid the anonymous-namespace collision with `ReEchoEchoStorageTests.cpp`.
- **Extended non-visual tests** (`ReEchoShopEchoSelectionTests.cpp`): inventory mode disabled vs shop mode enabled; backend Store/Skip/Replace failures do not produce a false `Stored`/`Skipped` state; pending-close enters confirm; skip success closes once; skip failure does not close; return-to-selection does not close; full-storage enter-replace / invalid-target / cancel / valid-replace; limit 0 / limit 1 single-select / limit N multi-select and invalid-GUID rejection. No screenshots or visual assertions.

### Evidence (rework pass)
- `python scripts/validate_project.py`: passed.
- `scripts/ue/Build-Editor.cmd -Configuration Development`: BUILD_EXIT=0 (UHT/UBT clean). Adaptive non-unity compiled `ReEchoInventoryShopWidget.cpp`, `ReEchoGameMode.cpp`, `ReEchoEchoReplayRuntimeTests.cpp`, `ReEchoShopEchoSelectionTests.cpp`.
- Automation `ReEcho.Shop.EchoSelection.Plan32`: `Result={Success}`.
- Affected `ReEcho.*` tests (Plan30 `EchoStorage*`, Plan31 `EchoReplayResolver.*`, Shop `PurchaseUpdatesInventory` / `PostDrawCurrencyCanPurchase`, Traits/Weapons) all `Result={Success}`; no `Result={Failure}` in the run.
- `git diff --check`: clean. Path audit: only the allowed files changed; `shared/LESSONS.md` left dirty/untouched/unstaged per Planner audit; no RunSubsystem/save/replay/asset touched.
- `clang-format`: binary not present locally; code follows the repo `.clang-format` style manually.
