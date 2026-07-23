#| 2026-07-21 | Codex | Echo translucent visual | `Content/ReEcho/Materials/M_EchoGhost.uasset` | Completed | User-requested serialized material; generated through UE 5.8 Editor. |
 ReEcho planner exchange

This is a coordination log, not the permanent rulebook. Settled decisions must be copied into `PROJECT_RULES.md`, role rules, or `PROJECT_STATE.md`.

## Active ownership

| Date | Owner | Plan/branch | Files or exclusive resources | Status | Notes |
|---|---|---|---|---|---|
| 2026-07-21 | Codex | AI workflow framework | `shared/`, tool adapters, `scripts/`, plan template | Completed | No `.uasset`/`.umap`; static, build, and automation checks passed |

## Decisions

| Date | Decision | Destination | Status |
|---|---|---|---|
| 2026-07-21 | Use `shared/` as the only project workflow authority | `AGENTS.md`, `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | Treat UE binary assets as serially owned resources | `PROJECT_RULES.md` | Adopted |
| 2026-07-21 | Use explicit evidence levels and never promote static checks to build/PIE claims | `PROJECT_RULES.md`, `PROJECT_STATE.md` | Adopted |
| 2026-07-21 | Use `.cmd` wrappers so Windows policy remains unchanged | `scripts/README.md`, `PROJECT_RULES.md` | Adopted |

## Warnings / blocked items

| Date | Item | Owner needed | Resolution |
|---|---|---|---|
| 2026-07-21 | No arena map or end-to-end PIE loop exists | Future executor | Claim the `.umap`, implement milestone A content, then request human play test |

## Pending human decisions

- Decide whether the root blueprint documents should remain as provenance or be removed after the first baseline commit.


