#pragma once

#include "Engine/DataAsset.h"
#include "ReEchoWeaponPresentationCatalog.generated.h"

class UReEchoWeaponPresentationProfile;

UCLASS(BlueprintType)

class REECHO_API UReEchoWeaponPresentationCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TMap<FName, TSoftObjectPtr<UReEchoWeaponPresentationProfile>> Profiles;
	UFUNCTION(BlueprintPure, Category = "Weapons")
	UReEchoWeaponPresentationProfile* ResolveProfile(FName WeaponVisualKey) const;
	void GatherPreloadAssetPaths(TArray<FString>& OutPaths) const;
};
