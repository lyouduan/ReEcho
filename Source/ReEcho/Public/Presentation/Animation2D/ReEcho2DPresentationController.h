#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DPresentationController.generated.h"

class UBillboardComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DFrameCollisionDriver;

/**
 * Presentation-only semantic state controller. Gameplay reports intent; this component resolves
 * profile clips, one-shot completion and the exclusive static/Flipbook renderer.
 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHO_API UReEcho2DPresentationController : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEcho2DPresentationController();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(UBillboardComponent* InStaticRenderer,
	               UReEcho2DAnimationComponent* InAnimationRenderer,
	               UReEcho2DCharacterPresentationProfile* InProfile,
	               FName InWeaponVisualSetId = NAME_None);
	void ClearProfile();
	void SetWeaponVisualSetId(FName InWeaponVisualSetId);
	void SetMoving(bool bInMoving);
	bool PlayAction(FGameplayTag SemanticKey, bool bRestart = true, int64 AttackInstanceId = INDEX_NONE);
	void SetFacingSign(float FacingSign);
	void BindCollisionDriver(UReEcho2DFrameCollisionDriver* InCollisionDriver);
	/** Poll one-shot completion without owning gameplay time; also used by deterministic tests. */
	void UpdatePlaybackCompletion();

	FGameplayTag GetActiveSemanticKey() const { return ActiveSemanticKey; }
	const UReEcho2DCharacterPresentationProfile* GetProfile() const { return Profile; }

private:
	void ApplyBaseState(bool bRestart = false);
	bool ApplySemantic(FGameplayTag SemanticKey, bool bRestart);
	void ShowStaticFallback();

	UPROPERTY()
	TObjectPtr<UBillboardComponent> StaticRenderer;
	UPROPERTY()
	TObjectPtr<UReEcho2DAnimationComponent> AnimationRenderer;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> Profile;
	UPROPERTY()
	TObjectPtr<UReEcho2DFrameCollisionDriver> CollisionDriver;

	FName WeaponVisualSetId;
	FGameplayTag ActiveSemanticKey;
	bool bMoving = false;
	bool bWaitingForOneShot = false;
	bool bDeathLocked = false;
	int64 ActiveAttackInstanceId = INDEX_NONE;
};
