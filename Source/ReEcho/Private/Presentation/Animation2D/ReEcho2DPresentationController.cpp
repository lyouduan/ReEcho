#include "Presentation/Animation2D/ReEcho2DPresentationController.h"

#include "Components/BillboardComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"

UReEcho2DPresentationController::UReEcho2DPresentationController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UReEcho2DPresentationController::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePlaybackCompletion();
}

void UReEcho2DPresentationController::UpdatePlaybackCompletion()
{
	if (bWaitingForOneShot && AnimationRenderer && !AnimationRenderer->IsPlaying())
	{
		bWaitingForOneShot = false;
		if (CollisionDriver && ActiveAttackInstanceId >= 0)
		{
			CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
		}
		ActiveAttackInstanceId = INDEX_NONE;
		if (!bDeathLocked)
		{
			ApplyBaseState();
		}
	}
}

void UReEcho2DPresentationController::Configure(UBillboardComponent* InStaticRenderer,
	UReEcho2DAnimationComponent* InAnimationRenderer,
	UReEcho2DCharacterPresentationProfile* InProfile, const FName InWeaponVisualSetId)
{
	StaticRenderer = InStaticRenderer;
	AnimationRenderer = InAnimationRenderer;
	Profile = InProfile;
	WeaponVisualSetId = InWeaponVisualSetId;
	if (CollisionDriver)
	{
		CollisionDriver->BindRenderer(AnimationRenderer);
	}
	bWaitingForOneShot = false;
	bDeathLocked = false;
	ActiveAttackInstanceId = INDEX_NONE;
	ApplyBaseState(true);
}

void UReEcho2DPresentationController::ClearProfile()
{
	Profile = nullptr;
	WeaponVisualSetId = NAME_None;
	ActiveSemanticKey = FGameplayTag();
	bWaitingForOneShot = false;
	bDeathLocked = false;
	ActiveAttackInstanceId = INDEX_NONE;
	ShowStaticFallback();
}

void UReEcho2DPresentationController::SetWeaponVisualSetId(const FName InWeaponVisualSetId)
{
	if (WeaponVisualSetId == InWeaponVisualSetId)
	{
		return;
	}
	WeaponVisualSetId = InWeaponVisualSetId;
	if (!bDeathLocked && !bWaitingForOneShot)
	{
		ApplyBaseState(true);
	}
}

void UReEcho2DPresentationController::SetMoving(const bool bInMoving)
{
	if (bMoving == bInMoving)
	{
		return;
	}
	bMoving = bInMoving;
	if (!bDeathLocked && !bWaitingForOneShot)
	{
		ApplyBaseState();
	}
}

bool UReEcho2DPresentationController::PlayAction(const FGameplayTag SemanticKey, const bool bRestart,
	const int64 AttackInstanceId)
{
	if (bDeathLocked && SemanticKey != ReEcho2DAnimationTags::Death)
	{
		return false;
	}
	if (!ApplySemantic(SemanticKey, bRestart))
	{
		return false;
	}
	if (CollisionDriver && ActiveAttackInstanceId >= 0 && ActiveAttackInstanceId != AttackInstanceId)
	{
		CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
	}
	bDeathLocked = SemanticKey == ReEcho2DAnimationTags::Death;
	bWaitingForOneShot = AnimationRenderer && !AnimationRenderer->IsLooping();
	ActiveAttackInstanceId = AttackInstanceId;
	if (CollisionDriver && ActiveAttackInstanceId >= 0)
	{
		CollisionDriver->BeginAttackInstance(ActiveAttackInstanceId);
	}
	return true;
}

void UReEcho2DPresentationController::BindCollisionDriver(UReEcho2DFrameCollisionDriver* InCollisionDriver)
{
	CollisionDriver = InCollisionDriver;
	if (CollisionDriver)
	{
		CollisionDriver->BindRenderer(AnimationRenderer);
	}
}

void UReEcho2DPresentationController::SetFacingSign(const float FacingSign)
{
	if (AnimationRenderer)
	{
		AnimationRenderer->SetFacingSign(FacingSign);
	}
}

void UReEcho2DPresentationController::ApplyBaseState(const bool bRestart)
{
	const FGameplayTag DesiredKey = bMoving ? ReEcho2DAnimationTags::Move : ReEcho2DAnimationTags::Idle;
	if (!ApplySemantic(DesiredKey, bRestart))
	{
		ShowStaticFallback();
	}
}

bool UReEcho2DPresentationController::ApplySemantic(const FGameplayTag SemanticKey, const bool bRestart)
{
	if (!Profile || !AnimationRenderer)
	{
		return false;
	}
	const FReEcho2DAnimationClip* Clip = Profile->ResolveClip(WeaponVisualSetId, SemanticKey);
	if (!Clip || !AnimationRenderer->PlayClip(*Clip, bRestart))
	{
		return false;
	}
	ActiveSemanticKey = SemanticKey;
	AnimationRenderer->SetVisibility(true);
	AnimationRenderer->SetHiddenInGame(false);
	if (StaticRenderer)
	{
		StaticRenderer->SetVisibility(false);
		StaticRenderer->SetHiddenInGame(true);
	}
	return true;
}

void UReEcho2DPresentationController::ShowStaticFallback()
{
	ActiveSemanticKey = FGameplayTag();
	if (AnimationRenderer)
	{
		AnimationRenderer->DeactivateAnimation();
	}
	if (StaticRenderer)
	{
		if (Profile && Profile->StaticFallback)
		{
			StaticRenderer->SetSprite(Profile->StaticFallback);
		}
		StaticRenderer->SetVisibility(true);
		StaticRenderer->SetHiddenInGame(false);
	}
}
