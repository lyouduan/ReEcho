#pragma once

#include "Engine/DataAsset.h"
#include "ReEchoWeaponPresentationCatalog.generated.h"

class UReEchoWeaponPresentationProfile;

UCLASS(BlueprintType)

class REECHO_API UReEchoWeaponPresentationCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Shared right-facing hand anchor for every character and weapon, normalized to character WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Layout")
	FVector RightHandAnchorRatio = FVector(-0.16f, 0.30f, 0.06f);

	/** Shared left-facing hand anchor for every character and weapon, normalized to character WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held Layout")
	FVector LeftHandAnchorRatio = FVector(-0.16f, -0.30f, 0.06f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TMap<FName, TSoftObjectPtr<UReEchoWeaponPresentationProfile>> Profiles;
	UFUNCTION(BlueprintPure, Category = "Weapons")
	UReEchoWeaponPresentationProfile* ResolveProfile(FName WeaponVisualKey) const;
	void GatherPreloadAssetPaths(TArray<FString>& OutPaths) const;
};
