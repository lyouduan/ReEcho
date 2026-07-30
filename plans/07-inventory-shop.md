# Plan 07 — Inventory and shop

## Goal

Integrate the supplied inventory and shop designs as functional in-game menus backed by run-state currency and owned-item data.

## Acceptance

- `B` toggles inventory and `M` toggles shop; either menu pauses combat and releases the mouse.
- Supplied artwork is imported as cooked Texture2D assets and used as the full-screen menu presentation.
- Shop purchases consume Time Shards, cannot be duplicated, enter the inventory, and apply their configured build effect.
- Inventory lists owned items and their effects.
- Escape closes an open inventory/shop before opening the pause menu.
- Editor build, ReEcho automation, static validation, and `git diff --check` pass.
- Human PIE validation confirms alignment, text readability, and mouse interaction.

## Implementation

- Extend `UReEchoRunSubsystem` with run-local inventory state and a guarded purchase API.
- Add a native `UReEchoInventoryShopWidget` with inventory/shop modes over the supplied art.
- Let `AReEchoGameMode` own menu lifecycle, pause state, and purchase handling.
- Bind the menu actions from `AReEchoPlayerPawn` and `DefaultInput.ini`.
- Import source images through a reproducible UE Python script.

## Execution notes

- Imported the supplied inventory and shop PNGs as cooked `/Game/ReEcho/Textures/UI/InventoryBackground` and `ShopBackground` assets while retaining reviewable sources under `Content/SourceArt/UI/`.
- Added a native full-screen inventory/shop widget, B/M paused-menu input, Escape-first-close behavior, and GameMode-owned menu lifecycle.
- Added a shared four-item shop catalog, run-local inventory state, Time Shard deduction, duplicate purchase protection, and immediate build effects.
- Added `ReEcho.Shop.PurchaseUpdatesInventory`; all six ReEcho automation tests pass.
- ReEchoEditor Win64 Development, asset import commandlet, `scripts/validate_project.py`, and `git diff --check` pass.
- Remaining risk: human PIE validation is required for overlay alignment, DPI scaling, Chinese glyph coverage, and mouse interaction over the supplied compositions.
