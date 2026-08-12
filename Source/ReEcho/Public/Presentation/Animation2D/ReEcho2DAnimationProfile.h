#pragma once

#include "CoreMinimal.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTypes.h"
#include "ReEcho2DAnimationProfile.generated.h"

class UPaperFlipbook;

/** Presentation-only Flipbook mapping. Unassigned states resolve to DefaultFlipbook. */
USTRUCT(BlueprintType)

struct REECHO_API FReEcho2DAnimationProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPaperFlipbook> DefaultFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<EReEcho2DAnimationState, TObjectPtr<UPaperFlipbook>> StateFlipbooks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WorldHeight = 224.0f;

	/** Preserve the Flipbook asset's authored world scale instead of normalizing it to WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseNativeScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TranslucentSortPriority = 10;

	UPaperFlipbook* Resolve(EReEcho2DAnimationState State) const;
};
