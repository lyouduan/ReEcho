"""Make every production scythe slash renderer follow one ground-plane basis."""

import unreal


PROFILE_PATH = "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Scythe"
ELEMENT_SYSTEM_PATHS = (
    "/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Fire",
    "/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Thunder",
    "/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Grass",
    "/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Water",
)

profile = unreal.EditorAssetLibrary.load_asset(PROFILE_PATH)
if not isinstance(profile, unreal.ReEchoWeaponPresentationProfile):
    raise RuntimeError(f"Missing scythe weapon presentation profile: {PROFILE_PATH}")

slot = profile.get_editor_property("attack_committed")
system_reference = slot.get_editor_property("system")
system = (
    system_reference
    if isinstance(system_reference, unreal.NiagaraSystem)
    else system_reference.load_synchronous()
)
if not isinstance(system, unreal.NiagaraSystem):
    raise RuntimeError("Scythe AttackCommitted Niagara is not configured")

default_system_path = system.get_path_name().split(".", 1)[0]
referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(default_system_path, False)
shared_weapon_profiles = []
for asset_path in unreal.EditorAssetLibrary.list_assets(
    "/Game/ReEcho/DataAsset/Weapon/Profiles", recursive=False, include_folder=False
):
    candidate = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(candidate, unreal.ReEchoWeaponPresentationProfile):
        continue
    candidate_system = candidate.get_editor_property("attack_committed").get_editor_property(
        "system"
    )
    if candidate_system == system and candidate.get_path_name().split(".", 1)[0] != PROFILE_PATH:
        shared_weapon_profiles.append(candidate.get_path_name())
if shared_weapon_profiles:
    raise RuntimeError(
        f"Scythe Niagara is shared by other weapon profiles; refusing mutation: {shared_weapon_profiles}"
    )

systems = [(default_system_path, system)]
for asset_path in ELEMENT_SYSTEM_PATHS:
    element_system = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(element_system, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing element scythe Niagara: {asset_path}")
    systems.append((asset_path, element_system))

for system_path, scythe_system in systems:
    if not unreal.ReEchoCombatVfxComponent.configure_melee_niagara_component_facing(
        scythe_system
    ):
        raise RuntimeError(
            f"Scythe Niagara requires enabled emitters and a mesh renderer: {system_path}"
        )
    if not unreal.ReEchoCombatVfxComponent.set_niagara_system_sprite_facing_owner_up(
        scythe_system
    ):
        raise RuntimeError(f"Scythe Niagara has no configurable sprite renderer: {system_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        scythe_system, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save scythe Niagara: {system_path}")
    unreal.log(
        "PLAN148_SCYTHE_GROUND_FACING "
        f"system={system_path} local_space=true mesh_facing=Default "
        "sprite_facing=CustomFacingVector normal=User.GroundNormal "
        f"tangent=User.GroundTangent referencers={referencers if system_path == default_system_path else []}"
    )
