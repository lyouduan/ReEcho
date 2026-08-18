#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DAnimationStateMachineAsset.generated.h"

USTRUCT(BlueprintType)
struct REECHOPRESENTATION_API FReEcho2DAnimationStateDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag StateTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag SemanticKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag CompletionStateTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 InterruptPriority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLockUntilPlaybackComplete = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bTerminal = false;
};

/** Editor-authored presentation FSM. States resolve their Flipbook through the owning appearance profile. */
UCLASS(BlueprintType)
class REECHOPRESENTATION_API UReEcho2DAnimationStateMachineAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FReEcho2DAnimationStateDefinition* FindState(FGameplayTag StateTag) const;
	const FReEcho2DAnimationStateDefinition* FindStateBySemantic(FGameplayTag SemanticKey) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine")
	FGameplayTag InitialStateTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine")
	TArray<FReEcho2DAnimationStateDefinition> States;
};
