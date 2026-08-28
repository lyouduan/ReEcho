"""Make NS_Echo_Chain update existing Beam particles from moving user endpoints."""

import unreal


ASSET_PATH = "/Game/VFX/Echo/Particle/NS_Echo_Chain"
system = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
if system is None:
    raise RuntimeError(f"Missing Niagara system: {ASSET_PATH}")
if not unreal.ReEchoCombatVfxComponent.ensure_niagara_update_beam_module(system):
    raise RuntimeError("Failed to add or verify Update Beam on every enabled Chain emitter")
if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
    raise RuntimeError(f"Failed to save Niagara system: {ASSET_PATH}")
unreal.log("PLAN142_CHAIN_UPDATE_BEAM_COMPLETE")
