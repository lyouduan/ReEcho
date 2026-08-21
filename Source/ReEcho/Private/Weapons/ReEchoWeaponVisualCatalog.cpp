#include "Weapons/ReEchoWeaponVisualCatalog.h"

const TCHAR* FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(const FName WeaponVisualKey)
{
	if (WeaponVisualKey == TEXT("CrescentBlade"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon");
	}
	if (WeaponVisualKey == TEXT("Scythe"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Scythe.Scythe");
	}
	if (WeaponVisualKey == TEXT("Whip"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Whip.Whip");
	}
	if (WeaponVisualKey == TEXT("Bow"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Bow.Bow");
	}
	if (WeaponVisualKey == TEXT("Gun"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/Gun.Gun");
	}
	if (WeaponVisualKey == TEXT("Staff") || WeaponVisualKey == TEXT("MoonStaff"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff");
	}
	return TEXT("");
}

const TCHAR* FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(const FName WeaponVisualKey)
{
	if (WeaponVisualKey == TEXT("Scythe"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/ScytheSweep.ScytheSweep");
	}
	if (WeaponVisualKey == TEXT("Whip"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/WhipLash.WhipLash");
	}
	if (WeaponVisualKey == TEXT("Bow"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/BowProjectile.BowProjectile");
	}
	if (WeaponVisualKey == TEXT("Gun"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/GunProjectile.GunProjectile");
	}
	if (WeaponVisualKey == TEXT("Staff") || WeaponVisualKey == TEXT("MoonStaff"))
	{
		return TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave");
	}
	return TEXT("/Game/ReEcho/Textures/Effects/SlashCrescent.SlashCrescent");
}

void FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	static const FName VisualKeys[] = {
	    TEXT("CrescentBlade"), TEXT("Scythe"), TEXT("Whip"), TEXT("Bow"), TEXT("Gun"), TEXT("Staff")};
	for (const FName VisualKey : VisualKeys)
	{
		OutPaths.Add(ResolveHeldTexturePath(VisualKey));
		OutPaths.Add(ResolveAttackTexturePath(VisualKey));
	}
}
