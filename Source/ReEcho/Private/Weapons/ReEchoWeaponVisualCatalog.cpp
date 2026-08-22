#include "Weapons/ReEchoWeaponVisualCatalog.h"

namespace
{
const FReEchoWeaponPresentationProfile Profiles[] = {
    {TEXT("CrescentBlade"),
     TEXT("/Game/ReEcho/Textures/Effects/CrescentWeapon.CrescentWeapon"),
     TEXT(""),
     EReEchoWeaponMotionMode::FullSpin,
     true},
    {TEXT("Scythe"),
     TEXT("/Game/ReEcho/Textures/Effects/Scythe.Scythe"),
     TEXT(""),
     EReEchoWeaponMotionMode::None,
     true},
    {TEXT("Whip"),
     TEXT("/Game/ReEcho/Textures/Effects/Whip.Whip"),
     TEXT("/Game/ReEcho/Textures/Effects/WhipLash.WhipLash"),
     EReEchoWeaponMotionMode::None,
     false},
    {TEXT("Bow"), TEXT("/Game/ReEcho/Textures/Effects/Bow.Bow"), TEXT(""), EReEchoWeaponMotionMode::None, true},
    {TEXT("Gun"), TEXT("/Game/ReEcho/Textures/Effects/Gun.Gun"), TEXT(""), EReEchoWeaponMotionMode::None, true},
    {TEXT("Staff"),
     TEXT("/Game/ReEcho/Textures/Effects/MoonStaff.MoonStaff"),
     TEXT("/Game/ReEcho/Textures/Effects/StaffLightWave.StaffLightWave"),
     EReEchoWeaponMotionMode::None,
     false}};
}

const FReEchoWeaponPresentationProfile* FReEchoWeaponVisualCatalog::ResolveProfile(const FName WeaponVisualKey)
{
	const FName CanonicalKey = WeaponVisualKey == TEXT("MoonStaff") ? FName(TEXT("Staff")) : WeaponVisualKey;
	for (const FReEchoWeaponPresentationProfile& Profile : Profiles)
	{
		if (Profile.VisualKey == CanonicalKey)
		{
			return &Profile;
		}
	}
	return nullptr;
}

const TCHAR* FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(const FName WeaponVisualKey)
{
	const FReEchoWeaponPresentationProfile* Profile = ResolveProfile(WeaponVisualKey);
	return Profile ? Profile->HeldTexturePath : TEXT("");
}

const TCHAR* FReEchoWeaponVisualCatalog::ResolveAttackTexturePath(const FName WeaponVisualKey)
{
	const FReEchoWeaponPresentationProfile* Profile = ResolveProfile(WeaponVisualKey);
	return Profile ? Profile->LegacyAttackTexturePath : TEXT("");
}

void FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	for (const FReEchoWeaponPresentationProfile& Profile : Profiles)
	{
		OutPaths.Add(Profile.HeldTexturePath);
		OutPaths.Add(Profile.LegacyAttackTexturePath);
	}
}
