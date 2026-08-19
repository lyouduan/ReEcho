#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Graybox/ReEchoBillboardDebug.h"
#include "Graybox/ReEchoCollisionDebug.h"
#include "Graybox/ReEchoHealthBarActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "UI/ReEchoDamageNumberActor.h"

namespace ReEchoEnemyVisual
{
constexpr float DefaultWorldHeight = 100.0f;
constexpr float HealthBarHeightRatio = 0.65f;
constexpr float HealthBarWidthScale = 0.72f;
constexpr float HitReactionDuration = 0.22f;
}

UReEchoEnemyPresentationComponent::UReEchoEnemyPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FVector UReEchoEnemyPresentationComponent::CalculateFootAlignmentOffset(const FBoxSphereBounds& FlipbookBounds,
                                                                        const FTransform& RendererToFlipbookRoot,
                                                                        const FTransform& FlipbookRootToMotionRoot,
                                                                        const FVector& AuthoredOffset)
{
	const FVector LocalBottomCenter(
	    FlipbookBounds.Origin.X, FlipbookBounds.Origin.Y, FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomInFlipbookRoot = RendererToFlipbookRoot.TransformPosition(LocalBottomCenter);
	const FVector BottomInMotionRoot = FlipbookRootToMotionRoot.TransformPosition(BottomInFlipbookRoot);
	return AuthoredOffset - BottomInMotionRoot;
}

float UReEchoEnemyPresentationComponent::CalculateFlipbookPresentationWidth(
    const FBoxSphereBounds& FlipbookBounds,
    const FTransform& RendererToFlipbookRoot,
    const FTransform& FlipbookRootToFootRoot)
{
	const FVector LocalLeft(FlipbookBounds.Origin.X - FlipbookBounds.BoxExtent.X,
	                        FlipbookBounds.Origin.Y,
	                        FlipbookBounds.Origin.Z);
	const FVector LocalRight(FlipbookBounds.Origin.X + FlipbookBounds.BoxExtent.X,
	                         FlipbookBounds.Origin.Y,
	                         FlipbookBounds.Origin.Z);
	const FVector LeftInFootRoot =
	    FlipbookRootToFootRoot.TransformPosition(RendererToFlipbookRoot.TransformPosition(LocalLeft));
	const FVector RightInFootRoot =
	    FlipbookRootToFootRoot.TransformPosition(RendererToFlipbookRoot.TransformPosition(LocalRight));
	return FVector::Distance(LeftInFootRoot, RightInFootRoot);
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
                                                            UTextRenderComponent* InElementAuraRing,
                                                            UTextRenderComponent* InElementAttachmentLabel,
                                                            UPointLightComponent* InElementAuraLight,
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
	ElementAuraRing = InElementAuraRing;
	ElementAttachmentLabel = InElementAttachmentLabel;
	ElementAuraLight = InElementAuraLight;
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
		GroundShadow->SetMaterial(
		    0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ReEcho/Materials/M_GroundShadow.M_GroundShadow")));
	}
}

void UReEchoEnemyPresentationComponent::BindEventSources(AActor* InHost,
                                                         UReEchoCombatantComponent* InCombatant,
                                                         UReEchoEnemyEventsComponent* InEnemyEvents,
                                                         UReEchoCombatEventsComponent* InCombatEvents)
{
	if (EnemyEvents)
	{
		EnemyEvents->OnActionCommitted.RemoveAll(this);
		EnemyEvents->OnBossIntent.RemoveAll(this);
		EnemyEvents->OnFuseChanged.RemoveAll(this);
	}
	if (CombatEvents)
	{
		CombatEvents->OnElementStateChanged.RemoveAll(this);
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnDeath.RemoveAll(this);
	}

	Host = InHost;
	Combatant = InCombatant;
	EnemyEvents = InEnemyEvents;
	CombatEvents = InCombatEvents;
	if (EnemyEvents)
	{
		EnemyEvents->OnActionCommitted.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleActionCommitted);
		EnemyEvents->OnBossIntent.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleBossIntent);
		EnemyEvents->OnFuseChanged.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleFuseChanged);
	}
	if (CombatEvents)
	{
		CombatEvents->OnElementStateChanged.AddDynamic(this,
		                                               &UReEchoEnemyPresentationComponent::HandleElementStateChanged);
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleCombatHurt);
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoEnemyPresentationComponent::HandleCombatDeath);
	}
}

void UReEchoEnemyPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindEventSources(nullptr, nullptr, nullptr, nullptr);
	Super::EndPlay(EndPlayReason);
}

void UReEchoEnemyPresentationComponent::ConfigureAppearance(const FName PresentationId)
{
	ApplyVisual(PresentationId);
	RefreshElementAttachmentVisual();
	if (!HealthBar && Host && Host->GetWorld())
	{
		HealthBar = Host->GetWorld()->SpawnActor<AReEchoHealthBarActor>();
	}
	if (HealthBar)
	{
		const UReEcho2DCharacterPresentationProfile* Profile =
		    PresentationCatalog ? PresentationCatalog->ResolveProfile(PresentationId) : nullptr;
		const float PresentationHeight = Profile ? Profile->WorldHeight : ReEchoEnemyVisual::DefaultWorldHeight;
		HealthBar->Initialize(Combatant,
		                      FLinearColor(1.0f, 0.08f, 0.04f),
		                      PresentationHeight * ReEchoEnemyVisual::HealthBarHeightRatio,
		                      ReEchoEnemyVisual::HealthBarWidthScale,
		                      FootRoot ? FootRoot.Get() : PresentationRoot.Get());
	}
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
	DeathVisualRemaining = 0.0f;
	bHitVisualActive = false;
	bDeathVisualActive = false;
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
	UpdateElementAttachmentFacing();
	if (Snapshot.Phase == EReEchoEnemyBehaviorPhase::Dead || bDeathVisualActive)
	{
		UpdateDeathAnimation(SafeDelta);
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

	const FBoxSphereBounds FlipbookBounds = Flipbook->GetRenderBounds();
	const FVector LocalBottomCenter(FlipbookBounds.Origin.X,
	                                FlipbookBounds.Origin.Y,
	                                FlipbookBounds.Origin.Z - FlipbookBounds.BoxExtent.Z);
	const FVector BottomWorld = SequenceAnimation->GetComponentTransform().TransformPosition(LocalBottomCenter);
	const FVector BottomInFootRoot = FootRoot->GetComponentTransform().InverseTransformPosition(BottomWorld);
	GroundRoot->SetRelativeLocation(
	    FVector(BottomInFootRoot.X, BottomInFootRoot.Y, AuthoredGroundRootLocation.Z));

	const float FlipbookWidth = CalculateFlipbookPresentationWidth(
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
	CalculatedFootAlignmentOffset = ActiveProfile ? ActiveProfile->FootpointOffset : FVector::ZeroVector;
	const UPaperFlipbook* Flipbook = SequenceAnimation ? SequenceAnimation->GetFlipbook() : nullptr;
	if (!FlipbookRoot || !SequenceAnimation || !Flipbook || (ActiveProfile && !ActiveProfile->bAutoAlignFootpoint))
	{
		return;
	}
	CalculatedFootAlignmentOffset = CalculateFootAlignmentOffset(Flipbook->GetRenderBounds(),
	                                                             SequenceAnimation->GetRelativeTransform(),
	                                                             FlipbookRoot->GetRelativeTransform(),
	                                                             CalculatedFootAlignmentOffset);
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

void UReEchoEnemyPresentationComponent::UpdateDeathAnimation(const float DeltaSeconds)
{
	if (!VisualEffectRoot)
	{
		return;
	}
	DeathVisualRemaining = FMath::Max(0.0f, DeathVisualRemaining - DeltaSeconds);
	const float Ratio = DeathVisualRemaining / 0.45f;
	ApplyPresentationMotion(FVector(0.0f, 0.0f, -28.0f * (1.0f - Ratio)), FVector(Ratio, Ratio, 1.0f));
}

void UReEchoEnemyPresentationComponent::RefreshElementAttachmentVisual()
{
	if (!Combatant || !ElementAuraRing || !ElementAttachmentLabel || !ElementAuraLight)
	{
		return;
	}
	const EReEchoElement AttachedElement = Combatant->GetElementState().Attached;
	const bool bHasAttachment = ReEchoElementReaction::IsCombatElement(AttachedElement);
	ElementAuraRing->SetVisibility(bHasAttachment);
	ElementAttachmentLabel->SetVisibility(bHasAttachment);
	ElementAuraLight->SetVisibility(bHasAttachment);
	if (!bHasAttachment)
	{
		return;
	}
	const FLinearColor ElementColor = ReEchoElementReaction::GetElementColor(AttachedElement);
	ElementAuraRing->SetTextRenderColor(ElementColor.ToFColor(false));
	ElementAttachmentLabel->SetText(FText::FromString(ReEchoElementReaction::GetElementLabel(AttachedElement)));
	ElementAttachmentLabel->SetTextRenderColor(ElementColor.ToFColor(false));
	ElementAuraLight->SetLightColor(ElementColor);
}

void UReEchoEnemyPresentationComponent::UpdateElementAttachmentFacing()
{
	if (!ElementAuraRing || !ElementAuraRing->IsVisible())
	{
		return;
	}
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FRotator CameraFacingRotation = (-Camera->GetCameraRotation().Vector()).Rotation();
		ElementAuraRing->SetWorldRotation(CameraFacingRotation);
		ElementAttachmentLabel->SetWorldRotation(CameraFacingRotation);
	}
	const float Pulse = 1.0f + 0.055f * FMath::Sin(VisualTime * 3.2f);
	ElementAuraRing->SetRelativeScale3D(FVector(Pulse, Pulse, 1.0f));
	ElementAuraLight->SetIntensity(850.0f + 180.0f * FMath::Sin(VisualTime * 2.6f));
}

void UReEchoEnemyPresentationComponent::HandleActionCommitted(const FReEchoEnemyActionCommittedEvent& Event)
{
	AttackVisualRemaining = 0.22f;
	if (PresentationController)
	{
		PresentationController->PlayAction(ReEcho2DAnimationTags::Attack_Basic, true, Event.Attack.Sequence);
	}
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
		PresentationController->PlayAction(ReEcho2DAnimationTags::Attack_Basic, true, Sequence);
	}
}

void UReEchoEnemyPresentationComponent::HandleFuseChanged(const FReEchoEnemyFuseEvent& Event)
{
	LastFuseRemaining = Event.RemainingSeconds;
	LastFuseDuration = Event.DurationSeconds;
}

void UReEchoEnemyPresentationComponent::HandleElementStateChanged(const FReEchoElementStateChangedEvent& Event)
{
	if (Event.Combatant == Combatant)
	{
		RefreshElementAttachmentVisual();
	}
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

void UReEchoEnemyPresentationComponent::HandleCombatDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target != Host)
	{
		return;
	}
	RefreshElementAttachmentVisual();
	DeathVisualRemaining = 0.45f;
	bDeathVisualActive = true;
	if (PresentationController)
	{
		PresentationController->PlayAction(ReEcho2DAnimationTags::Death, true);
	}
}
