import unreal


SYSTEM_PATHS = (
    "/Game/VFX/Element/Fire/Particle/NS_Element_Fire_Goat",
    "/Game/VFX/Element/Water/Particle/NS_Element_Water_Goat",
)


for system_path in SYSTEM_PATHS:
    system = unreal.EditorAssetLibrary.load_asset(system_path)
    if not isinstance(system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing Boss element reaction Niagara system: {system_path}")
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
        raise RuntimeError(f"Boss element reaction Niagara has no enabled emitter: {system_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save Boss element reaction Niagara system: {system_path}")
    unreal.log(f"BOSS_ELEMENT_REACTION_LOCAL_SPACE_OK system={system_path}")
