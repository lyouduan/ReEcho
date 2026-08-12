# ReEcho planner exchange

Live coordination only. Closed history belongs in Plans and Git; permanent rules belong in the authoritative rule file. Remove released ownership and stale refs promptly.

## Planned and active work announcements

| Plan | Planner | Task status | Human validation | Ref / base | Impact and Writes | Dependencies / other-Planner action |
|---|---|---|---|---|---|---|
| Shared settings UI + Plan 18 full-run weapon lock | ReEcho teammate-side Planner | `Closed` | `PendingFollowUp` | Local reviewed merge candidate based on current `origin/main`; remote publication awaits renewed candidate approval | Shared settings shell for start/pause menus; removes runtime weapon hotkeys/GAS abilities/run setter/recording and echo switch timeline while retaining pre-run selection, save restore and compatibility `InputSlot` data | Visual/menu PIE remains follow-up. Direct user decision supersedes historical Plan 18 switching. After publication, peer Planners fetch/rebase and must not restore switching from historical Plan 03/13/24 notes. |
| Plan 25 designer usability follow-up | Gavyn-side Planner | `Closed` | `PendingFollowUp` | Delivered on `origin/main`; QA uses a disposable local branch/clone | `ReadOnly` QA unless the human asks to keep workbook edits; no repository ownership is claimed by disposable tests | Designers follow `Design/Data/ReEchoData策划验收清单.md`. Publishable findings go to the current `WorkbookWriter`; do not hand-edit generated CSV. |
| Plan 26 weapon read-only UI consumers | ReEcho teammate-side Planner | `Ready` | `PendingBeforeClose` | New work starts from freshly fetched `origin/main`; an already-started branch from the former coordination ref must integrate current `origin/main` before `Review` | `ReadOnly` provider consumption plus isolated Writes to `UI/ReEchoInventoryShopWidget.*`, `UI/ReEchoStatsWidget.*` and Plan26-only UI tests | Consume typed weapon identity/display/query APIs. Do not edit data registry/reader/schema, canonical workbook or generated CSV. Rebroadcast before requesting a shared-contract change. |
| Plan 28 player automatic/manual attacks | Gavyn-side Planner | `Ready` | `PendingBeforeClose` | Planning ref `coord/gavyn-plan28-player-attack-modes`; implementation starts from freshly fetched, human-approved `origin/main` on `plan/28-player-attack-modes` | `Isolated` Writes to player attack-mode state, GameMode integration, attack-mode controls in the existing Esc pause widget, narrow additive range/target queries if needed and Plan28-only tests | Plan26 may continue. Executor stops at static/build/cheap deterministic checks and supplies a runnable handoff; the user owns PIE, interaction, readability and play-feel validation. Preserve existing pause actions, GAS/weapon execution, save/recording schemas and Echo behavior. |
| Plan 29 echo storage/save foundation | Gavyn-side Planner | `Ready` | `NotRequired` | Create `plan/29-echo-storage-foundation` from fetched `origin/coord/gavyn-echo-storage-replay` containing `a72928a`; that ref is current `origin/main` plus pure planning commits | `SharedContract` Writes to Core recording state, RunSubsystem, RunSaveGame and focused storage/save tests | May run beside Plans 26/28. Publishes pending/latest/stored/selected-ID state, capacity/capability APIs and v4 migration for Plans 30/31; do not merge other coordination refs and do not write GameMode/UI. |
| Plan 30 specific single/multi replay runtime | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Planning ref `coord/gavyn-echo-storage-replay`; future branch `plan/30-specific-echo-replay-runtime` | Replay resolver plus GameMode zero/one/many Echo spawning/resume and focused tests | Do not start until Plan29 is accepted and Plan28 releases GameMode. Limit 0 defaults to immediately previous encounter; positive limit uses only selected stored GUIDs. |
| Plan 31 shop echo storage/selection UI | Gavyn-side Planner | `Proposed` | `PendingBeforeClose` | Planning ref `coord/gavyn-echo-storage-replay`; future branch `plan/31-shop-echo-selection-ui` | Existing InventoryShopWidget plus GameMode bridge and cheap UI-state tests | Real write overlap with Plan26 and downstream of Plans 29/30. Do not start until all three are accepted/released; user owns visual and interaction validation. |

## Active ownership

| Owner | Plan/branch | Mode | State | Files or exclusive resource | Notes |
|---|---|---|---|---|---|
| ReEcho teammate-side Planner | Plan 26 / `plan/26-weapon-readonly-ui` | `Isolated` + `ReadOnly` | `Reserved` | Plan26 UI files and Plan26-only tests | Reservation does not block other disjoint/read-only work. It becomes `Active` when implementation starts. |
| Gavyn-side Planner | Plan 28 / `plan/28-player-attack-modes` | `Isolated` | `Reserved` | Player attack-mode state, GameMode integration, existing `ReEchoRestartWidget.*` pause UI and Plan28-only tests | Disjoint from Plan26. Additive weapon/enemy target queries are allowed only if live ownership remains clear; otherwise rebroadcast before expanding. |
| Gavyn-side Planner | Plan 29 / `plan/29-echo-storage-foundation` | `SharedContract` | `Reserved` | Core echo state, RunSubsystem, RunSaveGame and Plan29-only storage/save tests | Disjoint from Plans 26/28. Plans 30/31 consume the accepted contract; they must not start from the proposal alone. |

## Warnings / blocked items

- Fetch `origin/main` before new planning or integration. Historical `coord/...` refs are handoff evidence, not current baselines.
- `Design/Data/ReEchoData.xlsx` is a single-writer binary only when a publication-intended edit is active. Local throwaway usability testing does not lock it for the team.
- Runtime arena has no dedicated serialized test map; claim any `.umap` before replacing it.

## Pending human decisions

- None.
