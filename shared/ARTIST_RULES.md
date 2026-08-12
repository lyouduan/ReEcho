# ReEcho artist-user route

Artist AI commits follow the identity rule in `shared/GIT_RULES.md`; this does not grant remote publication authority.

This file applies after the user confirms “美术”. An artist AI supports source art, presentation assets, UMG appearance, animation/audio assets and visual QA; it does not own gameplay logic or remote publication.

## Read route

1. Identify the exact target asset and its runtime consumer from `shared/CODEBASE_MAP.md`.
2. For UI/WBP work, read `Design/UI/ReEcho_UI修改指导.md` and the native parent header's binding contract.
3. Read only the matching visual/audio section of `shared/LESSONS.md`, the assigned Plan and live ownership row.

## Before editing

- Confirm the deliverable, target path, resolution/aspect/alpha or audio format, import settings, states/variants and human reference.
- Claim every publication-intended `.uasset`, `.umap`, canonical source-art file or other merge-hostile asset as `Active Exclusive` before writing.
- Use a local non-main specialist branch. For Unreal asset work, use the same-clone Unreal lock from `EXECUTOR_RULES.md`.

## Allowed work

- Create or revise reviewable source art/audio and import it through Unreal Editor or a reproducible import script.
- Adjust UMG hierarchy, anchors, Safe Zones, SizeBoxes, padding, styles, fonts, animation and presentation-only materials while preserving required widget names/types.
- Keep source files, imported targets and import settings traceable. Prefer deterministic scripts for repeatable bulk import.
- Run objective asset load/import checks, `CompileAllBlueprints` for WBP changes, affected build/cook checks and `git diff --check` as required.
- Ask the artist human to judge composition, readability, motion, color, sound character and final in-game presentation; AI screenshots or metrics are evidence, not aesthetic approval.

## Hard boundary

- Never hand-edit `.uasset`/`.umap` bytes, generated build products or runtime CSV.
- Do not put authoritative gameplay, save, currency, inventory, card, weapon or run-state logic in WBP/animation events.
- Do not rename/delete native-bound widgets, change stable IDs, collision/gameplay geometry, asset-loading contracts or C++ merely to make an asset fit.
- If new native bindings, runtime state, schema, collision behavior or audio-system code are needed, provide the programmer route with exact asset paths and desired behavior, then stop at that boundary.
- Do not push any remote ref or publish `main`; hand the local commit/assets and evidence to a programmer Planner.

## Handoff contents

- Source and imported asset paths, ownership status and exact changed variants.
- Technical specifications and import settings.
- Objective compile/load/cook results and known platform risks.
- Ordered in-game scenes/resolutions/states for human visual or audio acceptance.
- Any programmer dependency, stated without implementing it in the asset layer.
