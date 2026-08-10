# Plan 19: Trait draw presentation and post-draw shop flow

## Goal

Improve the three-choice trait draw screen and connect the completed draw sequence to the existing Time Shard shop before the next encounter.

## Runtime contract

- The draw screen clearly identifies trait versus forge choices, shows current Time Shards, and presents three centered readable cards.
- Cards remain disabled during the existing staggered reveal animation and clearly state that selection cannot be undone.
- Forge and Sage bonus choices finish their extra draw steps before opening the shop.
- After the final card choice, the existing shop opens in shop mode without briefly resuming combat.
- Closing the post-draw shop starts the next encounter; manually opened inventory/shop menus retain their existing close behavior.
- Purchases continue to use the existing guarded Time Shard and run-local inventory logic.

## Verification

- Format changed C++ files and run `git diff --check`.
- Build the Development Editor target with Unreal Editor closed.
- Run all `ReEcho.*` automation tests.
- Human PIE: inspect card layout/reveal at target resolution, select a card, buy an affordable item, close the shop, and verify the next encounter starts with the purchased effect.

## Execution notes

- Reworked the draw overlay into a centered three-card presentation with distinct card colors, a clear choice/forge title, current Time Shards, irreversible-choice copy, and the existing staggered reveal animation.
- The final draw now opens the existing shop while gameplay remains paused; closing that post-draw shop restores input and starts the next encounter. Forge and Sage bonus draws complete first.
- Removed the unused encounter-2/4 shop predicate so the runtime contract has one unconditional post-draw route.
- Added `ReEcho.Shop.PostDrawCurrencyCanPurchase` coverage for encounter reward, completed draw phase, and an immediate 15-shard purchase.
- Development Editor build, all 17 `ReEcho.*` automation tests, static validation, and `git diff --check` pass.
- Human PIE review remains for card layout, reveal pacing, shop transition, and purchased-effect readability.
