#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Presentation/Animation2D/ReEcho2DAnimationClip.h"
#include "ReEcho2DCharacterPresentationProfile.generated.h"

class UTexture2D;

/** Animation clips selected by the equipped weapon's stable VisualKey. */
USTRUCT(BlueprintType)
struct REECHO_API FReEcho2DCompositeAnimationSet
{
	GENERATED_BODY()

	/** NAME_None is the character-level fallback set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaponVisualSetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, FReEcho2DAnimationClip> Clips;

	const FReEcho2DAnimationClip* FindClip(FGameplayTag SemanticKey) const;
};

/** AppearanceId-owned presentation policy; gameplay IDs never select assets directly. */
UCLASS(BlueprintType)
class REECHO_API UReEcho2DCharacterPresentationProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName AppearanceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fallback")
	TObjectPtr<UTexture2D> StaticFallback;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TArray<FReEcho2DCompositeAnimationSet> AnimationSets;

	const FReEcho2DCompositeAnimationSet* FindAnimationSet(FName WeaponVisualSetId) const;
	const FReEcho2DAnimationClip* ResolveClip(FName WeaponVisualSetId, FGameplayTag SemanticKey) const;
};
