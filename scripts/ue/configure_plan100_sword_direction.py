import unreal


PROFILE_PATHS = (
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade."
    "DA_WeaponPresentation_CrescentBlade",
)

for profile_path in PROFILE_PATHS:
    profile = unreal.load_object(None, profile_path)
    if not profile:
        raise RuntimeError(f"Missing melee weapon profile: {profile_path}")
    slot = profile.get_editor_property("attack_committed")
    system = slot.get_editor_property("system")
    if not system:
        raise RuntimeError(f"AttackCommitted Niagara is not configured: {profile_path}")
    if not unreal.ReEchoCombatVfxComponent.configure_melee_niagara_component_facing(system):
        raise RuntimeError(f"Could not configure component-facing mesh renderers for {system.get_path_name()}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {system.get_path_name()}")
    unreal.log(
        f"PLAN100_MELEE_DIRECTION system={system.get_path_name()} "
        "local_space=true mesh_facing=default"
    )
