#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"

#include "PaperFlipbook.h"
#include "PaperSprite.h"

EReEcho2DAnimationState ReEchoResolve2DAnimationState(const bool bMoving, const bool bAttacking)
{
	if (bAttacking)
	{
		return EReEcho2DAnimationState::Attack;
	}
	return bMoving ? EReEcho2DAnimationState::Walk : EReEcho2DAnimationState::Idle;
}

UPaperFlipbook* FReEcho2DAnimationProfile::Resolve(const EReEcho2DAnimationState State) const
{
	if (const TObjectPtr<UPaperFlipbook>* Match = StateFlipbooks.Find(State); Match && Match->Get())
	{
		return Match->Get();
	}
	return DefaultFlipbook;
}

UReEcho2DAnimationComponent::UReEcho2DAnimationComponent()
{
	// UPaperFlipbookComponent advances playback from its component tick.
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetLooping(true);
	SetVisibility(false);
	SetHiddenInGame(true);
	SetAbsolute(false, true, false);
	SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
}

EReEcho2DAnimationActivationResult
UReEcho2DAnimationComponent::ActivateProfile(const FReEcho2DAnimationProfile& InProfile)
{
	DeactivateAnimation();
	if (!InProfile.bUseNativeScale && InProfile.WorldHeight <= 0.0f)
	{
		return EReEcho2DAnimationActivationResult::MissingProfile;
	}
	if (!InProfile.DefaultFlipbook)
	{
		return EReEcho2DAnimationActivationResult::MissingFlipbook;
	}

	ActiveProfile = InProfile;
	ActiveState = EReEcho2DAnimationState::Default;
	bAnimationActive = true;
	SetLooping(true);
	SetRelativeLocation(ActiveProfile.LocalOffset);
	SetTranslucentSortPriority(ActiveProfile.TranslucentSortPriority);
	ApplyFlipbook(ActiveProfile.DefaultFlipbook);
	SetHiddenInGame(false);
	SetVisibility(true);
	SetComponentTickEnabled(true);
	PlayFromStart();
	return EReEcho2DAnimationActivationResult::Activated;
}

bool UReEcho2DAnimationComponent::SetAnimationState(const EReEcho2DAnimationState NewState,
                                                    const bool bShouldLoop,
                                                    const bool bRestart)
{
	if (!bAnimationActive)
	{
		return false;
	}
	UPaperFlipbook* ResolvedFlipbook = ActiveProfile.Resolve(NewState);
	if (!bRestart && ActiveState == NewState && GetFlipbook() == ResolvedFlipbook && IsLooping() == bShouldLoop &&
	    IsPlaying())
	{
		return true;
	}
	ActiveState = NewState;
	SetLooping(bShouldLoop);
	ApplyFlipbook(ResolvedFlipbook);
	SetLooping(bShouldLoop);
	return GetFlipbook() != nullptr;
}

void UReEcho2DAnimationComponent::DeactivateAnimation()
{
	Stop();
	SetFlipbook(nullptr);
	SetVisibility(false);
	SetHiddenInGame(true);
	SetComponentTickEnabled(false);
	bAnimationActive = false;
	ActiveState = EReEcho2DAnimationState::Default;
}

void UReEcho2DAnimationComponent::SetFacingSign(const float InFacingSign)
{
	FacingSign = InFacingSign < 0.0f ? -1.0f : 1.0f;
	ApplyDisplayScale();
}

bool UReEcho2DAnimationComponent::IsAnimationActive()
{
	return bAnimationActive && GetFlipbook() != nullptr;
}

bool UReEcho2DAnimationComponent::RebuildSpriteAsset(UPaperSprite* Sprite)
{
#if WITH_EDITOR
	if (!Sprite)
	{
		return false;
	}
	Sprite->Modify();
	Sprite->RebuildData();
	Sprite->MarkPackageDirty();
	return Sprite->GetRenderBounds().BoxExtent.Z > 0.0f;
#else
	return false;
#endif
}

void UReEcho2DAnimationComponent::ApplyFlipbook(UPaperFlipbook* NewFlipbook)
{
	if (!NewFlipbook)
	{
		NewFlipbook = ActiveProfile.DefaultFlipbook;
	}
	if (GetFlipbook() != NewFlipbook)
	{
		SetFlipbook(NewFlipbook);
	}
	SetLooping(true);
	SetComponentTickEnabled(true);
	PlayFromStart();
	ApplyDisplayScale();
}

void UReEcho2DAnimationComponent::ApplyDisplayScale()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	const float NativeWorldHeight = Flipbook ? Flipbook->GetRenderBounds().BoxExtent.Z * 2.0f : 0.0f;
	const float UniformScale = ActiveProfile.bUseNativeScale || NativeWorldHeight <= 0.0f
	                               ? 1.0f
	                               : ActiveProfile.WorldHeight / NativeWorldHeight;
	SetRelativeScale3D(FVector(UniformScale * FacingSign, UniformScale, UniformScale));
}
