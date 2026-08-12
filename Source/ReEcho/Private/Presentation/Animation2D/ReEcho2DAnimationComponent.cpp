#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"

#include "PaperFlipbook.h"

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
	PrimaryComponentTick.bCanEverTick = false;
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
	if (InProfile.WorldHeight <= 0.0f)
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
	SetRelativeLocation(ActiveProfile.LocalOffset);
	SetTranslucentSortPriority(ActiveProfile.TranslucentSortPriority);
	ApplyFlipbook(ActiveProfile.DefaultFlipbook);
	SetHiddenInGame(false);
	SetVisibility(true);
	PlayFromStart();
	return EReEcho2DAnimationActivationResult::Activated;
}

bool UReEcho2DAnimationComponent::SetAnimationState(const EReEcho2DAnimationState NewState)
{
	if (!bAnimationActive)
	{
		return false;
	}
	ActiveState = NewState;
	ApplyFlipbook(ActiveProfile.Resolve(NewState));
	return GetFlipbook() != nullptr;
}

void UReEcho2DAnimationComponent::DeactivateAnimation()
{
	Stop();
	SetFlipbook(nullptr);
	SetVisibility(false);
	SetHiddenInGame(true);
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

void UReEcho2DAnimationComponent::ApplyFlipbook(UPaperFlipbook* NewFlipbook)
{
	if (!NewFlipbook)
	{
		NewFlipbook = ActiveProfile.DefaultFlipbook;
	}
	if (GetFlipbook() != NewFlipbook)
	{
		SetFlipbook(NewFlipbook);
		PlayFromStart();
	}
	ApplyDisplayScale();
}

void UReEcho2DAnimationComponent::ApplyDisplayScale()
{
	const UPaperFlipbook* Flipbook = GetFlipbook();
	const float NativeWorldHeight = Flipbook ? Flipbook->GetRenderBounds().BoxExtent.Z * 2.0f : 0.0f;
	const float UniformScale = NativeWorldHeight > 0.0f ? ActiveProfile.WorldHeight / NativeWorldHeight : 1.0f;
	SetRelativeScale3D(FVector(UniformScale * FacingSign, UniformScale, UniformScale));
}
