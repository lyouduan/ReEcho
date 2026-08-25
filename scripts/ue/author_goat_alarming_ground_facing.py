"""Author the shared Goat alarming sprites with an explicit world-up ground normal."""

import unreal


ASSET_PATHS = [
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_BeAttacked",
]

for asset_path in ASSET_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Goat ground Niagara failed to load: {asset_path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_sprite_facing_owner_up(system):
        raise RuntimeError(f"Goat ground Niagara has no valid sprite renderer: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Goat ground Niagara: {asset_path}")
    unreal.log(f"GOAT_GROUND_FACING_OK asset={asset_path}")
