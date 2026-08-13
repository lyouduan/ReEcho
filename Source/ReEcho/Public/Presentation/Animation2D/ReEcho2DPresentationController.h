#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ReEcho2DPresentationController.generated.h"

class UBillboardComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;

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
	bool PlayAction(FGameplayTag SemanticKey, bool bRestart = true);
	void SetFacingSign(float FacingSign);

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

	FName WeaponVisualSetId;
	FGameplayTag ActiveSemanticKey;
	bool bMoving = false;
	bool bWaitingForOneShot = false;
	bool bDeathLocked = false;
};
