# Plan 15: Character promotion

## Goal

Promote the initial girl after the fourth selected trait card. The build score selects one of four role systems with deterministic tie priority: physical Hunter, elemental Poet, survival Brave, then utility Sage.

## Runtime design

- Initial: 15 health, 5 physical attack, 5 elemental attack.
- Sage (`J_SPADE`): periodically receives one extra trait choice after each four normal selections.
- Hunter (`J_DIAMOND`): gains physical specialization plus 20% critical rate and 50% critical effect.
- Poet (`J_CLOVER`): fires deterministic pseudo-random water/fire/grass projectiles and gains 5% reaction efficiency after each cleared encounter.
- Brave (`J_HEART`): every second attack deals 50% extra damage and each cleared encounter opens light/medium/extreme forging before the normal trait choice.
- Promotion changes the live player portrait/model and is captured in subsequent echo recordings.

## Verification

- Compile the Editor target.
- Run trait, character-promotion, and element-reaction automation.
- Run static validation and `git diff --check`.

## Execution notes

- Implemented promotion scoring, one-time stat conversion, role visuals and the four role mechanics in mergeable C++.
- Reused the existing three-card widget for Brave forging; no binary assets were edited.
- UE 5.8 Editor target built successfully; all 13 `ReEcho.*` automation tests passed, including `PromotionRoles`, `BraveForge`, and `ElementReactions`.
- `python scripts/validate_project.py` and `git diff --check` passed.
- Planner review corrected runtime maximum health to the documented 15-point baseline and made Sage bonus choices wait for four normal choices rather than counting the previous bonus choice. Both behaviors now have regression coverage.
- Integration verification on 2026-08-05: ReEchoEditor built successfully; all 15 `ReEcho.*` tests, static validation, and `git diff --check` passed.
- Human PIE checks remain: role readability, forge pacing, Hunter crit feel, and Brave risk/reward tuning.
