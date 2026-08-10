# Plan 17: Start menu and resumable run save

## Goal

Show a blocking start panel before gameplay. A profile without a valid run save offers only a new game; a profile with a valid save offers continue and new game.

## Runtime contract

- New game clears the previous run and starts from encounter one.
- Continue restores the latest save: build, cards, inventory, Time Shards, encounter index, recordings and anchor.
- Regular progression writes safe checkpoints. Choosing "确认保存并退出" during an encounter additionally preserves encounter time, player transform/health, the in-progress recording, and every living enemy's transform/health/element/attack/fuse state.
- Transient projectiles and visual animation frames are intentionally not persisted; they are cleared across process restart.
- Card/forge decisions, purchases and completed encounters update the checkpoint.
- Failed and completed runs remove the resumable save.
- No binary assets are required; the panel is built in mergeable C++ like the existing pause menu.
- Esc opens the existing pause menu. Its exit action becomes a second confirmation step with only "继续游戏" and "确认保存并退出"; save failure keeps the game open.

## Verification

- Build the Editor target.
- Run all `ReEcho.*` automation, including save snapshot round-trip coverage.
- Run static validation and `git diff --check`.
- Human PIE: verify first launch, continue, overwrite with new game, exit confirmation/cancel, save failure safety, and a process-restart mid-encounter restore.

## Execution notes

- Added a blocking C++ start panel with save-aware new/continue actions and versioned `USaveGame` persistence.
- Added exact save-and-quit encounter snapshots for player transform/health/live stats/velocity, encounter clock, active recording and living enemy transform/health/element/attack/fuse/knockback state; safe checkpoint saves remain in use outside explicit encounter exit.
- Changed the Esc pause menu exit button into a safe two-step confirmation. Continue/Esc resumes immediately; confirmed exit writes successfully before calling platform quit.
- Restored the saved player weapon alongside character/build state. In-flight projectiles and presentation-only animation frames restart cleanly instead of being serialized.
- Full closed-editor `ReEchoEditor Win64 Development` build passed on 2026-08-10. All 16 `ReEcho.*` automation tests passed, including expanded `ReEcho.Run.SaveSnapshot` coverage; `git diff --check` passed.
- The reported `ReEchoAbilitySystemTests.cpp:54` crash was a Live Coding patch static automation registration failure. A full base-DLL build and test run load cleanly; do not Live Code this runtime module while automation tests are compiled in.
- Remaining human check: PIE the two confirmation buttons, verify saved health/positions/enemy state after a real quit/relaunch, and review Chinese glyph/DPI layout. Human acceptance is still pending.
