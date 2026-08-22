#include "Weapons/ReEchoWeaponVisualCatalog.h"

#include "Presentation/Weapon/ReEchoWeaponPresentationCatalog.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"

namespace
{
constexpr const TCHAR* CatalogPath =
    TEXT("/Game/ReEcho/DataAsset/Weapon/Catalogs/DA_WeaponPresentationCatalog.DA_WeaponPresentationCatalog");
}

UReEchoWeaponPresentationCatalog* FReEchoWeaponVisualCatalog::ResolveCatalog()
{
	return LoadObject<UReEchoWeaponPresentationCatalog>(nullptr, CatalogPath);
}

UReEchoWeaponPresentationProfile* FReEchoWeaponVisualCatalog::ResolveProfile(const FName WeaponVisualKey)
{
	const UReEchoWeaponPresentationCatalog* Catalog = ResolveCatalog();
	return Catalog ? Catalog->ResolveProfile(WeaponVisualKey) : nullptr;
}

FString FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(const FName WeaponVisualKey)
{
	const UReEchoWeaponPresentationProfile* Profile = ResolveProfile(WeaponVisualKey);
	return Profile ? Profile->HeldTexture.ToSoftObjectPath().ToString() : FString();
}

FString FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(const FName WeaponVisualKey)
{
	const UReEchoWeaponPresentationProfile* Profile = ResolveProfile(WeaponVisualKey);
	return Profile ? Profile->LegacyAttackTexture.ToSoftObjectPath().ToString() : FString();
}

void FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	OutPaths.Add(CatalogPath);
	if (const UReEchoWeaponPresentationCatalog* Catalog = ResolveCatalog())
	{
		Catalog->GatherPreloadAssetPaths(OutPaths);
	}
}
