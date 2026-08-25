import unreal


PROFILE_PATHS = (
    "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_CrescentBlade."
    "DA_WeaponPresentation_CrescentBlade",
)

RETAINED_PROFILE_KEYS = ("CrescentBlade", "Scythe", "Bow", "Gun")
VFX_SLOT_NAMES = ("attack_committed", "travel", "damage_applied")

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

configured_systems = set()
for key in RETAINED_PROFILE_KEYS:
    profile_path = (
        f"/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_{key}."
        f"DA_WeaponPresentation_{key}"
    )
    profile = unreal.load_object(None, profile_path)
    if not profile:
        raise RuntimeError(f"Missing retained weapon profile: {profile_path}")
    for slot_name in VFX_SLOT_NAMES:
        system = profile.get_editor_property(slot_name).get_editor_property("system")
        if not system or system.get_path_name() in configured_systems:
            continue
        if not unreal.ReEchoCombatVfxComponent.set_niagara_system_emitters_local_space(system):
            raise RuntimeError(f"Could not configure local-space emitters for {system.get_path_name()}")
        if not unreal.EditorAssetLibrary.save_loaded_asset(system, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save {system.get_path_name()}")
        configured_systems.add(system.get_path_name())
        unreal.log(f"PLAN100_WEAPON_DIRECTION system={system.get_path_name()} local_space=true")
