"""Read-only audit of material sharing for the scythe slash Niagara."""

import unreal


MATERIAL_PATHS = (
    "/Game/VFX/People/Sword/MI/BaseVFX003_Inst1",
    "/Game/VFX/People/Sword/MI/BaseVFX003_Inst4",
    "/Game/VFX/People/Sword/MI/BaseVFX003_Inst5",
)
registry = unreal.AssetRegistryHelpers.get_asset_registry()

for path in MATERIAL_PATHS:
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(material, unreal.MaterialInterface):
        raise RuntimeError(f"Missing material interface: {path}")
    parent = material.get_editor_property("parent") if isinstance(material, unreal.MaterialInstance) else None
    referencers = sorted(
        unreal.EditorAssetLibrary.find_package_referencers_for_asset(path, load_assets_to_confirm=True)
    )
    unreal.log(
        "PLAN148_SCYTHE_MATERIAL_AUDIT "
        f"material={path} class={material.get_class().get_name()} "
        f"parent={parent.get_path_name() if parent else 'None'} referencers={referencers}"
    )
