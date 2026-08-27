"""Make the four element-specific Gun flight systems follow their projectile component."""

import unreal


SYSTEM_PATHS = (
    "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fire_Fly",
    "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Thunder_Fly",
    "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Grass_Fly",
    "/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Water_Fly",
)

for path in SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing element-specific Gun Niagara system: {path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save element-specific Gun Niagara system: {path}")
    unreal.log(f"PLAN139_GUN_FLIGHT_LOCAL_SPACE_OK system={path}")
