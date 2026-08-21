#pragma once

#include "CoreMinimal.h"

/** Centralized paths for the six data-authored weapon presentation families. */
struct REECHO_API FReEchoWeaponVisualCatalog
{
	static const TCHAR* ResolveHeldTexturePath(FName WeaponVisualKey);
	static const TCHAR* ResolveAttackTexturePath(FName WeaponVisualKey);
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
};
