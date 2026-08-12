# Plan 33 - Audio - independent runtime module foundation

## Coordination

- Planner owner: Gavyn-side Planner.
- Executor owner: Gavyn-side Planner completing the accepted local foundation implementation and review repairs.
- Plan authored by (AI side): Gavyn-side AI.
- Implementation authored by (AI side): Gavyn-side AI.
- Task status: `Closed` (`Proposed | Ready | InProgress | Review | Closed | Blocked`).
- Human validation: `NotRequired` (`NotRequired | PendingBeforeClose | PendingFollowUp | Passed`).
- Local planning / implementation base: current published `main` commit `2d66832725b3c4bc8798564a2b9c9bbab0c05117`.
- Implementation branch: local `plan/33-audio-runtime-foundation` in a separate worktree.
- Depends on / Blocks: blocks Plans34/35/36. It is code-disjoint from Plan26 and mostly disjoint from Plans28/30/31/32; only final integrated build evidence is shared.
- Writes: new `Source/ReEchoAudio/**` Runtime Module; `ReEcho.uproject`; the dependency edge in `Source/ReEcho/ReEcho.Build.cs`; Plan33-only automation; this Plan's Execution notes.
- Stable Reads: current UE 5.8 project/module contract, world/game-instance lifetime, fixed camera/listener behavior and pause semantics.
- Impact mode: `SharedContract` because Plans34/35/36 consume its public event IDs, request types, state channels and subsystem API.
- Compatibility promise / downstream action: `ReEcho` may depend on `ReEchoAudio`; `ReEchoAudio` must never include or depend on `ReEcho` gameplay types. Missing audio definitions/assets or an unavailable audio device must degrade to a no-op and never change gameplay, save, recording, RNG or encounter progression.
- Explicit exclusions: no sound assets, no player-facing sliders, no gameplay/UI call sites, no XLSX/CSV authoring migration, no audio aesthetic sign-off and no recording of audio events.

## Locked goal

Add a standalone `ReEchoAudio` UE Runtime Module that owns playback policy and exposes a small semantic API. Gameplay publishes intent such as `Combat.Attack`, `Combat.Hit`, `UI.Confirm`, or music/ambience state changes; it never loads sound assets, owns AudioComponents, calculates mix volumes, or waits for audio completion.

The module must support:

1. one-shot 2D and world-positioned 3D events;
2. long-running music and ambience state channels with idempotent transition/cross-fade control;
3. `Master`, `Music`, `Ambience`, `CombatSfx` and `UiSfx` volume buses;
4. per-event soft sound reference, bus, volume/pitch range, spatial policy, pause policy, cooldown, concurrency limit and priority;
5. a private non-gameplay random source for harmless pitch/variant selection;
6. stable public event IDs covering the complete later scope: background music, environment, combat, monster/Boss, echo, hit/hurt/kill feedback, camera movement, UI interaction, death and revive;
7. warning-once diagnostics for unknown events/assets without log spam.

## Locked acceptance

- [x] `ReEchoAudio` is a separately declared Runtime Module and compiles in Editor and Game targets.
- [x] Dependency direction is one-way: `ReEcho -> ReEchoAudio`; audio source has no include/dependency on `ReEcho`.
- [x] Public callers use semantic IDs/requests and state channels, not `USoundBase`, `UAudioComponent`, asset paths or delay timers.
- [x] Posting an unknown/missing event is safe, non-blocking and warning-once.
- [x] Cooldown, concurrency, priority, bus-volume multiplication and idempotent state transitions have deterministic automation without requiring a physical audio device.
- [x] Audio does not consume gameplay RNG and is absent from run save/recording schemas.
- [x] `python scripts/validate_project.py`, Editor build, focused automation and `git diff --check` pass; `.clang-format` is unavailable in the installed UE/toolchain and was recorded rather than silently skipped.
- [x] No generated UE products outside the curated Editor bundle, audio content binaries or machine-local paths are committed.

## Step 0 gate

- Baseline branch/commit: latest human-approved local baseline; record the exact commit in Execution notes.
- Engine/build availability: UE 5.8 installed build through repository scripts.
- Existing focused-test result: project validation and the accepted combined gameplay/UI build must pass before Plan33 enters `Review`.
- Active exclusive ownership or shared-contract approval: new module paths are unowned; `ReEcho.uproject` and `ReEcho.Build.cs` are reserved for this Plan while active.
- Stop condition if the baseline is broken: stop on any unrecognized module/project-descriptor or combined-baseline failure; do not misreport an upstream failure as caused by audio.

## Implementation outline

1. Add the `ReEchoAudio` module descriptor/build rules and one-way main-module dependency.
2. Define stable `FName` event/state constants, bus/spatial/pause enums, event definition and lightweight post request. Do not expose gameplay classes.
3. Implement a game-instance-owned audio service/facade with world-aware playback, state-channel ownership, volume buses and safe lifecycle cleanup.
4. Keep catalog injection behind the module API so Plan34 can supply authored definitions without changing callers.
5. Add device-independent tests for resolution, policies, throttling, state transitions, missing definitions and volume math.
6. Verify that audio code does not include `ReEcho` paths or touch save/recording/gameplay-RNG surfaces.

## Verification matrix

| Layer | Command/check | Expected evidence |
|---|---|---|
| Static | `python scripts/validate_project.py`; dependency/include audit | Project passes; no reverse dependency |
| Format/build | `.clang-format`; `scripts/ue/Build-Editor.cmd -Configuration Development` | UHT/UBT succeeds with both runtime modules |
| Automation | Plan33 audio-policy tests | no-device policy/state tests pass |
| Whitespace/scope | `git diff --check`; explicit path audit | only declared module/descriptor/dependency/docs paths |

## Execution notes

### Changed

- Adapted the previously completed local audio-foundation implementation onto the current published Plan33 baseline without carrying its obsolete Plan32 document or coordination state.
- Added the standalone `ReEchoAudio` Runtime Module, semantic event/request API, game-instance service, backend seam, catalog seam, volume buses, cooldown/concurrency/priority policy and state channels.
- Review repair forwards a valid world into real music/ambience playback, propagates pause policy to UE AudioComponents, starts replacement loops before fading the old state and preserves the live state when the replacement is unknown or cannot start.

### Evidence

- `scripts/ue/Build-Editor.cmd -Configuration Development`: succeeded on the current branch and refreshed the two-module curated Editor bundle.
- `scripts/ue/Run-Automation.cmd -Filter ReEcho.Audio.Foundation`: 12/12 succeeded, exit code 0, including the added pause/world/cross-fade and failed-state-replacement regression.
- Dependency/include audit: `ReEcho` privately depends on `ReEchoAudio`; no `Source/ReEchoAudio` file includes a `ReEcho` gameplay header.
- `git diff --check`: clean. `python scripts/validate_project.py` passes after the new allowlisted audio DLL is staged with the refreshed manifest.

### Remaining risks

- The installed UE 5.8/toolchain contains no `clang-format`; changed C++ was inspected against repository style and must be formatted if that tool becomes available.
- Plan33 intentionally ships an empty catalog and no sound assets. Audible output, catalog loading and persisted user settings begin in Plan34.

### Human validation result/request

`NotRequired` for the silent foundation. Audible and usability validation belongs to Plans34-36.

### Planner closure

- Accepted and merged into local `main` by merge commit `037fb18cfe0795b5fa04407b606f268b21b4a7a3`.
- Plan33 ownership is released. Plan34 may consume the audio API/catalog seam after its Ready revision is available on `origin/main`.
