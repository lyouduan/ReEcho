#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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
#include "UI/ReEchoDamageNumberActor.h"

namespace ReEchoEnemyVisual
{
constexpr float HitReactionDuration = 0.22f;
}

UReEchoEnemyPresentationComponent::UReEchoEnemyPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoEnemyPresentationComponent::SetPresentationCatalog(UReEcho2DPresentationCatalog* InPresentationCatalog)
{
	PresentationCatalog = InPresentationCatalog;
}

void UReEchoEnemyPresentationComponent::ConfigureComponents(USceneComponent* InPresentationRoot,
                                                            USceneComponent* InVisualEffectRoot,
                                                            USceneComponent* InFootRoot,
                                                            USceneComponent* InFlipbookRoot,
                                                            USceneComponent* InEffectsRoot,
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
	FlipbookRoot = InFlipbookRoot;
	EffectsRoot = InEffectsRoot;
	CharacterSprite = InCharacterSprite;
	SequenceAnimation = InSequenceAnimation;
	PresentationController = InPresentationController;
	FrameCollisionDriver = InFrameCollisionDriver;
	GroundShadow = InGroundShadow;
	GroundRoot = GroundShadow ? GroundShadow->GetAttachParent() : nullptr;
	Collision = InCollision;
	AuthoredMotionLocation = VisualEffectRoot ? VisualEffectRoot->GetRelativeLocation() : FVector::ZeroVector;
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
		PresentationController->Configure(CharacterSprite, SequenceAnimation, Profile);
	}
	if (VisualEffectRoot)
	{
		VisualEffectRoot->SetRelativeLocation(AuthoredMotionLocation);
		VisualEffectRoot->SetRelativeScale3D(FVector::OneVector);
		BaseVisualScale = VisualEffectRoot->GetRelativeScale3D();
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

bool UReEchoEnemyPresentationComponent::BeginTerminalDeath(FSimpleDelegate OnCompleted,
                                                           float& OutExpectedDurationSeconds)
{
	OutExpectedDurationSeconds = 0.0f;
	if (bDeathVisualActive)
	{
		return false;
	}
	bDeathVisualActive = true;
	bHitVisualActive = false;
	AttackVisualRemaining = 0.0f;
	ResetTransientRoot();
	if (EffectsRoot)
	{
		EffectsRoot->SetVisibility(false, true);
		EffectsRoot->SetHiddenInGame(true, true);
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

void UReEchoEnemyPresentationComponent::Advance(const FReEchoEnemyPresentationSnapshot& Snapshot,
                                                const float DeltaSeconds)
{
	const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
	VisualTime += SafeDelta;
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
		RefreshFootpointAlignment();
		if (VisualEffectRoot)
		{
			VisualEffectRoot->SetRelativeLocation(AuthoredMotionLocation + CalculatedFootAlignmentOffset);
		}
		RefreshGroundShadowFromFlipbook();
		return;
	}
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
	if (PresentationController)
	{
		const FVector CameraRight = FRotationMatrix(Camera->GetCameraRotation()).GetUnitAxis(EAxis::Y);
		const float ScreenHorizontalDirection = FVector::DotProduct(Snapshot.FacingDirection, CameraRight);
		PresentationController->SetFacingSign(ScreenHorizontalDirection < 0.0f ? -1.0f : 1.0f);
	}
}

void UReEchoEnemyPresentationComponent::ResetTransientRoot()
{
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
	if (bDeathVisualActive)
	{
		CurrentSprite = Flipbook->GetSpriteAtTime(SequenceAnimation->GetPlaybackPosition(), true);
		if (CurrentSprite)
		{
			FlipbookBounds = CurrentSprite->GetRenderBounds();
		}
	}
	FVector2D AuthoredDeathPivot;
	const bool bUseAuthoredDeathPivot =
	    CurrentSprite && CurrentSprite->GetPivotMode(AuthoredDeathPivot) == ESpritePivotMode::Custom;
	const FVector LocalGroundAnchor = bUseAuthoredDeathPivot
	                                      ? FVector::ZeroVector
	                                      : FVector(FlipbookBounds.Origin.X,
	                                                FlipbookBounds.Origin.Y,
	                                                FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomWorld = SequenceAnimation->GetComponentTransform().TransformPosition(LocalGroundAnchor);
	const FVector BottomInFootRoot = FootRoot->GetComponentTransform().InverseTransformPosition(BottomWorld);
	GroundRoot->SetRelativeLocation(FVector(BottomInFootRoot.X, BottomInFootRoot.Y, AuthoredGroundRootLocation.Z));

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
	if (bDeathVisualActive)
	{
		CurrentSprite = Flipbook->GetSpriteAtTime(SequenceAnimation->GetPlaybackPosition(), true);
		if (CurrentSprite)
		{
			AlignmentBounds = CurrentSprite->GetRenderBounds();
		}
	}
	const FVector ProfileFootpointOffset = ActiveProfile ? ActiveProfile->FootpointOffset : FVector::ZeroVector;
	FVector2D AuthoredDeathPivot;
	if (CurrentSprite && CurrentSprite->GetPivotMode(AuthoredDeathPivot) == ESpritePivotMode::Custom)
	{
		CalculatedFootAlignmentOffset =
		    UReEcho2DAnimationComponent::CalculatePivotAlignmentOffset(SequenceAnimation->GetRelativeTransform(),
		                                                               FlipbookRoot->GetRelativeTransform(),
		                                                               AuthoredMotionLocation,
		                                                               ProfileFootpointOffset);
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
	if (Intent.Type != EReEchoBossIntentType::TelegraphStarted &&
	    Intent.Type != EReEchoBossIntentType::AttackWindowStarted)
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
	}
}

void UReEchoEnemyPresentationComponent::HandlePresentationAction(const FReEchoPresentationActionEvent& Event)
{
	if (!PresentationController)
	{
		return;
	}
	if (Event.Phase == EReEchoPresentationActionPhase::Ended ||
	    Event.Phase == EReEchoPresentationActionPhase::Cancelled)
	{
		PresentationController->CancelAttackAction();
		return;
	}
	PresentationController->PlayAction(Event.Phase == EReEchoPresentationActionPhase::Windup
	                                       ? ReEcho2DAnimationTags::Attack_Charge
	                                       : ReEcho2DAnimationTags::Attack_Basic,
	                                   true,
	                                   Event.Key.Sequence);
}

void UReEchoEnemyPresentationComponent::HandlePhaseTransition(const FReEchoEnemyPhaseTransitionEvent& Event)
{
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
		PresentationController->CompleteAnimationSetTransition(Event.AnimationSetId);
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
	const FLinearColor Color = Event.Element == EReEchoElement::None
	                               ? FLinearColor::White
	                               : ReEchoElementReaction::GetElementColor(Event.Element);
	AReEchoDamageNumberActor::SpawnDamageNumber(
	    Host ? Host->GetWorld() : nullptr, Event.WorldLocation, Event.AppliedDamage, Color);
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
	if (PresentationController)
	{
		PresentationController->PlayAction(ReEcho2DAnimationTags::Hit, true);
	}
}
