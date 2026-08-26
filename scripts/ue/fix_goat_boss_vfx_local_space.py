"""Make Goat effects that attach to or follow runtime components use local space."""

import unreal


SYSTEM_PATHS = (
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Charging",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Charging",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Charging",
)

for path in SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing Goat Niagara system: {path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Goat Niagara system: {path}")
    unreal.log(f"GOAT_LOCAL_SPACE_OK system={path}")
