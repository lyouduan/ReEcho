"""Keep delivered Bow flight and Longsword slash element systems in component-local space."""

import unreal


SYSTEMS = (
    ("Fire", "Fire"),
    ("Thunder", "Thunder"),
    ("Grass", "Grass"),
    ("Water", "Water"),
)

SWORD_SYSTEM_PATHS = tuple(
    f"/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_{suffix}" for suffix, _ in SYSTEMS
)
SICKLE_SYSTEM_PATHS = tuple(
    f"/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_{suffix}" for suffix, _ in SYSTEMS
)

for source_suffix, destination_suffix in SYSTEMS:
    source = f"/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_{source_suffix}"
    destination = f"/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_{destination_suffix}"
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        system = unreal.EditorAssetLibrary.load_asset(destination)
    else:
        if not unreal.EditorAssetLibrary.does_asset_exist(source):
            raise RuntimeError(f"Missing delivered Bow element Niagara system: {source}")
        if not unreal.EditorAssetLibrary.rename_asset(source, destination):
            raise RuntimeError(f"Failed to move Bow element Niagara system: {source} -> {destination}")
        system = unreal.EditorAssetLibrary.load_asset(destination)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Bow element asset is not a Niagara system: {destination}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {destination}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Bow element Niagara system: {destination}")
    unreal.log(f"BOW_ELEMENT_FLIGHT_OK system={destination}")

for path in SWORD_SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing element-specific Longsword Niagara system: {path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Longsword element Niagara system: {path}")
    unreal.log(f"LONGSWORD_ELEMENT_SLASH_OK system={path}")

for path in SICKLE_SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing element-specific Scythe Niagara system: {path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Failed to set enabled emitters to local space: {path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save Scythe element Niagara system: {path}")
    unreal.log(f"SCYTHE_ELEMENT_SLASH_OK system={path}")
