"""Load the authored Goat Boss Niagara roots and print their recursive package dependencies."""

import unreal


ROOTS = [
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Charging",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_BeAttacked",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Charging",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_BeAttacked",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Charging",
    "/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting",
]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
pending = list(ROOTS)
visited = set()
while pending:
    package_name = pending.pop(0)
    if package_name in visited:
        continue
    visited.add(package_name)
    asset = unreal.EditorAssetLibrary.load_asset(package_name)
    if package_name in ROOTS:
        if not isinstance(asset, unreal.NiagaraSystem):
            raise RuntimeError(f"Goat Boss Niagara root failed to load: {package_name}")
        unreal.log(f"GOAT_ROOT_OK package={package_name}")
    for dependency in registry.get_dependencies(package_name):
        dependency_name = str(dependency)
        if dependency_name.startswith("/Game/") and dependency_name not in visited:
            pending.append(dependency_name)

for package_name in sorted(visited):
    if package_name not in ROOTS:
        unreal.log(f"GOAT_DEP package={package_name}")
unreal.log(f"GOAT_AUDIT_OK roots={len(ROOTS)} packages={len(visited)}")
