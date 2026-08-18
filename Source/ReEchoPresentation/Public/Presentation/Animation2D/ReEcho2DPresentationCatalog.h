#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DPresentationCatalog.generated.h"

class UReEcho2DCharacterPresentationProfile;

/** Cook-visible presentation binding selected only through a stable visual id. */
USTRUCT(BlueprintType)

struct REECHOPRESENTATION_API FReEcho2DPresentationCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PresentationId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DCharacterPresentationProfile> Profile;

};

/** Project-level AppearanceId registry. Hard references keep configured profiles cook-visible. */
UCLASS(BlueprintType)

class REECHOPRESENTATION_API UReEcho2DPresentationCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FReEcho2DPresentationCatalogEntry> PresentationEntries;

	/** Legacy player-only list retained while existing assets migrate to PresentationEntries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UReEcho2DCharacterPresentationProfile>> CharacterProfiles;

	UReEcho2DCharacterPresentationProfile* ResolveProfile(FName PresentationId) const;
	/** Strict registered-tag bridge for deterministic Editor asset authoring. */
	UFUNCTION(BlueprintPure, Category = "ReEcho|Animation2D")
	static FGameplayTag ResolveSemanticTag(FName TagName);
};
