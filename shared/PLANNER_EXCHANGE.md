# ReEcho planner exchange

This is a coordination log, not the permanent rulebook. Settled decisions must be copied into `PROJECT_RULES.md`, role rules, or `PROJECT_STATE.md`.

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|
| 2026-07-21 | Codex | AI workflow framework | `shared/`, tool adapters, `scripts/`, plan template | Completed | Static, build, and automation checks passed |
| 2026-07-21 | Codex | Echo translucent visual | `Content/ReEcho/Materials/M_EchoGhost.uasset` | Completed | Generated through UE 5.8 Editor |
| 2026-07-23 | Codex | `plan/03-gas-player-abilities` | GAS/player C++, 2D character textures, collision/UI/menu code, config and plan/routing docs | Completed | Editor build, three automation tests, clean Windows Shipping Cook and launch smoke test passed; user requested merge |

## Decisions

| Date | Decision | Destination | Status |
|---|---|---|---|
| 2026-07-21 | Use `shared/` as the only project workflow authority | `AGENTS.md`, `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | Treat UE binary assets as serially owned resources | `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | Use explicit evidence levels and never promote static checks to build/PIE claims | `PROJECT_RULES.md`, `PROJECT_STATE.md` | Adopted |
| 2026-07-21 | Use `.cmd` wrappers so Windows policy remains unchanged | `scripts/README.md`, `PROJECT_RULES.md` | Adopted |
| 2026-07-23 | Synchronize relevant Markdown before every commit | `PROJECT_RULES.md`, `EXECUTOR_RULES.md`, `PLANNER_RULES.md` | Adopted |

## Warnings / blocked items

| Date | Item | Owner needed | Resolution |
|---|---|---|---|
| 2026-07-23 | Runtime arena has no committed project `.umap` | Future planning | Current runtime-generated arena is playable; claim serialized map ownership before replacing it |

## Pending human decisions

- Decide whether the root blueprint documents should remain as provenance or be removed after the first baseline commit.