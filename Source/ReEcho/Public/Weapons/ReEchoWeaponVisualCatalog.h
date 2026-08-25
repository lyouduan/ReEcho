#pragma once

#include "CoreMinimal.h"

class UReEchoWeaponPresentationCatalog;
class UReEchoWeaponPresentationProfile;

/** Centralized profiles for the four production weapons plus non-roster presentation helpers. */
struct REECHO_API FReEchoWeaponVisualCatalog
{
	static UReEchoWeaponPresentationCatalog* ResolveCatalog();
	static UReEchoWeaponPresentationProfile* ResolveProfile(FName WeaponVisualKey);
	static FString ResolveHeldTexturePath(FName WeaponVisualKey);
	static FString ResolveAttackTexturePath(FName WeaponVisualKey);
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
};
