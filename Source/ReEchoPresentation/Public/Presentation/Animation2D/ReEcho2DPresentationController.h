#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DPresentationController.generated.h"

class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DFrameCollisionDriver;
class UBillboardComponent;
struct FReEcho2DAnimationStateDefinition;

/**
 * Presentation-only semantic state controller. Gameplay reports intent; this component resolves
 * profile clips, one-shot completion and the exclusive static/Flipbook renderer.
 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOPRESENTATION_API UReEcho2DPresentationController : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEcho2DPresentationController();
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(UReEcho2DAnimationComponent* InAnimationRenderer,
	               UReEcho2DCharacterPresentationProfile* InProfile,
	               FName InWeaponVisualSetId = NAME_None);
	/** Compatibility adapter for the EnemyPresentation host contract on the runtime mainline. */
	void Configure(UBillboardComponent* InStaticRenderer,
	               UReEcho2DAnimationComponent* InAnimationRenderer,
	               UReEcho2DCharacterPresentationProfile* InProfile);
	void ClearProfile();
	void SetWeaponVisualSetId(FName InWeaponVisualSetId);
	/** Select a target animation set without flashing its base state, then play its transition clip. */
	bool BeginAnimationSetTransition(FName InAnimationSetId, FGameplayTag TransitionSemanticKey);
	/** End a transition deterministically and enter the target set's Move base loop when authored. */
	void CompleteAnimationSetTransition(FName InAnimationSetId);
	void SetMoving(bool bInMoving);
	bool PlayAction(FGameplayTag SemanticKey, bool bRestart = true, int64 AttackInstanceId = INDEX_NONE);
	/** Replace every other presentation with one non-looping terminal Death. Completion never returns to Move. */
	bool BeginTerminalDeath(FSimpleDelegate OnCompleted, float& OutExpectedDurationSeconds);
	/** End an active attack presentation without affecting Hit, Transform or Death. */
	bool CancelAttackAction();
	void SetFacingSign(float FacingSign);
	void BindCollisionDriver(UReEcho2DFrameCollisionDriver* InCollisionDriver);
	/** Poll one-shot completion without owning gameplay time; also used by deterministic tests. */
	void UpdatePlaybackCompletion();

	FGameplayTag GetActiveSemanticKey() const
	{
		return ActiveSemanticKey;
	}

	FGameplayTag GetActiveStateTag() const
	{
		return ActiveStateTag;
	}

	const UReEcho2DCharacterPresentationProfile* GetProfile() const
	{
		return Profile;
	}

private:
	void ApplyBaseState(bool bRestart = false);
	bool ApplySemantic(FGameplayTag SemanticKey, bool bRestart);
	bool CanEnterState(const FReEcho2DAnimationStateDefinition* DesiredState) const;
	const FReEcho2DAnimationStateDefinition* ResolveState(FGameplayTag SemanticKey) const;
	void DeactivatePresentation();

	UPROPERTY()
	TObjectPtr<UReEcho2DAnimationComponent> AnimationRenderer;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> Profile;
	UPROPERTY()
	TObjectPtr<UReEcho2DFrameCollisionDriver> CollisionDriver;

	FName WeaponVisualSetId;
	FName PendingAnimationSetId;
	FGameplayTag ActiveSemanticKey;
	FGameplayTag ActiveStateTag;
	bool bMoving = false;
	bool bActionActive = false;
	bool bWaitingForOneShot = false;
	bool bTerminalDeathActive = false;
	FSimpleDelegate TerminalDeathCompleted;
	int64 ActiveAttackInstanceId = INDEX_NONE;
};
