"""Make attached Echo aura Niagara emitters follow their owning Echo component."""

"""Repair authored Echo aura Niagara emitters to use local space."""

import unreal


SYSTEM_PATHS = (
    "/Game/VFX/Echo/Particle/NS_Echo_Water",
    "/Game/VFX/Echo/Particle/NS_Echo_Grass",
)

for path in SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing Echo aura Niagara system: {path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Echo aura Niagara system: {path}")
    unreal.log(f"ECHO_AURA_LOCAL_SPACE_OK system={path}")
