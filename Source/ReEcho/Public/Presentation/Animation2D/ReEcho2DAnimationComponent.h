#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationClip.h"
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
	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	static FRotator CalculateCameraFacingRotation(const FRotator& CameraRotation);

	/** Faithfully plays one data-owned clip without resolving gameplay or character state. */
	bool PlayClip(const FReEcho2DAnimationClip& Clip, bool bRestart = false);
	EReEcho2DAnimationActivationResult ActivateProfile(const FReEcho2DAnimationProfile& InProfile);
	bool SetAnimationState(EReEcho2DAnimationState NewState, bool bShouldLoop = true, bool bRestart = false);
	void DeactivateAnimation();
	void SetFacingSign(float InFacingSign);
	bool IsAnimationActive();

	const FReEcho2DAnimationClip& GetActiveClip() const
	{
		return ActiveClip;
	}

	int32 GetCurrentKeyFrameIndex();

	float GetFacingSign() const
	{
		return FacingSign;
	}

	bool IsUsingEachFrameCollision() const;

	/** Flipbook source art faces right when unmirrored. Clear this for assets authored facing left. */
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Character Scene|Flipbook",
	          meta = (DisplayName = "Source Faces Right (资源默认朝右)"))
	bool bSourceFacesRight = true;

	/** Scene grading for 2D characters. Tune per Gameplay Blueprint to match the map and plant cards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Scene|Flipbook")
	FLinearColor CharacterTint = FLinearColor(0.82f, 0.84f, 0.78f, 1.0f);

	/** Editor asset-repair seam used by the deterministic import script; it has no runtime gameplay effect. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|Animation2D", meta = (DevelopmentOnly))
	static bool RebuildSpriteAsset(UPaperSprite* Sprite);

private:
	void ApplyDisplayScale();
	void ApplyCharacterTint();
	void ApplyCollisionPolicy();
	void DrawCurrentFrameCollisionDebug();

	UPROPERTY()
	FReEcho2DAnimationProfile ActiveProfile;
	UPROPERTY()
	FReEcho2DAnimationClip ActiveClip;
	EReEcho2DAnimationState ActiveState = EReEcho2DAnimationState::Default;
	float FacingSign = 1.0f;
	bool bAnimationActive = false;
};
