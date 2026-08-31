#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "UI/ReEchoElementReactionPopupActor.h"

namespace ReEchoEnemyVisual
{
constexpr float HitReactionDuration = 0.22f;
constexpr float BlinkSlamDuration = 0.5f;
constexpr float BlinkSlamStartHeightCm = 300.0f;
constexpr float Phase3BlinkSlamWindupDuration = 0.5f;
constexpr float Phase3BlinkSlamWindupHeightCm = 300.0f;
constexpr float Phase3LandingHoldDuration = 0.1f;
constexpr float Phase3AnimationScaleMultiplier = 2.0f;
constexpr float Phase3AttackScaleMultiplier = 2.0f;
constexpr float DeathKnockbackDurationSeconds = 0.3f;
constexpr float DeathKnockbackDistanceCm = 90.0f;
constexpr TCHAR MoonStaffProfilePath[] =
    TEXT("/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_MoonStaff.DA_WeaponPresentation_MoonStaff");

FVector
ResolveBossWeaponFacingOffset(const float FacingSign, const FVector& RightFacingOffset, const FVector& LeftFacingOffset)
{
	return FacingSign < 0.0f ? LeftFacingOffset : RightFacingOffset;
}
}

UReEchoEnemyPresentationComponent::UReEchoEnemyPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoEnemyPresentationComponent::SetPresentationCatalog(UReEcho2DPresentationCatalog* InPresentationCatalog)
{
	PresentationCatalog = InPresentationCatalog;
}

bool UReEchoEnemyPresentationComponent::IsBornPlaying() const
{
	return PresentationController && PresentationController->GetActiveSemanticKey() == ReEcho2DAnimationTags::Born;
}

void UReEchoEnemyPresentationComponent::ConfigureComponents(USceneComponent* InPresentationRoot,
                                                            USceneComponent* InVisualEffectRoot,
                                                            USceneComponent* InFootRoot,
                                                            USceneComponent* InFlipbookRoot,
                                                            USceneComponent* InEffectsRoot,
                                                            USceneComponent* InBossWeaponRoot,
                                                            USceneComponent* InBossWeaponFacingRoot,
                                                            USceneComponent* InBossWeaponTipRoot,
                                                            UBillboardComponent* InBossWeaponSprite,
                                                            UBillboardComponent* InCharacterSprite,
                                                            UReEcho2DAnimationComponent* InSequenceAnimation,
                                                            UReEcho2DPresentationController* InPresentationController,
                                                            UReEcho2DFrameCollisionDriver* InFrameCollisionDriver,
                                                            UStaticMeshComponent* InGroundShadow,
                                                            UBoxComponent* InCollision)
{
	PresentationRoot = InPresentationRoot;
	VisualEffectRoot = InVisualEffectRoot;
	FootRoot = InFootRoot;
	TerminalDeathMotionRoot = VisualEffectRoot ? VisualEffectRoot->GetAttachParent() : nullptr;
	FlipbookRoot = InFlipbookRoot;
	EffectsRoot = InEffectsRoot;
	BossWeaponRoot = InBossWeaponRoot;
	BossWeaponFacingRoot = InBossWeaponFacingRoot;
	BossWeaponTipRoot = InBossWeaponTipRoot;
	BossWeaponSprite = InBossWeaponSprite;
	CharacterSprite = InCharacterSprite;
	SequenceAnimation = InSequenceAnimation;
	PresentationController = InPresentationController;
	FrameCollisionDriver = InFrameCollisionDriver;
	GroundShadow = InGroundShadow;
	GroundRoot = GroundShadow ? GroundShadow->GetAttachParent() : nullptr;
	Collision = InCollision;
	AuthoredMotionLocation = VisualEffectRoot ? VisualEffectRoot->GetRelativeLocation() : FVector::ZeroVector;
	AuthoredTerminalDeathMotionLocation =
	    TerminalDeathMotionRoot ? TerminalDeathMotionRoot->GetRelativeLocation() : FVector::ZeroVector;
	if (PresentationController)
	{
		PresentationController->BindCollisionDriver(FrameCollisionDriver);
	}
	if (GroundShadow)
	{
		AuthoredGroundRootLocation = GroundRoot ? GroundRoot->GetRelativeLocation() : FVector::ZeroVector;
		AuthoredGroundShadowScale = GroundShadow->GetRelativeScale3D();
		GroundShadow->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		if (!GroundShadow->GetMaterial(0))
		{
			GroundShadow->SetMaterial(
			    0,
			    LoadObject<UMaterialInterface>(
			        nullptr, TEXT("/Game/ReEcho/Materials/M_GroundShadow_Procedural.M_GroundShadow_Procedural")));
		}
	}
}

void UReEchoEnemyPresentationComponent::BindEventSources(AActor* InHost,
                                                         UReEchoEnemyEventsComponent* InEnemyEvents,
                                                         UReEchoCombatEventsComponent* InCombatEvents)
{
	if (EnemyEvents)
	{
		EnemyEvents->OnBossIntent.RemoveAll(this);
		EnemyEvents->OnPhaseTransition.RemoveAll(this);
		EnemyEvents->OnFuseChanged.RemoveAll(this);
	}
	if (CombatPresentationCoordinator)
	{
		CombatPresentationCoordinator->OnActionPhase.RemoveAll(this);
	}
	if (CombatEvents)
	{
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnElementReactionResolved.RemoveAll(this);
	}

	Host = InHost;
	EnemyEvents = InEnemyEvents;
	CombatEvents = InCombatEvents;
	CombatPresentationCoordinator =
	    InHost ? InHost->FindComponentByClass<UReEchoCombatPresentationCoordinator>() : nullptr;
	if (EnemyEvents)
	{
		EnemyEvents->OnBossIntent.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleBossIntent);
		EnemyEvents->OnPhaseTransition.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandlePhaseTransition);
		EnemyEvents->OnFuseChanged.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleFuseChanged);
	}
	if (CombatPresentationCoordinator)
	{
		CombatPresentationCoordinator->OnActionPhase.AddDynamic(
		    this, &UReEchoEnemyPresentationComponent::HandlePresentationAction);
	}
	if (CombatEvents)
	{
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleCombatHurt);
		CombatEvents->OnElementReactionResolved.AddDynamic(
		    this, &UReEchoEnemyPresentationComponent::HandleElementReactionResolved);
	}
}

void UReEchoEnemyPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindEventSources(nullptr, nullptr, nullptr);
	Super::EndPlay(EndPlayReason);
}

void UReEchoEnemyPresentationComponent::ConfigureAppearance(const FName PresentationId)
{
	ApplyVisual(PresentationId);
	ConfigureBossWeapon(PresentationId);
}

bool UReEchoEnemyPresentationComponent::TryPlayBorn()
{
	return PresentationController && PresentationController->PlayAction(ReEcho2DAnimationTags::Born);
}

void UReEchoEnemyPresentationComponent::CancelBornForRuntimeRestore()
{
	if (!IsBornPlaying())
	{
		return;
	}
	if (SequenceAnimation)
	{
		SequenceAnimation->Stop();
	}
	PresentationController->UpdatePlaybackCompletion();
}

void UReEchoEnemyPresentationComponent::ConfigureBossWeapon(const FName PresentationId)
{
	const bool bTimeGuard = PresentationId == TEXT("Enemy.TimeGuard");
	if (!BossWeaponRoot || !BossWeaponFacingRoot || !BossWeaponTipRoot || !BossWeaponSprite)
	{
		return;
	}
	BossWeaponSprite->SetVisibility(bTimeGuard);
	BossWeaponSprite->SetHiddenInGame(!bTimeGuard);
	if (!bTimeGuard)
	{
		return;
	}
	const UReEchoWeaponPresentationProfile* WeaponProfile =
	    LoadObject<UReEchoWeaponPresentationProfile>(nullptr, ReEchoEnemyVisual::MoonStaffProfilePath);
	UTexture2D* HeldTexture = WeaponProfile ? WeaponProfile->HeldTexture.LoadSynchronous() : nullptr;
	if (!WeaponProfile || !HeldTexture || !ActiveProfile)
	{
		BossWeaponSprite->SetVisibility(false);
		BossWeaponSprite->SetHiddenInGame(true);
		return;
	}
	const float CharacterWorldHeight = FMath::Max(ActiveProfile->WorldHeight, 1.0f);
	const float HeldLength = WeaponProfile->bOverrideHeldLength
	                             ? FMath::Max(WeaponProfile->HeldLengthOverrideCm, 1.0f)
	                             : CharacterWorldHeight * FMath::Max(WeaponProfile->HeldLengthRatio, 0.01f);
	const float TextureAxisLength = WeaponProfile->HeldSizeAxis == EReEchoHeldWeaponSizeAxis::Width
	                                    ? FMath::Max(HeldTexture->GetSizeX(), 1)
	                                    : FMath::Max(HeldTexture->GetSizeY(), 1);
	BossWeaponRightFacingOffset = WeaponProfile->HeldRightFacingOffsetRatio * CharacterWorldHeight;
	BossWeaponLeftFacingOffset = WeaponProfile->HeldLeftFacingOffsetRatio * CharacterWorldHeight;
	RefreshBossWeaponFacingOffset(1.0f);
	BossWeaponRestRotation = WeaponProfile->HeldRotationOffset;
	BossWeaponRoot->SetRelativeRotation(BossWeaponRestRotation);
	BossWeaponSprite->SetSprite(HeldTexture);
	BossWeaponSprite->SetTranslucentSortPriority(7);
	BossWeaponSprite->SetRelativeTransform(FTransform::Identity);
	BossWeaponSprite->SetRelativeScale3D(FVector(HeldLength / TextureAxisLength));
	BossWeaponTipRoot->SetRelativeLocation(ResolveBossWeaponTipOffset(HeldLength));
}

FVector UReEchoEnemyPresentationComponent::ResolveBossWeaponTipOffset(const float HeldLengthCm)
{
	return FVector::UpVector * FMath::Max(0.0f, HeldLengthCm) * 0.5f;
}

void UReEchoEnemyPresentationComponent::ApplyVisual(const FName PresentationId)
{
	ActiveProfile = PresentationCatalog ? PresentationCatalog->ResolveProfile(PresentationId) : nullptr;
	UReEcho2DCharacterPresentationProfile* Profile = ActiveProfile.Get();
	if (CharacterSprite)
	{
		CharacterSprite->SetVisibility(!Profile);
		CharacterSprite->SetHiddenInGame(Profile != nullptr);
		CharacterSprite->SetRelativeLocation(FVector::ZeroVector);
	}
	if (PresentationController)
	{
		if (SequenceAnimation)
		{
			SequenceAnimation->SetDisplayScaleMultiplier(1.0f);
			SequenceAnimation->SetDisplayScaleReferenceHeight(0.0f);
		}
		PresentationController->Configure(CharacterSprite, SequenceAnimation, Profile);
	}
	if (VisualEffectRoot)
	{
		VisualEffectRoot->SetRelativeLocation(AuthoredMotionLocation);
		VisualEffectRoot->SetRelativeScale3D(FVector::OneVector);
		BaseVisualScale = VisualEffectRoot->GetRelativeScale3D();
	}
	if (TerminalDeathMotionRoot)
	{
		TerminalDeathMotionRoot->SetRelativeLocation(AuthoredTerminalDeathMotionLocation);
	}
	if (FlipbookRoot)
	{
		BaseFlipbookLocation = FlipbookRoot->GetRelativeLocation();
		BaseFlipbookScale = FlipbookRoot->GetRelativeScale3D();
	}
	if (EffectsRoot)
	{
		BaseEffectsLocation = EffectsRoot->GetRelativeLocation();
		BaseEffectsScale = EffectsRoot->GetRelativeScale3D();
	}
	VisualTime = 0.0f;
	AttackVisualRemaining = 0.0f;
	bHitVisualActive = false;
	bDeathVisualActive = false;
}

bool UReEchoEnemyPresentationComponent::BeginTerminalDeath(const FVector& KnockbackWorldDirection,
                                                           FSimpleDelegate OnCompleted,
                                                           float& OutExpectedDurationSeconds)
{
	OutExpectedDurationSeconds = 0.0f;
	if (bDeathVisualActive)
	{
		return false;
	}
	bDeathVisualActive = true;
	bHitVisualActive = false;
	bStunPaused = false;
	bCancelAttackWhenStunClears = false;
	if (SequenceAnimation)
	{
		SequenceAnimation->SetPlaybackPaused(false);
	}
	AttackVisualRemaining = 0.0f;
	ResetTransientRoot();
	DeathKnockbackElapsedSeconds = 0.0f;
	DeathKnockbackLocalDirection = PresentationRoot ? PresentationRoot->GetComponentTransform()
	                                                      .InverseTransformVectorNoScale(KnockbackWorldDirection)
	                                                      .GetSafeNormal2D()
	                                                : KnockbackWorldDirection.GetSafeNormal2D();
	if (EffectsRoot)
	{
		EffectsRoot->SetVisibility(false, true);
		EffectsRoot->SetHiddenInGame(true, true);
	}
	if (CurrentBossPhaseIndex >= 3 && PresentationController)
	{
		// Phase3 has Walk/Slam only. Its terminal presentation explicitly reuses the black-goat Death,
		// rather than letting the missing Phase3 clip fall back to the default white-goat set.
		PresentationController->SetWeaponVisualSetId(TEXT("Phase2"));
		if (SequenceAnimation)
		{
			SequenceAnimation->SetDisplayScaleReferenceHeight(0.0f);
			SequenceAnimation->SetDisplayScaleMultiplier(1.0f);
		}
	}
	const bool bStarted = PresentationController &&
	                      PresentationController->BeginTerminalDeath(MoveTemp(OnCompleted), OutExpectedDurationSeconds);
	if (bStarted)
	{
		// The shadow is grounded presentation, not a transient effect. Keep its authored visibility and
		// realign it after switching to the Death Flipbook so the final pose retains contact with the floor.
		ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
	}
	return bStarted;
}

void UReEchoEnemyPresentationComponent::SetStunPaused(const bool bPaused)
{
	bStunPaused = bPaused;
	if (SequenceAnimation)
	{
		SequenceAnimation->SetPlaybackPaused(bPaused);
	}
	if (!bPaused && bCancelAttackWhenStunClears)
	{
		bCancelAttackWhenStunClears = false;
		if (PresentationController)
		{
			PresentationController->CancelAttackAction();
		}
	}
}

void UReEchoEnemyPresentationComponent::Advance(const FReEchoEnemyPresentationSnapshot& Snapshot,
                                                const float DeltaSeconds)
{
	const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
	if (bSacrificeVisualActive)
	{
		return;
	}
	if (Host && CharacterSprite && SequenceAnimation && !SequenceAnimation->IsAnimationActive())
	{
		ReEchoBillboardDebug::DrawBounds(
		    Host, CharacterSprite, Snapshot.Archetype == EReEchoEnemyArchetype::Boss ? FColor::Yellow : FColor::Red);
	}
	if (Host && Collision)
	{
		ReEchoCollisionDebug::DrawBox(
		    Host, Collision, Snapshot.Archetype == EReEchoEnemyArchetype::Boss ? FColor::Orange : FColor::Cyan);
	}
	UpdateCameraFacing(Snapshot);
	if (Snapshot.Phase == EReEchoEnemyBehaviorPhase::Dead || bDeathVisualActive)
	{
		bCancelAttackWhenStunClears = false;
		SetStunPaused(false);
		UpdateDeathKnockbackMotion(SafeDelta);
		return;
	}
	SetStunPaused(Snapshot.bStunned);
	if (Snapshot.bStunned)
	{
		return;
	}
	VisualTime += SafeDelta;
	UpdateBossWeaponMotion(SafeDelta);
	if (Snapshot.Phase == EReEchoEnemyBehaviorPhase::HitReaction)
	{
		UpdateHitReaction(Snapshot);
		return;
	}
	if (bHitVisualActive)
	{
		ResetTransientRoot();
		bHitVisualActive = false;
	}
	UpdateSpriteAnimation(Snapshot, SafeDelta);
	UpdateBossPhase3BlinkSlamWindupMotion(SafeDelta);
	UpdateBossBlinkSlamMotion(SafeDelta);
}

void UReEchoEnemyPresentationComponent::NormalizeSacrificeBornDuration()
{
	if (IsBornPlaying() && SequenceAnimation && SequenceAnimation->GetFlipbook())
	{
		// Runtime-only rate: preserve the complete clip and let normal completion enter the base pose.
		SequenceAnimation->SetLooping(false);
		SequenceAnimation->SetPlayRate(SequenceAnimation->GetFlipbookLength() / 0.5f);
	}
}

bool UReEchoEnemyPresentationComponent::PrepareSacrificeGroundPose()
{
	if (bSacrificeVisualActive)
	{
		return true;
	}
	if (PresentationController)
	{
		PresentationController->UpdatePlaybackCompletion();
	}
	// Host gameplay Tick is frozen during the push, but Born still needs per-frame foot alignment.
	ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
	return !IsBornPlaying();
}

bool UReEchoEnemyPresentationComponent::BeginSacrificeVisual()
{
	if (!PrepareSacrificeGroundPose())
	{
		return false;
	}
	if (bSacrificeVisualActive || bDeathVisualActive || !VisualEffectRoot || !SequenceAnimation ||
	    !SequenceAnimation->GetFlipbook() || !SequenceAnimation->IsVisible())
	{
		return false;
	}
	bSacrificeVisualActive = true;
	bSacrificeTransformPlayed = false;
	bSacrificeTransformStartedSuccessfully = false;
	SacrificeOriginalTransform = VisualEffectRoot->GetRelativeTransform();
	SacrificeOriginalColor = SequenceAnimation->GetSpriteColor();
	SacrificeRiseHeight = FMath::Max(20.0f, SequenceAnimation->Bounds.BoxExtent.Z * 2.0f) * 1.8f;
	bSacrificeOriginalShadowVisible = GroundShadow && GroundShadow->IsVisible();
	if (PresentationController)
	{
		bSacrificeOriginalControllerTick = PresentationController->IsComponentTickEnabled();
		PresentationController->SetComponentTickEnabled(false);
	}
	return true;
}

void UReEchoEnemyPresentationComponent::PlaySacrificeTransformAnimation()
{
	if (!bSacrificeVisualActive || bSacrificeTransformPlayed)
	{
		return;
	}
	bSacrificeTransformPlayed = true;
	// Presentation only: use this monster's authored transition, without changing gameplay phase/stats.
	if (PresentationController)
	{
		bSacrificeTransformStartedSuccessfully = PresentationController->BeginAnimationSetTransition(
		    TEXT("Phase2"), ReEcho2DAnimationTags::Transform_Phase2);
		if (bSacrificeTransformStartedSuccessfully && SequenceAnimation)
		{
			SequenceAnimation->SetLooping(false);
		}
	}
}

bool UReEchoEnemyPresentationComponent::IsSacrificeTransformComplete() const
{
	// The controller is deliberately frozen; query renderer completion, not its stale semantic state.
	return !bSacrificeTransformStartedSuccessfully || !SequenceAnimation ||
	       (!SequenceAnimation->IsPlaying() && !SequenceAnimation->IsPlaybackPaused());
}

void UReEchoEnemyPresentationComponent::UpdateSacrificeVisual(const float RiseAlpha,
                                                              const float Opacity,
                                                              const FVector& ShakeWorldOffset)
{
	if (!bSacrificeVisualActive || !VisualEffectRoot || !SequenceAnimation)
	{
		return;
	}
	const FVector WorldOffset =
	    FVector::UpVector * SacrificeRiseHeight * FMath::Clamp(RiseAlpha, 0.0f, 1.0f) + ShakeWorldOffset;
	const USceneComponent* Parent = VisualEffectRoot->GetAttachParent();
	const FVector LocalOffset =
	    Parent ? Parent->GetComponentTransform().InverseTransformVector(WorldOffset) : WorldOffset;
	VisualEffectRoot->SetRelativeLocation(SacrificeOriginalTransform.GetLocation() + LocalOffset);
	FLinearColor Color = SacrificeOriginalColor;
	Color.A *= FMath::Clamp(Opacity, 0.0f, 1.0f);
	SequenceAnimation->SetSpriteColor(Color);
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(bSacrificeOriginalShadowVisible && Opacity > 0.01f);
	}
}

FVector UReEchoEnemyPresentationComponent::GetSacrificeVisualCenter() const
{
	return SequenceAnimation ? SequenceAnimation->Bounds.Origin
	                         : (GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
}

void UReEchoEnemyPresentationComponent::EndSacrificeVisual()
{
	if (!bSacrificeVisualActive)
	{
		return;
	}
	bSacrificeVisualActive = false;
	if (VisualEffectRoot)
	{
		VisualEffectRoot->SetRelativeTransform(SacrificeOriginalTransform);
	}
	if (SequenceAnimation)
	{
		SequenceAnimation->SetSpriteColor(SacrificeOriginalColor);
	}
	if (GroundShadow)
	{
		GroundShadow->SetVisibility(bSacrificeOriginalShadowVisible);
	}
	if (PresentationController)
	{
		PresentationController->SetComponentTickEnabled(bSacrificeOriginalControllerTick);
	}
}

FVector UReEchoEnemyPresentationComponent::ResolveBlinkSlamVisualOffset(const float RemainingSeconds,
                                                                        const float DurationSeconds,
                                                                        const float StartHeightCm)
{
	if (DurationSeconds <= KINDA_SMALL_NUMBER || RemainingSeconds <= 0.0f || StartHeightCm <= 0.0f)
	{
		return FVector::ZeroVector;
	}
	const float RemainingRatio = FMath::Clamp(RemainingSeconds / DurationSeconds, 0.0f, 1.0f);
	return FVector::UpVector * StartHeightCm * FMath::Square(RemainingRatio);
}

FVector UReEchoEnemyPresentationComponent::ResolvePhase3BlinkSlamWindupOffset(const float RemainingSeconds,
                                                                              const float DurationSeconds,
                                                                              const float EndHeightCm)
{
	if (DurationSeconds <= KINDA_SMALL_NUMBER || RemainingSeconds <= 0.0f || EndHeightCm <= 0.0f)
	{
		return RemainingSeconds <= 0.0f ? FVector::UpVector * FMath::Max(0.0f, EndHeightCm) : FVector::ZeroVector;
	}
	const float Progress = 1.0f - FMath::Clamp(RemainingSeconds / DurationSeconds, 0.0f, 1.0f);
	const float FastRise = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
	return FVector::UpVector * EndHeightCm * FastRise;
}

float UReEchoEnemyPresentationComponent::ResolveCameraFacingSign(const FVector& FacingDirection,
                                                                 const FVector& CameraRight)
{
	return FVector::DotProduct(FacingDirection, CameraRight) < 0.0f ? -1.0f : 1.0f;
}

FVector UReEchoEnemyPresentationComponent::ResolveDeathKnockbackOffset(const FVector& LocalDirection,
                                                                       const float ElapsedSeconds,
                                                                       const float DurationSeconds,
                                                                       const float DistanceCm)
{
	if (DurationSeconds <= KINDA_SMALL_NUMBER || DistanceCm <= 0.0f || LocalDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	const float Progress = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	const float EaseOut = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
	return LocalDirection.GetSafeNormal2D() * DistanceCm * EaseOut;
}

void UReEchoEnemyPresentationComponent::UpdateDeathKnockbackMotion(const float DeltaSeconds)
{
	DeathKnockbackElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	ApplyTerminalDeathMotion(ResolveDeathKnockbackOffset(DeathKnockbackLocalDirection,
	                                                     DeathKnockbackElapsedSeconds,
	                                                     ReEchoEnemyVisual::DeathKnockbackDurationSeconds,
	                                                     ReEchoEnemyVisual::DeathKnockbackDistanceCm));
}

void UReEchoEnemyPresentationComponent::UpdateBossBlinkSlamMotion(const float DeltaSeconds)
{
	if (BossBlinkSlamRemaining <= 0.0f)
	{
		return;
	}
	BossBlinkSlamRemaining = FMath::Max(0.0f, BossBlinkSlamRemaining - DeltaSeconds);
	// Foot alignment must observe the final Phase3 scale. Applying the scale after alignment lets the enlarged
	// GroundSlam expand below its ground anchor for the first landing frame.
	ApplyBossPhase3FixedRendererScale();
	ApplyPresentationMotion(
	    ResolveBlinkSlamVisualOffset(BossBlinkSlamRemaining, BossBlinkSlamDuration, BossBlinkSlamStartHeightCm),
	    FVector::OneVector);
}

void UReEchoEnemyPresentationComponent::UpdateBossPhase3BlinkSlamWindupMotion(const float DeltaSeconds)
{
	if (BossPhase3BlinkSlamWindupRemaining <= 0.0f)
	{
		return;
	}
	BossPhase3BlinkSlamWindupRemaining =
	    FMath::Max(0.0f, BossPhase3BlinkSlamWindupRemaining - FMath::Max(0.0f, DeltaSeconds));
	if (bBossPhase3PendingWindupWalk)
	{
		BossPhase3LandingHoldRemaining =
		    FMath::Max(0.0f, BossPhase3LandingHoldRemaining - FMath::Max(0.0f, DeltaSeconds));
		if (BossPhase3LandingHoldRemaining <= 0.0f)
		{
			bBossPhase3PendingWindupWalk = false;
			bBossPhase3AttackScaleActive = false;
			if (PresentationController)
			{
				PresentationController->CancelAttackAction();
			}
		}
	}
	ApplyBossPhase3FixedRendererScale();
	ApplyPresentationMotion(ResolvePhase3BlinkSlamWindupOffset(BossPhase3BlinkSlamWindupRemaining,
	                                                           BossPhase3BlinkSlamWindupDuration,
	                                                           ReEchoEnemyVisual::Phase3BlinkSlamWindupHeightCm),
	                        FVector::OneVector);
	if (BossPhase3BlinkSlamWindupRemaining <= 0.0f)
	{
		bBossPhase3BlinkSlamHidden = true;
		if (SequenceAnimation)
		{
			SequenceAnimation->SetVisibility(false);
			SequenceAnimation->SetHiddenInGame(true);
		}
		if (GroundShadow)
		{
			GroundShadow->SetVisibility(false);
			GroundShadow->SetHiddenInGame(true);
		}
	}
}

void UReEchoEnemyPresentationComponent::ApplyBossPhase3FixedRendererScale()
{
	if (CurrentBossPhaseIndex < 3 || !SequenceAnimation)
	{
		return;
	}
	SequenceAnimation->SetDisplayScaleMultiplier(ReEchoEnemyVisual::Phase3AnimationScaleMultiplier);
}

FVector UReEchoEnemyPresentationComponent::ResolveBossPhase3AnimationScale(const FVector& AuthoredScale,
                                                                           const bool bAttackActive)
{
	return AuthoredScale.GetAbs() * (bAttackActive ? ReEchoEnemyVisual::Phase3AttackScaleMultiplier
	                                               : ReEchoEnemyVisual::Phase3AnimationScaleMultiplier);
}

void UReEchoEnemyPresentationComponent::ApplyBossPhase3Facing(const FVector& LockedDirection)
{
	if (!Host || !PresentationController || LockedDirection.IsNearlyZero())
	{
		return;
	}
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(Host, 0);
	if (!Camera)
	{
		return;
	}
	const FVector CameraRight = FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y);
	const float FacingSign = ResolveCameraFacingSign(LockedDirection, CameraRight);
	ApplyFacingSign(FacingSign);
	RefreshBossWeaponFacingOffset(FacingSign);
}

void UReEchoEnemyPresentationComponent::FitBossPhase3AttackPlaybackToWindow(const float WindowSeconds)
{
	if (!SequenceAnimation || WindowSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float ClipSeconds = SequenceAnimation->GetFlipbookLength();
	if (ClipSeconds > KINDA_SMALL_NUMBER)
	{
		SequenceAnimation->SetPlayRate(ClipSeconds / WindowSeconds);
	}
}

void UReEchoEnemyPresentationComponent::UpdateBossWeaponMotion(const float DeltaSeconds)
{
	if (!BossWeaponRoot || BossWeaponSwingDuration <= KINDA_SMALL_NUMBER || BossWeaponSwingRemaining <= 0.0f)
	{
		return;
	}
	BossWeaponSwingRemaining = FMath::Max(0.0f, BossWeaponSwingRemaining - DeltaSeconds);
	const float NormalizedTime = 1.0f - BossWeaponSwingRemaining / BossWeaponSwingDuration;
	const float SwingDegrees = FMath::Sin(NormalizedTime * PI) * 120.0f;
	BossWeaponRoot->SetRelativeRotation(BossWeaponRestRotation + FRotator(0.0f, 0.0f, SwingDegrees));
	if (BossWeaponSwingRemaining <= 0.0f)
	{
		BossWeaponRoot->SetRelativeRotation(BossWeaponRestRotation);
	}
}

void UReEchoEnemyPresentationComponent::UpdateCameraFacing(const FReEchoEnemyPresentationSnapshot& Snapshot)
{
	if (!Host)
	{
		return;
	}
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(Host, 0);
	if (!Camera)
	{
		return;
	}
	if (FlipbookRoot)
	{
		FlipbookRoot->SetWorldRotation(
		    UReEcho2DAnimationComponent::CalculateCameraFacingRotation(Camera->GetCameraRotation()));
	}
	const FVector CameraRight = FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y);
	const float FacingSign = ResolveCameraFacingSign(Snapshot.FacingDirection, CameraRight);
	ApplyFacingSign(FacingSign);
	RefreshBossWeaponFacingOffset(FacingSign);
}

void UReEchoEnemyPresentationComponent::ApplyFacingSign(const float FacingSign)
{
	if (PresentationController)
	{
		PresentationController->SetFacingSign(FacingSign);
	}
}

void UReEchoEnemyPresentationComponent::RefreshBossWeaponFacingOffset(const float FacingSign)
{
	if (!BossWeaponFacingRoot)
	{
		return;
	}
	BossWeaponFacingRoot->SetRelativeLocation(ReEchoEnemyVisual::ResolveBossWeaponFacingOffset(
	    FacingSign, BossWeaponRightFacingOffset, BossWeaponLeftFacingOffset));
}

#if WITH_DEV_AUTOMATION_TESTS
FVector UReEchoEnemyPresentationComponent::ResolveBossWeaponFacingOffsetForTests(const float FacingSign,
                                                                                 const FVector& RightFacingOffset,
                                                                                 const FVector& LeftFacingOffset)
{
	return ReEchoEnemyVisual::ResolveBossWeaponFacingOffset(FacingSign, RightFacingOffset, LeftFacingOffset);
}
#endif

void UReEchoEnemyPresentationComponent::ResetTransientRoot()
{
	if (TerminalDeathMotionRoot)
	{
		TerminalDeathMotionRoot->SetRelativeLocation(AuthoredTerminalDeathMotionLocation);
	}
	ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
}

void UReEchoEnemyPresentationComponent::ApplyTerminalDeathMotion(const FVector& Offset)
{
	if (TerminalDeathMotionRoot)
	{
		TerminalDeathMotionRoot->SetRelativeLocation(AuthoredTerminalDeathMotionLocation + Offset);
	}
	ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
}

void UReEchoEnemyPresentationComponent::ApplyPresentationMotion(const FVector& Offset, const FVector& Scale)
{
	if (FlipbookRoot)
	{
		FlipbookRoot->SetRelativeLocation(BaseFlipbookLocation);
		FlipbookRoot->SetRelativeScale3D(BaseFlipbookScale * Scale);
	}
	if (EffectsRoot)
	{
		EffectsRoot->SetRelativeLocation(BaseEffectsLocation);
		EffectsRoot->SetRelativeScale3D(BaseEffectsScale * Scale);
	}
	RefreshFootpointAlignment();
	if (VisualEffectRoot)
	{
		VisualEffectRoot->SetRelativeLocation(AuthoredMotionLocation + CalculatedFootAlignmentOffset + Offset);
		VisualEffectRoot->SetRelativeScale3D(BaseVisualScale);
	}
	RefreshGroundShadowFromFlipbook();
}

void UReEchoEnemyPresentationComponent::RefreshGroundShadowFromFlipbook()
{
	const UPaperFlipbook* Flipbook = SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr;
	const UStaticMesh* ShadowMesh = GroundShadow ? GroundShadow->GetStaticMesh() : nullptr;
	if (!FootRoot || !GroundRoot || !FlipbookRoot || !SequenceAnimation || !Flipbook || !GroundShadow || !ShadowMesh)
	{
		return;
	}

	FBoxSphereBounds FlipbookBounds = Flipbook->GetRenderBounds();
	const UPaperSprite* CurrentSprite = nullptr;
	if (bDeathVisualActive || IsBornPlaying())
	{
		CurrentSprite = Flipbook->GetSpriteAtTime(SequenceAnimation->GetPlaybackPosition(), true);
		if (CurrentSprite)
		{
			FlipbookBounds = CurrentSprite->GetRenderBounds();
		}
	}
	const bool bUseAuthoredDeathPivot =
	    bDeathVisualActive && CurrentSprite && ActiveProfile && ActiveProfile->bUseAuthoredDeathPivot;
	if (bUseAuthoredDeathPivot)
	{
		GroundRoot->SetRelativeLocation(AuthoredGroundRootLocation);
	}
	const FVector LocalGroundAnchor = bUseAuthoredDeathPivot
	                                      ? FVector::ZeroVector
	                                      : FVector(FlipbookBounds.Origin.X,
	                                                FlipbookBounds.Origin.Y,
	                                                FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomWorld = SequenceAnimation->GetComponentTransform().TransformPosition(LocalGroundAnchor);
	if (!bUseAuthoredDeathPivot)
	{
		const USceneComponent* GroundParent = GroundRoot->GetAttachParent();
		const FVector BottomInGroundParent =
		    GroundParent ? GroundParent->GetComponentTransform().InverseTransformPosition(BottomWorld)
		                 : FootRoot->GetComponentTransform().InverseTransformPosition(BottomWorld);
		GroundRoot->SetRelativeLocation(
		    FVector(BottomInGroundParent.X, BottomInGroundParent.Y, AuthoredGroundRootLocation.Z));
	}

	const float FlipbookWidth = UReEcho2DAnimationComponent::CalculateFlipbookPresentationWidth(
	    FlipbookBounds, SequenceAnimation->GetRelativeTransform(), FlipbookRoot->GetRelativeTransform());
	const float ShadowNativeWidth = ShadowMesh->GetBounds().BoxExtent.Y * 2.0f;
	if (FlipbookWidth <= UE_SMALL_NUMBER || ShadowNativeWidth <= UE_SMALL_NUMBER)
	{
		return;
	}

	FVector ShadowScale = AuthoredGroundShadowScale;
	ShadowScale.Y = FlipbookWidth / ShadowNativeWidth;
	GroundShadow->SetRelativeScale3D(ShadowScale);
}

void UReEchoEnemyPresentationComponent::RefreshFootpointAlignment()
{
	CalculatedFootAlignmentOffset = FVector::ZeroVector;
	const UPaperFlipbook* Flipbook = SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr;
	if (!FlipbookRoot || !SequenceAnimation || !Flipbook || (ActiveProfile && !ActiveProfile->bAutoAlignFootpoint))
	{
		return;
	}
	FBoxSphereBounds AlignmentBounds = Flipbook->GetRenderBounds();
	const UPaperSprite* CurrentSprite = nullptr;
	if (bDeathVisualActive || IsBornPlaying())
	{
		CurrentSprite = Flipbook->GetSpriteAtTime(SequenceAnimation->GetPlaybackPosition(), true);
		if (CurrentSprite)
		{
			AlignmentBounds = CurrentSprite->GetRenderBounds();
		}
	}
	const FVector ProfileFootpointOffset = ActiveProfile ? ActiveProfile->FootpointOffset : FVector::ZeroVector;
	if (bDeathVisualActive && CurrentSprite && ActiveProfile && ActiveProfile->bUseAuthoredDeathPivot)
	{
		CalculatedFootAlignmentOffset = UReEcho2DAnimationComponent::CalculatePivotAlignmentOffset(
		    SequenceAnimation->GetRelativeTransform(),
		    FlipbookRoot->GetRelativeTransform(),
		    AuthoredMotionLocation,
		    ProfileFootpointOffset,
		    ActiveProfile ? ActiveProfile->DeathGroundSink : 0.0f);
		return;
	}
	CalculatedFootAlignmentOffset =
	    UReEcho2DAnimationComponent::CalculateFootAlignmentOffset(AlignmentBounds,
	                                                              SequenceAnimation->GetRelativeTransform(),
	                                                              FlipbookRoot->GetRelativeTransform(),
	                                                              AuthoredMotionLocation,
	                                                              ProfileFootpointOffset);
}

void UReEchoEnemyPresentationComponent::UpdateHitReaction(const FReEchoEnemyPresentationSnapshot& Snapshot)
{
	if (!VisualEffectRoot)
	{
		return;
	}
	const float EffectiveRatio =
	    Snapshot.HitReactionDurationSeconds > 0.0f
	        ? FMath::Clamp(Snapshot.HitReactionRemainingSeconds / Snapshot.HitReactionDurationSeconds, 0.0f, 1.0f)
	        : 0.0f;
	if (ShakeDirection.IsNearlyZero() && !Snapshot.KnockbackVelocity.IsNearlyZero())
	{
		ShakeDirection =
		    FVector::CrossProduct(FVector::UpVector, Snapshot.KnockbackVelocity.GetSafeNormal2D()).GetSafeNormal();
	}
	const float ElapsedTime = ReEchoEnemyVisual::HitReactionDuration * (1.0f - EffectiveRatio);
	const float ShakeDistance = FMath::Sin(ElapsedTime * 90.0f) * 8.0f * EffectiveRatio;
	ApplyPresentationMotion(ShakeDirection * ShakeDistance, FVector(1.12f, 0.86f, 1.0f));
	bHitVisualActive = true;
}

void UReEchoEnemyPresentationComponent::UpdateSpriteAnimation(const FReEchoEnemyPresentationSnapshot& Snapshot,
                                                              const float DeltaSeconds)
{
	if (!VisualEffectRoot)
	{
		return;
	}
	if (PresentationController)
	{
		PresentationController->SetMoving(Snapshot.bMoving);
	}
	AttackVisualRemaining = FMath::Max(0.0f, AttackVisualRemaining - DeltaSeconds);
	ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
}

void UReEchoEnemyPresentationComponent::HandleBossIntent(const FReEchoBossIntent& Intent)
{
	const bool bBlinkSlam = Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlam ||
	                        Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving;
	const bool bPhase3BlinkSlam = CurrentBossPhaseIndex >= 3 && bBlinkSlam;
	const EReEchoBossIntentType ShakeEvent = (Intent.bPhaseOpening || Intent.bGroundedSlam) && bBlinkSlam
	                                             ? EReEchoBossIntentType::ImpactResolved
	                                             : EReEchoBossIntentType::AttackWindowStarted;
	if (Intent.Type == ShakeEvent && Intent.Attack.IsValid() && Intent.Attack.Sequence != LastBossCameraShakeSequence &&
	    (bBlinkSlam || Intent.AbilityKind == EReEchoBossAbilityKind::PrayerBeam))
	{
		if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Host, 0))
		{
			if (AReEchoArenaCameraActor* ArenaCamera = Cast<AReEchoArenaCameraActor>(PlayerController->GetViewTarget()))
			{
				if (Intent.AbilityKind == EReEchoBossAbilityKind::PrayerBeam)
				{
					ArenaCamera->PlayImpactShake(20.0f, 0.20f);
				}
				else
				{
					ArenaCamera->PlayImpactShake(14.0f, 0.25f);
				}
				LastBossCameraShakeSequence = Intent.Attack.Sequence;
			}
		}
	}
	if (bPhase3BlinkSlam && (Intent.bPhaseOpening || Intent.bGroundedSlam))
	{
		// Opening is three grounded Attack clips, not the normal blink/rise/descent choreography.
		bBossPhase3PendingWindupWalk = false;
		BossPhase3LandingHoldRemaining = 0.0f;
		BossPhase3BlinkSlamWindupRemaining = 0.0f;
		BossBlinkSlamRemaining = 0.0f;
		bBossPhase3BlinkSlamHidden = false;
		bBossPhase3AttackScaleActive = Intent.Type != EReEchoBossIntentType::AbilityEnded;
		ApplyBossPhase3Facing(Intent.LockedDirection);
		if (SequenceAnimation)
		{
			SequenceAnimation->SetVisibility(true);
			SequenceAnimation->SetHiddenInGame(false);
		}
		if (GroundShadow)
		{
			GroundShadow->SetVisibility(true);
			GroundShadow->SetHiddenInGame(false);
		}
		if (PresentationController && Intent.Type == EReEchoBossIntentType::TelegraphStarted)
		{
			PresentationController->SetWeaponVisualSetId(TEXT("Phase3"));
			PresentationController->PlayAction(ReEcho2DAnimationTags::Attack_Basic, true, Intent.Attack.Sequence);
			FitBossPhase3AttackPlaybackToWindow(Intent.WindupSeconds + Intent.ActiveSeconds);
		}
		ApplyBossPhase3FixedRendererScale();
		ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
		return;
	}
	if (bPhase3BlinkSlam && Intent.Type == EReEchoBossIntentType::TelegraphStarted)
	{
		const bool bHoldPreviousLanding = bBossPhase3AttackScaleActive;
		bBossPhase3PendingWindupWalk = bHoldPreviousLanding;
		BossPhase3LandingHoldRemaining = bHoldPreviousLanding ? ReEchoEnemyVisual::Phase3LandingHoldDuration : 0.0f;
		if (!bHoldPreviousLanding)
		{
			bBossPhase3AttackScaleActive = false;
		}
		ApplyBossPhase3Facing(Intent.LockedDirection);
		// Combo windup may begin before the prior strike's 0.5 second descent presentation finishes.
		BossBlinkSlamRemaining = 0.0f;
		BossPhase3BlinkSlamWindupDuration = ReEchoEnemyVisual::Phase3BlinkSlamWindupDuration;
		BossPhase3BlinkSlamWindupRemaining = BossPhase3BlinkSlamWindupDuration;
		bBossPhase3BlinkSlamHidden = false;
		if (PresentationController)
		{
			PresentationController->SetWeaponVisualSetId(TEXT("Phase3"));
			// Charging/rising keeps the Phase3 Walk loop. In a combo this also retires the prior slam action before the
			// next ascent; GroundSlam is selected only when the attack window starts and the boss begins descending.
			if (!bHoldPreviousLanding)
			{
				PresentationController->CancelAttackAction();
			}
			ApplyBossPhase3FixedRendererScale();
			ApplyPresentationMotion(
			    ResolvePhase3BlinkSlamWindupOffset(BossPhase3BlinkSlamWindupRemaining,
			                                       BossPhase3BlinkSlamWindupDuration,
			                                       ReEchoEnemyVisual::Phase3BlinkSlamWindupHeightCm),
			    FVector::OneVector);
		}
	}
	if ((Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlam ||
	     Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving) &&
	    Intent.Type == EReEchoBossIntentType::AttackWindowStarted)
	{
		if (bPhase3BlinkSlam)
		{
			bBossPhase3PendingWindupWalk = false;
			BossPhase3LandingHoldRemaining = 0.0f;
			bBossPhase3AttackScaleActive = true;
			ApplyBossPhase3Facing(Intent.LockedDirection);
		}
		if (bPhase3BlinkSlam && PresentationController)
		{
			PresentationController->SetWeaponVisualSetId(TEXT("Phase3"));
		}
		BossPhase3BlinkSlamWindupRemaining = 0.0f;
		bBossPhase3BlinkSlamHidden = false;
		if (SequenceAnimation)
		{
			SequenceAnimation->SetVisibility(true);
			SequenceAnimation->SetHiddenInGame(false);
		}
		if (GroundShadow)
		{
			GroundShadow->SetVisibility(true);
			GroundShadow->SetHiddenInGame(false);
		}
		BossBlinkSlamDuration = ReEchoEnemyVisual::BlinkSlamDuration;
		BossBlinkSlamRemaining = BossBlinkSlamDuration;
		BossBlinkSlamStartHeightCm = ReEchoEnemyVisual::BlinkSlamStartHeightCm;
		ApplyBossPhase3FixedRendererScale();
		ApplyPresentationMotion(
		    ResolveBlinkSlamVisualOffset(BossBlinkSlamRemaining, BossBlinkSlamDuration, BossBlinkSlamStartHeightCm),
		    FVector::OneVector);
	}
	if (Intent.AbilityId == TEXT("M_SHEEP_MeleeSweep") && Intent.Type == EReEchoBossIntentType::AttackWindowStarted)
	{
		BossWeaponSwingDuration = FMath::Max(Intent.ActiveSeconds, 0.22f);
		BossWeaponSwingRemaining = BossWeaponSwingDuration;
	}
	else if (Intent.Type == EReEchoBossIntentType::AbilityEnded && BossWeaponRoot)
	{
		BossWeaponSwingRemaining = 0.0f;
		BossWeaponRoot->SetRelativeRotation(BossWeaponRestRotation);
	}
	if (Intent.Type == EReEchoBossIntentType::AbilityEnded &&
	    (Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlam ||
	     Intent.AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving))
	{
		bBossPhase3PendingWindupWalk = false;
		BossPhase3LandingHoldRemaining = 0.0f;
		bBossPhase3AttackScaleActive = false;
		BossPhase3BlinkSlamWindupRemaining = 0.0f;
		bBossPhase3BlinkSlamHidden = false;
		BossBlinkSlamRemaining = 0.0f;
		if (SequenceAnimation)
		{
			SequenceAnimation->SetVisibility(true);
			SequenceAnimation->SetHiddenInGame(false);
		}
		if (GroundShadow)
		{
			GroundShadow->SetVisibility(true);
			GroundShadow->SetHiddenInGame(false);
		}
		ApplyBossPhase3FixedRendererScale();
		ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
	}
	if (Intent.Type == EReEchoBossIntentType::AbilityEnded && bStunPaused)
	{
		bCancelAttackWhenStunClears = true;
	}
	if (Intent.Type != EReEchoBossIntentType::TelegraphStarted &&
	    Intent.Type != EReEchoBossIntentType::AttackWindowStarted)
	{
		return;
	}
	if (bPhase3BlinkSlam && Intent.Type == EReEchoBossIntentType::TelegraphStarted)
	{
		return;
	}
	AttackVisualRemaining = FMath::Max(AttackVisualRemaining, 0.22f);
	if (PresentationController)
	{
		const int64 Sequence = Intent.Attack.IsValid() ? Intent.Attack.Sequence : INDEX_NONE;
		PresentationController->PlayAction(Intent.Type == EReEchoBossIntentType::TelegraphStarted
		                                       ? ReEcho2DAnimationTags::Attack_Charge
		                                       : ReEcho2DAnimationTags::Attack_Basic,
		                                   true,
		                                   Sequence);
		if (bPhase3BlinkSlam)
		{
			FitBossPhase3AttackPlaybackToWindow(Intent.Type == EReEchoBossIntentType::TelegraphStarted
			                                        ? BossPhase3BlinkSlamWindupDuration
			                                        : BossBlinkSlamDuration);
			ApplyBossPhase3FixedRendererScale();
		}
	}
}

void UReEchoEnemyPresentationComponent::HandlePresentationAction(const FReEchoPresentationActionEvent& Event)
{
	if (!PresentationController)
	{
		return;
	}
	// Phase3 owns only Boss Skill03. BossIntent already selects and scales its authored attack; replaying the
	// generic committed action afterwards resets the renderer to the profile-normalized (smaller) scale.
	if (CurrentBossPhaseIndex >= 3)
	{
		return;
	}
	if (Event.Phase == EReEchoPresentationActionPhase::Ended ||
	    Event.Phase == EReEchoPresentationActionPhase::Cancelled)
	{
		if (bStunPaused)
		{
			bCancelAttackWhenStunClears = true;
		}
		else
		{
			PresentationController->CancelAttackAction();
		}
		return;
	}
	PresentationController->PlayAction(Event.Phase == EReEchoPresentationActionPhase::Windup
	                                       ? ReEcho2DAnimationTags::Attack_Charge
	                                       : ReEcho2DAnimationTags::Attack_Basic,
	                                   true,
	                                   Event.Key.Sequence);
}

#if WITH_DEV_AUTOMATION_TESTS
void UReEchoEnemyPresentationComponent::CompleteActiveAnimationForTests()
{
	if (SequenceAnimation)
	{
		SequenceAnimation->Stop();
	}
	if (PresentationController)
	{
		PresentationController->UpdatePlaybackCompletion();
	}
}

void UReEchoEnemyPresentationComponent::ConsumePresentationActionForTests(const FReEchoPresentationActionEvent& Event)
{
	HandlePresentationAction(Event);
}
#endif

void UReEchoEnemyPresentationComponent::HandlePhaseTransition(const FReEchoEnemyPhaseTransitionEvent& Event)
{
	CurrentBossPhaseIndex = Event.AnimationSetId == TEXT("Phase3")   ? 3
	                        : Event.AnimationSetId == TEXT("Phase2") ? 2
	                                                                 : CurrentBossPhaseIndex;
	if (Event.AnimationSetId == TEXT("Phase3") && BossWeaponSprite)
	{
		bBossPhase3AttackScaleActive = false;
		BossWeaponSprite->SetVisibility(false);
		BossWeaponSprite->SetHiddenInGame(true);
	}
	if (!PresentationController)
	{
		return;
	}
	if (Event.bStarted)
	{
		PresentationController->BeginAnimationSetTransition(Event.AnimationSetId,
		                                                    ReEcho2DAnimationTags::Transform_Phase2);
	}
	else
	{
		const bool bEnteringPhase3 = Event.AnimationSetId == TEXT("Phase3");
		if (SequenceAnimation && !bEnteringPhase3)
		{
			SequenceAnimation->SetDisplayScaleMultiplier(1.0f);
			SequenceAnimation->SetDisplayScaleReferenceHeight(0.0f);
		}
		PresentationController->CompleteAnimationSetTransition(Event.AnimationSetId);
		if (bEnteringPhase3 && SequenceAnimation)
		{
			const UPaperFlipbook* Phase3Walk = SequenceAnimation->GetFlipbook();
			const float Phase3ReferenceHeight = Phase3Walk ? Phase3Walk->GetRenderBounds().BoxExtent.Z * 2.0f : 0.0f;
			SequenceAnimation->SetDisplayScaleReferenceHeight(Phase3ReferenceHeight);
			SequenceAnimation->SetDisplayScaleMultiplier(ReEchoEnemyVisual::Phase3AnimationScaleMultiplier);
			// Recalculate foot and shadow anchors from the enlarged Phase3 Walk bounds immediately on transition.
			ApplyPresentationMotion(FVector::ZeroVector, FVector::OneVector);
		}
	}
}

void UReEchoEnemyPresentationComponent::HandleFuseChanged(const FReEchoEnemyFuseEvent& Event)
{
	LastFuseRemaining = Event.RemainingSeconds;
	LastFuseDuration = Event.DurationSeconds;
}

void UReEchoEnemyPresentationComponent::HandleCombatHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != Host || Event.AppliedDamage <= 0.0f)
	{
		return;
	}
	const FLinearColor Color = ReEchoElementReaction::GetDamageNumberColor(Event);
	AReEchoDamageNumberActor::SpawnDamageNumber(Host ? Host->GetWorld() : nullptr,
	                                            Event.WorldLocation,
	                                            ReEchoElementReaction::GetDamageNumberValue(Event),
	                                            Color,
	                                            Event.bCritical);
	if (Event.bFatal || bDeathVisualActive)
	{
		return;
	}
	FVector KnockbackDirection = (Event.WorldLocation - Event.SourceWorldLocation).GetSafeNormal2D();
	if (KnockbackDirection.IsNearlyZero() && Host)
	{
		const AReEchoEnemyActor* EnemyHost = Cast<AReEchoEnemyActor>(Host);
		KnockbackDirection = EnemyHost ? -EnemyHost->GetFacingDirection() : FVector::BackwardVector;
	}
	ShakeDirection = FVector::CrossProduct(FVector::UpVector, KnockbackDirection).GetSafeNormal();
	bHitVisualActive = true;
	// Phase3 only authors Walk and GroundSlam. Resolving its missing Hit semantic would fall back to the
	// default (Phase1) animation set and interrupt the multi-slam presentation.
	if (PresentationController && CurrentBossPhaseIndex < 3)
	{
		PresentationController->PlayAction(ReEcho2DAnimationTags::Hit, true);
	}
}

void UReEchoEnemyPresentationComponent::HandleElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event)
{
	if (!AReEchoElementReactionPopupActor::ShouldDisplayForTarget(Event, Host))
	{
		return;
	}
	AReEchoElementReactionPopupActor::SpawnReactionPopup(
	    Host ? Host->GetWorld() : nullptr, Event.PrimaryTarget->GetActorLocation(), Event.ReactionBehaviorId);
}
