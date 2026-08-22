#include "Presentation/Animation2D/ReEcho2DPresentationController.h"

#include "ReEchoPresentation.h"
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DAnimationStateMachineAsset.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"

UReEcho2DPresentationController::UReEcho2DPresentationController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UReEcho2DPresentationController::TickComponent(const float DeltaTime,
                                                    const ELevelTick TickType,
                                                    FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePlaybackCompletion();
}

void UReEcho2DPresentationController::UpdatePlaybackCompletion()
{
	if (bWaitingForOneShot && AnimationRenderer && !AnimationRenderer->IsPlaying())
	{
		bActionActive = false;
		bWaitingForOneShot = false;
		if (CollisionDriver && ActiveAttackInstanceId >= 0)
		{
			CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
		}
		ActiveAttackInstanceId = INDEX_NONE;
		{
			const FReEcho2DAnimationStateDefinition* ActiveState =
			    Profile && Profile->StateMachine ? Profile->StateMachine->FindState(ActiveStateTag) : nullptr;
			if (!ActiveState || !ActiveState->CompletionStateTag.IsValid())
			{
				ApplyBaseState();
			}
			else if (const FReEcho2DAnimationStateDefinition* CompletionState =
			             Profile->StateMachine->FindState(ActiveState->CompletionStateTag))
			{
				ApplySemantic(CompletionState->SemanticKey, true);
			}
		}
	}
}

void UReEcho2DPresentationController::Configure(UReEcho2DAnimationComponent* InAnimationRenderer,
                                                UReEcho2DCharacterPresentationProfile* InProfile,
                                                const FName InWeaponVisualSetId)
{
	AnimationRenderer = InAnimationRenderer;
	Profile = InProfile;
	WeaponVisualSetId = InWeaponVisualSetId;
	if (CollisionDriver)
	{
		CollisionDriver->BindRenderer(AnimationRenderer);
	}
	bWaitingForOneShot = false;
	bActionActive = false;
	ActiveAttackInstanceId = INDEX_NONE;
	if (!Profile && GetOwner() && !GetOwner()->HasAnyFlags(RF_ClassDefaultObject))
	{
		UE_LOG(LogReEchoPresentation,
		       Warning,
		       TEXT("Animation2D profile is missing for '%s'; retaining the Gameplay Blueprint fallback Flipbook."),
		       *GetNameSafe(GetOwner()));
	}
	ApplyBaseState(true);
}

void UReEcho2DPresentationController::Configure(UBillboardComponent* InStaticRenderer,
                                                UReEcho2DAnimationComponent* InAnimationRenderer,
                                                UReEcho2DCharacterPresentationProfile* InProfile)
{
	Configure(InAnimationRenderer, InProfile, NAME_None);
}

void UReEcho2DPresentationController::ClearProfile()
{
	Profile = nullptr;
	WeaponVisualSetId = NAME_None;
	ActiveSemanticKey = FGameplayTag();
	ActiveStateTag = FGameplayTag();
	bActionActive = false;
	bWaitingForOneShot = false;
	ActiveAttackInstanceId = INDEX_NONE;
	DeactivatePresentation();
}

void UReEcho2DPresentationController::SetWeaponVisualSetId(const FName InWeaponVisualSetId)
{
	if (WeaponVisualSetId == InWeaponVisualSetId)
	{
		return;
	}
	WeaponVisualSetId = InWeaponVisualSetId;
	if (!bActionActive)
	{
		ApplyBaseState(true);
	}
}

bool UReEcho2DPresentationController::BeginAnimationSetTransition(const FName InAnimationSetId,
                                                                  const FGameplayTag TransitionSemanticKey)
{
	if (CollisionDriver && ActiveAttackInstanceId >= 0)
	{
		CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
	}
	ActiveAttackInstanceId = INDEX_NONE;
	bActionActive = false;
	bWaitingForOneShot = false;
	ActiveStateTag = FGameplayTag();
	WeaponVisualSetId = InAnimationSetId;
	return PlayAction(TransitionSemanticKey, true);
}

void UReEcho2DPresentationController::CompleteAnimationSetTransition(const FName InAnimationSetId)
{
	if (CollisionDriver && ActiveAttackInstanceId >= 0)
	{
		CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
	}
	ActiveAttackInstanceId = INDEX_NONE;
	bActionActive = false;
	bWaitingForOneShot = false;
	ActiveStateTag = FGameplayTag();
	WeaponVisualSetId = InAnimationSetId;
	ApplyBaseState(true);
}

void UReEcho2DPresentationController::SetMoving(const bool bInMoving)
{
	if (bMoving == bInMoving)
	{
		return;
	}
	bMoving = bInMoving;
	if (!bActionActive)
	{
		ApplyBaseState();
	}
}

bool UReEcho2DPresentationController::PlayAction(const FGameplayTag SemanticKey,
                                                 const bool bRestart,
                                                 const int64 AttackInstanceId)
{
	const FReEcho2DAnimationStateDefinition* DesiredState = ResolveState(SemanticKey);
	if (!CanEnterState(DesiredState))
	{
		return false;
	}
	if (!ApplySemantic(SemanticKey, bRestart))
	{
		if (GetOwner() && !GetOwner()->HasAnyFlags(RF_ClassDefaultObject))
		{
			UE_LOG(LogReEchoPresentation,
			       Warning,
			       TEXT("Animation2D action '%s' could not resolve for profile '%s' on '%s'."),
			       *SemanticKey.ToString(),
			       *GetPathNameSafe(Profile),
			       *GetNameSafe(GetOwner()));
		}
		return false;
	}
	if (CollisionDriver && ActiveAttackInstanceId >= 0 && ActiveAttackInstanceId != AttackInstanceId)
	{
		CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
	}
	bActionActive = true;
	bWaitingForOneShot = AnimationRenderer && !AnimationRenderer->IsLooping();
	ActiveAttackInstanceId = AttackInstanceId;
	if (CollisionDriver && ActiveAttackInstanceId >= 0)
	{
		CollisionDriver->BeginAttackInstance(ActiveAttackInstanceId);
	}
	return true;
}

bool UReEcho2DPresentationController::CancelAttackAction()
{
	if (ActiveSemanticKey != ReEcho2DAnimationTags::Attack_Charge &&
	    ActiveSemanticKey != ReEcho2DAnimationTags::Attack_Basic)
	{
		return false;
	}
	if (CollisionDriver && ActiveAttackInstanceId >= 0)
	{
		CollisionDriver->EndAttackInstance(ActiveAttackInstanceId);
	}
	ActiveAttackInstanceId = INDEX_NONE;
	bActionActive = false;
	bWaitingForOneShot = false;
	ActiveStateTag = FGameplayTag();
	ApplyBaseState(true);
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
	bActionActive = false;
	const FGameplayTag DesiredKey = bMoving ? ReEcho2DAnimationTags::Move : ReEcho2DAnimationTags::Idle;
	if (!ApplySemantic(DesiredKey, bRestart))
	{
		// Derived Blueprint component overrides are not applied while native and
		// skeleton CDOs are being assembled. Their serialized fallback is valid on
		// instances, so constructor-time presentation must remain side-effect free.
		if (GetOwner() && GetOwner()->HasAnyFlags(RF_ClassDefaultObject))
		{
			return;
		}
		if (AnimationRenderer && AnimationRenderer->GetFlipbook())
		{
			FReEcho2DAnimationClip FallbackClip;
			FallbackClip.Flipbook = AnimationRenderer->GetFlipbook();
			FallbackClip.bLooping = true;
			FallbackClip.bUseNativeScale = true;
			AnimationRenderer->PlayClip(FallbackClip, bRestart);
			UE_LOG(LogReEchoPresentation,
			       Warning,
			       TEXT("Animation2D semantic '%s' could not resolve for '%s'; using Blueprint fallback '%s'."),
			       *DesiredKey.ToString(),
			       *GetNameSafe(GetOwner()),
			       *GetPathNameSafe(AnimationRenderer->GetFlipbook()));
		}
		else
		{
			DeactivatePresentation();
			UE_LOG(LogReEchoPresentation,
			       Error,
			       TEXT("Animation2D semantic '%s' could not resolve for '%s' and no Blueprint fallback exists."),
			       *DesiredKey.ToString(),
			       *GetNameSafe(GetOwner()));
		}
	}
}

bool UReEcho2DPresentationController::ApplySemantic(const FGameplayTag SemanticKey, const bool bRestart)
{
	if (!Profile || !AnimationRenderer)
	{
		return false;
	}
	const FReEcho2DAnimationClip* Clip = Profile->ResolveClip(WeaponVisualSetId, SemanticKey);
	if (!Clip)
	{
		return false;
	}
	FReEcho2DAnimationClip ResolvedClip = *Clip;
	ResolvedClip.bUseNativeScale = false;
	ResolvedClip.WorldHeight = FMath::Max(Profile->WorldHeight, 1.0f);
	if (!AnimationRenderer->PlayClip(ResolvedClip, bRestart))
	{
		return false;
	}
	ActiveSemanticKey = SemanticKey;
	if (const FReEcho2DAnimationStateDefinition* State = ResolveState(SemanticKey))
	{
		ActiveStateTag = State->StateTag;
	}
	AnimationRenderer->SetVisibility(true);
	AnimationRenderer->SetHiddenInGame(false);
	return true;
}

const FReEcho2DAnimationStateDefinition*
UReEcho2DPresentationController::ResolveState(const FGameplayTag SemanticKey) const
{
	return Profile && Profile->StateMachine ? Profile->StateMachine->FindStateBySemantic(SemanticKey) : nullptr;
}

bool UReEcho2DPresentationController::CanEnterState(const FReEcho2DAnimationStateDefinition* DesiredState) const
{
	if (!DesiredState || !Profile || !Profile->StateMachine)
	{
		return true;
	}
	const FReEcho2DAnimationStateDefinition* CurrentState = Profile->StateMachine->FindState(ActiveStateTag);
	if (!CurrentState)
	{
		return true;
	}
	if (CurrentState->bTerminal && CurrentState->StateTag != DesiredState->StateTag)
	{
		return false;
	}
	return !CurrentState->bLockUntilPlaybackComplete || !bActionActive ||
	       DesiredState->InterruptPriority >= CurrentState->InterruptPriority;
}

void UReEcho2DPresentationController::DeactivatePresentation()
{
	ActiveSemanticKey = FGameplayTag();
	ActiveStateTag = FGameplayTag();
	bActionActive = false;
	if (AnimationRenderer)
	{
		AnimationRenderer->DeactivateAnimation();
	}
}

// ReEchoPresentation runtime implementation.
