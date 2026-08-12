#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationProfile.h"
#include "ReEcho2DAnimationComponent.generated.h"

class UPaperSprite;

/** Reusable visual-only Paper2D player. It never owns gameplay state or Actor transforms. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEcho2DAnimationComponent : public UPaperFlipbookComponent
{
	GENERATED_BODY()

public:
	UReEcho2DAnimationComponent();

	EReEcho2DAnimationActivationResult ActivateProfile(const FReEcho2DAnimationProfile& InProfile);
	bool SetAnimationState(EReEcho2DAnimationState NewState);
	void DeactivateAnimation();
	void SetFacingSign(float InFacingSign);
	bool IsAnimationActive();

	/** Editor asset-repair seam used by the deterministic import script; it has no runtime gameplay effect. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|Animation2D", meta = (DevelopmentOnly))
	static bool RebuildSpriteAsset(UPaperSprite* Sprite);

private:
	void ApplyFlipbook(UPaperFlipbook* NewFlipbook);
	void ApplyDisplayScale();

	UPROPERTY()
	FReEcho2DAnimationProfile ActiveProfile;
	EReEcho2DAnimationState ActiveState = EReEcho2DAnimationState::Default;
	float FacingSign = 1.0f;
	bool bAnimationActive = false;
};
