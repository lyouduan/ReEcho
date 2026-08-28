"""Read-only audit for the delivered Echo connection Niagara dependency closure."""

import unreal


ASSETS = (
    "/Game/VFX/Echo/Particle/NS_Echo_Chain",
    "/Game/VFX/Echo/MI/BaseVFX003_Inst25",
    "/Game/01_Textures/02_Turbulence/Tur_C080",
    "/Game/01_Textures/02_Turbulence/Tur_C090",
)

registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True,
    include_hard_package_references=True,
    include_searchable_names=True,
    include_soft_management_references=True,
    include_hard_management_references=True,
)

for asset_path in ASSETS:
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Plan142 missing asset: {asset_path}")
    unreal.log(f"PLAN142_AUDIT loaded={asset.get_path_name()} class={asset.get_class().get_name()}")
    for dependency in registry.get_dependencies(asset_path, options):
        unreal.log(f"PLAN142_AUDIT dependency owner={asset_path} path={dependency}")

unreal.log("PLAN142_AUDIT_COMPLETE")
