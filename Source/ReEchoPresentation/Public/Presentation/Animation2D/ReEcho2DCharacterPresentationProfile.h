#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Presentation/Animation2D/ReEcho2DAnimationClip.h"
#include "ReEcho2DCharacterPresentationProfile.generated.h"

class UReEcho2DAnimationStateMachineAsset;

/** Animation clips selected by the equipped weapon's stable VisualKey. */
USTRUCT(BlueprintType)

struct REECHOPRESENTATION_API FReEcho2DCompositeAnimationSet
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

class REECHOPRESENTATION_API UReEcho2DCharacterPresentationProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName AppearanceId;

	/** 同一外观全部序列统一归一化到该世界高度，避免源图画布尺寸影响比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scale", meta = (ClampMin = "1.0"))
	float WorldHeight = 100.0f;

	/** Automatically place the complete Flipbook render bounds so its bottom center meets the character footpoint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footpoint")
	bool bAutoAlignFootpoint = true;

	/** Art-authored correction in FootRoot space, applied after automatic bounds alignment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footpoint")
	FVector FootpointOffset = FVector::ZeroVector;

	/** Stable held-weapon anchor in actor space, normalized to WorldHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FVector WeaponAnchorRatio = FVector(-0.16f, 0.30f, 0.06f);

	/** 表现状态、优先级和中断规则；Flipbook仍由下方外观/武器动画集提供。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UReEcho2DAnimationStateMachineAsset> StateMachine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TArray<FReEcho2DCompositeAnimationSet> AnimationSets;

	const FReEcho2DCompositeAnimationSet* FindAnimationSet(FName WeaponVisualSetId) const;
	const FReEcho2DAnimationClip* ResolveClip(FName WeaponVisualSetId, FGameplayTag SemanticKey) const;
};
