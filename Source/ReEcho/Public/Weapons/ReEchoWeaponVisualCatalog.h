#pragma once

#include "CoreMinimal.h"

enum class EReEchoWeaponMotionMode : uint8
{
	None,
	FullSpin
};

/** One-to-one presentation profile. It never contains character animation assets or gameplay rules. */
struct REECHO_API FReEchoWeaponPresentationProfile
{
	FName VisualKey = NAME_None;
	const TCHAR* HeldTexturePath = TEXT("");
	const TCHAR* LegacyAttackTexturePath = TEXT("");
	EReEchoWeaponMotionMode MotionMode = EReEchoWeaponMotionMode::None;
	bool bUsesDedicatedAttackVfx = false;
};

/** Centralized profiles for the six data-authored weapon presentation families. */
struct REECHO_API FReEchoWeaponVisualCatalog
{
	static const FReEchoWeaponPresentationProfile* ResolveProfile(FName WeaponVisualKey);
	static const TCHAR* ResolveHeldTexturePath(FName WeaponVisualKey);
	static const TCHAR* ResolveAttackTexturePath(FName WeaponVisualKey);
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
};
