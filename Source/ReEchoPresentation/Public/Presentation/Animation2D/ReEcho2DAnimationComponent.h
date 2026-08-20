#pragma once

// Public API of the ReEchoPresentation runtime module.

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationClip.h"
#include "Presentation/Animation2D/ReEcho2DAnimationProfile.h"
#include "ReEcho2DAnimationComponent.generated.h"

class UPaperSprite;

/** Reusable visual-only Paper2D player. It never owns gameplay state or Actor transforms. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOPRESENTATION_API UReEcho2DAnimationComponent : public UPaperFlipbookComponent
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
	static FVector CalculateFootAlignmentOffset(const FBoxSphereBounds& FlipbookBounds,
	                                            const FTransform& RendererToFlipbookRoot,
	                                            const FTransform& FlipbookRootToMotionRoot,
	                                            const FVector& AuthoredMotionLocation,
	                                            const FVector& FootpointOffset = FVector::ZeroVector);
	static float CalculateFlipbookPresentationWidth(const FBoxSphereBounds& FlipbookBounds,
	                                                const FTransform& RendererToFlipbookRoot,
	                                                const FTransform& FlipbookRootToTarget);

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
