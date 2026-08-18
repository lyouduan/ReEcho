import unreal


LEGACY_ASSETS = (
    "/Game/ReEcho/Animation2D/DA_Character_J_CAT",
    "/Game/ReEcho/Animation2D/Generated/Players/J_CAT/IdleSprite",
    "/Game/ReEcho/Animation2D/Generated/Players/J_CAT/Idle",
    "/Game/ReEcho/Textures/Characters/NewCast/Player_Cat",
    "/Game/ReEcho/Textures/Characters/NewCast/Echo_Cat",
    "/Game/ReEcho/Textures/Characters/Boss2D",
)


def main() -> None:
    failures = []
    for asset_path in LEGACY_ASSETS:
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            unreal.log_warning(f"PLAN50_AUDIT missing target: {asset_path}")
            continue
        referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
            asset_path, load_assets_to_confirm=False
        )
        unreal.log(f"PLAN50_AUDIT {asset_path} referencers={referencers}")
        if referencers:
            failures.append((asset_path, referencers))
    if failures:
        raise RuntimeError(f"Plan50 legacy assets still have referencers: {failures}")
    unreal.log("PLAN50_AUDIT all legacy assets are unreferenced")


main()
