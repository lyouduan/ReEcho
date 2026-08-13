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

bool UReEcho2DAnimationComponent::PlayClip(const FReEcho2DAnimationClip& Clip, const bool bRestart)
{
	if (!Clip.IsValid())
	{
		return false;
	}

	const bool bClipChanged = GetFlipbook() != Clip.Flipbook;
	const bool bPolicyChanged = IsLooping() != Clip.bLooping || !FMath::IsNearlyEqual(GetPlayRate(), Clip.PlayRate);
	ActiveClip = Clip;
	bAnimationActive = true;
	SetFlipbook(Clip.Flipbook);
	SetLooping(Clip.bLooping);
	SetPlayRate(Clip.PlayRate);
	SetRelativeLocation(Clip.LocalOffset);
	SetTranslucentSortPriority(Clip.TranslucentSortPriority);
	SetHiddenInGame(false);
	SetVisibility(true);
	SetComponentTickEnabled(true);
	ApplyDisplayScale();
	if (bRestart || Clip.bRestartOnRequest || bClipChanged || bPolicyChanged || !IsPlaying())
	{
		PlayFromStart();
	}
	return true;
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
	FReEcho2DAnimationClip Clip;
	Clip.Flipbook = ActiveProfile.DefaultFlipbook;
	Clip.bLooping = true;
	Clip.bUseNativeScale = ActiveProfile.bUseNativeScale;
	Clip.WorldHeight = ActiveProfile.WorldHeight;
	Clip.LocalOffset = ActiveProfile.LocalOffset;
	Clip.TranslucentSortPriority = ActiveProfile.TranslucentSortPriority;
	PlayClip(Clip, true);
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
	FReEcho2DAnimationClip Clip = ActiveClip;
	Clip.Flipbook = ResolvedFlipbook;
	Clip.bLooping = bShouldLoop;
	Clip.bRestartOnRequest = bRestart;
	PlayClip(Clip, bRestart);
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
	ActiveClip = FReEcho2DAnimationClip();
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

int32 UReEcho2DAnimationComponent::GetCurrentKeyFrameIndex()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	if (!bAnimationActive || !Flipbook || Flipbook->GetNumFrames() <= 0)
	{
		return INDEX_NONE;
	}
	return FMath::Clamp(Flipbook->GetKeyFrameIndexAtTime(GetPlaybackPosition(), true), 0, Flipbook->GetNumFrames() - 1);
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

void UReEcho2DAnimationComponent::ApplyDisplayScale()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	const float NativeWorldHeight = Flipbook ? Flipbook->GetRenderBounds().BoxExtent.Z * 2.0f : 0.0f;
	const float UniformScale =
	    ActiveClip.bUseNativeScale || NativeWorldHeight <= 0.0f ? 1.0f : ActiveClip.WorldHeight / NativeWorldHeight;
	SetRelativeScale3D(FVector(UniformScale * FacingSign, UniformScale, UniformScale));
}
