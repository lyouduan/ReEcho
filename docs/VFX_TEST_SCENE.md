# VFX Test Scene

Open `/Game/ReEcho/Testing/VFX/L_VFXAuthoring` and select **VFX Preview Rig - Production Read Only / Sandbox**.

## Production preview

Choose `RawNiagara`, `CombatSemantic`, `ElementSemantic`, or `WeaponProfile` in Details. Production modes resolve the same Catalog or Weapon Profile used by gameplay. Their asset mapping, trigger, carrier and lifecycle remain read-only; this scene never generates attacks, damage, AI, saves or recordings.

Move the `Source` and `Target` components, then use `Play Preview`, `Stop Preview`, `Restart Preview`, `Face Target`, or `Swap Source And Target`. A missing slot remains visibly `Missing`; it never borrows another character or weapon effect.

## Multi-target scenarios

The main workflow is the editable `Scenario Preset`: `SingleHit`, `Projectile`, `Conduct2Targets`, `ConductChain`, `RadiusBoundary`, `BurnState`, `Vaporize`, or `CrowdStress`. Use the labeled Scenario Target actors as editable anchors/reference hosts. `Run/Pause/Step/Restart/Reset`, `Add/Remove Target`, `Show Radius/Direction`, and `Generate Scenario Report` are editor-preview controls only.

`Authored Reaction Links` belongs only to yellow Visual Calibration and is **NOT APPLIED**. Production Conduct is explicitly PIE-only: enable `Run Production Conduct In PIE`, edit `Production Target Offsets`, and start PIE. The Rig spawns real Enemy/Combatant hosts, formally attaches Water with `ReEchoElementReaction::ApplyHitToWorld`, then applies Lightning to the primary. The production resolver reads CSV radius and world positions, publishes `FReEchoElementReactionResolvedEvent`, and each enemy's normal Combat VFX component consumes its authoritative `ReactionLinks`. Cyan radius/arrows display the captured formal event and update while targets move.

PIE starts in `Ready` and does not release automatically unless the optional compatibility flag is enabled. Click **释放特效 / Release VFX** or press `Space`; every release destroys the previous transient hosts/effects, recreates targets from the current offsets, and invokes the formal resolver again. The overlay reports Scenario, release count, resolved targets/links and `Ready/Playing/Missing/NOT APPLIED`. `R` restarts and releases, `T` resets targets without release, and `C` clears.

Set `Production Weapon Id` to an actual gameplay WeaponId to inspect its production `Conduct Link Propagation Delay Seconds`. The event snapshots that stable WeaponId; presentation resolves the matching CSV VisualKey and Weapon Presentation Profile. Missing identities/profiles diagnose and safely use `0`, preserving simultaneous playback. `Calibration Conduct Delay Override Seconds` is yellow sandbox data and always **NOT APPLIED**. Damage and the complete authoritative ReactionLinks resolve immediately; only Niagara link playback is staggered.

The test scene owns a transient copy of the production tilted orthographic defaults (`Location -900,0,900`, rotation `-45,0,0`, Ortho Width `2560`). It never writes the production Camera Blueprint/config. In PIE use `WASD` to pan, `Q/E` to rotate, the mouse wheel to zoom, and `Home` or **Reset Camera** to restore defaults. Camera Pan/Rotation/Ortho Width/Move Speed/Zoom Speed remain editable on the test Rig.

## Sandbox calibration

`Sandbox Transform`, sort priority and projectile display values affect only the disposable preview component. They are not written to a production Catalog, Profile, Gameplay Blueprint or Niagara asset. Record any proposed offset, rotation or scale together with the resolved semantic, asset path, authored axis, Local Space, bounds, renderer count, sort result and intended production owner.

All Visual Calibration fields are transient proposals and explicitly **NOT APPLIED**. Production Simulation exists only in PIE because CallInEditor does not provide the gameplay world/initialized Combatants required by the formal resolver. Pause/Step advance only the editor preview timeline and never become gameplay or damage authority.

## Validation workflow

Compare at least one selected Combat, Element and Weapon semantic in this map and through its real event in `/Game/Level00`. Check four/eight directions, size, anchor, bounds/culling, foreground and background occlusion, looping and cleanup. The test map is a calibration aid; only the real gameplay entry can validate event timing and lifecycle ownership.

The authoring script creates missing test assets once and preserves an existing artist-edited layout. The verification script is read-only. Neither script may write outside `/Game/ReEcho/Testing/VFX/**`.
