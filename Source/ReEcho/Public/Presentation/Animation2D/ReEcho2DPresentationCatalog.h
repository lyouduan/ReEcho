#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DPresentationCatalog.generated.h"

class UReEcho2DCharacterPresentationProfile;

/** Project-level AppearanceId registry. Hard references keep configured profiles cook-visible. */
UCLASS(BlueprintType)
class REECHO_API UReEcho2DPresentationCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UReEcho2DCharacterPresentationProfile>> CharacterProfiles;

	UReEcho2DCharacterPresentationProfile* ResolveProfile(FName AppearanceId) const;

	/** Strict registered-tag bridge for deterministic Editor asset authoring. */
	UFUNCTION(BlueprintPure, Category = "ReEcho|Animation2D")
	static FGameplayTag ResolveSemanticTag(FName TagName);
};
