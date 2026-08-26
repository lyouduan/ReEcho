#include "Graybox/ReEchoEnemyActor.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Core/ReEchoRabbitProjectilePattern.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyCrowdSteering.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "ReEcho.h"
#include "ReEchoGameMode.h"
#include "Presentation/Scene/ReEcho2DSceneLightingComponent.h"
#include "ReEchoAudioEvents.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "Weapons/ReEchoWeaponGeometry.h"

namespace ReEchoEnemyHost
{
EReEchoEnemyArchetype ToArchetype(const EReEchoEnemyKind Kind)
{
	switch (Kind)
	{
		case EReEchoEnemyKind::Shield:
			return EReEchoEnemyArchetype::Shield;
		case EReEchoEnemyKind::Bomber:
			return EReEchoEnemyArchetype::Bomber;
		case EReEchoEnemyKind::Boss:
			return EReEchoEnemyArchetype::Boss;
		case EReEchoEnemyKind::Slime:
			return EReEchoEnemyArchetype::Slime;
		case EReEchoEnemyKind::Ranged:
			return EReEchoEnemyArchetype::Ranged;
		case EReEchoEnemyKind::Elite:
			return EReEchoEnemyArchetype::Elite;
		default:
			return EReEchoEnemyArchetype::Grunt;
	}
}

EReEchoEnemyKind ToLegacyKind(const EReEchoEnemyArchetype Archetype)
{
	switch (Archetype)
	{
		case EReEchoEnemyArchetype::Shield:
			return EReEchoEnemyKind::Shield;
		case EReEchoEnemyArchetype::Bomber:
			return EReEchoEnemyKind::Bomber;
		case EReEchoEnemyArchetype::Boss:
			return EReEchoEnemyKind::Boss;
		case EReEchoEnemyArchetype::Slime:
			return EReEchoEnemyKind::Slime;
		case EReEchoEnemyArchetype::Ranged:
			return EReEchoEnemyKind::Ranged;
		case EReEchoEnemyArchetype::Elite:
			return EReEchoEnemyKind::Elite;
		default:
			return EReEchoEnemyKind::Grunt;
	}
}
}

AReEchoEnemyActor::AReEchoEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	CombatAttributes = CreateDefaultSubobject<UReEchoCombatAttributeSet>(TEXT("CombatAttributes"));

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitBoxExtent(FVector(22.0f, 22.0f, 40.0f));
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetVisibility(false);

	PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(RootComponent);
	FootRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FootRoot"));
	FootRoot->SetupAttachment(PresentationRoot);
	FootRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	PresentationMotionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationMotionRoot"));
	PresentationMotionRoot->SetupAttachment(FootRoot);
	FlipbookRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlipbookRoot"));
	FlipbookRoot->SetupAttachment(PresentationMotionRoot);
	FlipbookRoot->SetRelativeRotation(
	    UReEcho2DAnimationComponent::CalculateCameraFacingRotation(FRotator(-45.0f, 0.0f, 0.0f)));
	GroundRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GroundRoot"));
	GroundRoot->SetupAttachment(FootRoot);
	EffectsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EffectsRoot"));
	EffectsRoot->SetupAttachment(PresentationMotionRoot);
	AttackVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AttackVfxRoot"));
	AttackVfxRoot->SetupAttachment(EffectsRoot);
	AttackVfxRoot->bEditableWhenInherited = true;
	HurtVfxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HurtVfxRoot"));
	HurtVfxRoot->SetupAttachment(EffectsRoot);
	HurtVfxRoot->bEditableWhenInherited = true;
	BossWeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BossWeaponRoot"));
	BossWeaponRoot->SetupAttachment(EffectsRoot);
	BossWeaponRoot->bEditableWhenInherited = true;
	BossWeaponSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("BossWeaponSprite"));
	BossWeaponSprite->SetupAttachment(BossWeaponRoot);
	BossWeaponSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BossWeaponSprite->SetCastShadow(false);
	BossWeaponSprite->SetHiddenInGame(true);
	BossWeaponSprite->SetVisibility(false);
	BossWeaponSprite->SetTranslucentSortPriority(7);
	BossWeaponSprite->bIsScreenSizeScaled = false;

	GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(GroundRoot);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-10);
	GroundShadow->SetRelativeLocation(FVector::ZeroVector);
	GroundShadow->SetRelativeScale3D(FVector(0.44f, 0.46f, 1.0f));
	GroundShadow->bEditableWhenInherited = true;

	UBillboardComponent* CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(FlipbookRoot);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(false);
	CharacterSprite->SetAbsolute(false, false, true);
	CharacterSprite->bIsScreenSizeScaled = false;
	SequenceAnimation = CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("FlipbookRenderer"));
	SequenceAnimation->SetupAttachment(FlipbookRoot);
	PresentationController = CreateDefaultSubobject<UReEcho2DPresentationController>(TEXT("PresentationController"));
	FrameCollisionDriver = CreateDefaultSubobject<UReEcho2DFrameCollisionDriver>(TEXT("FrameCollisionDriver"));
	SceneLighting = CreateDefaultSubobject<UReEcho2DSceneLightingComponent>(TEXT("SceneLighting"));
	SceneLighting->Configure(SequenceAnimation, GroundShadow);

	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	CombatVfx = CreateDefaultSubobject<UReEchoCombatVfxComponent>(TEXT("CombatVfx"));
	CombatVfx->ConfigureAttachmentRoots(AttackVfxRoot, HurtVfxRoot, BossWeaponRoot);
	EnemyLogic = CreateDefaultSubobject<UReEchoEnemyLogicComponent>(TEXT("EnemyLogic"));
	EnemyEvents = CreateDefaultSubobject<UReEchoEnemyEventsComponent>(TEXT("EnemyEvents"));
	CombatPresentationCoordinator =
	    CreateDefaultSubobject<UReEchoCombatPresentationCoordinator>(TEXT("CombatPresentationCoordinator"));
	EnemyPresentation = CreateDefaultSubobject<UReEchoEnemyPresentationComponent>(TEXT("EnemyPresentation"));
	EnemyPresentation->ConfigureComponents(PresentationRoot,
	                                       PresentationMotionRoot,
	                                       FootRoot,
	                                       FlipbookRoot,
	                                       EffectsRoot,
	                                       BossWeaponRoot,
	                                       BossWeaponSprite,
	                                       CharacterSprite,
	                                       SequenceAnimation,
	                                       PresentationController,
	                                       FrameCollisionDriver,
	                                       GroundShadow,
	                                       Collision);

	Tags.Add(TEXT("ReEchoEnemy"));
}

void AReEchoEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetActorScale3D(FVector(FMath::Max(CharacterScale, 0.01f)));
	RefreshPresentationHierarchy();
	RefreshFootRoot();
}

void AReEchoEnemyActor::RefreshPresentationHierarchy()
{
	const FAttachmentTransformRules KeepRelative = FAttachmentTransformRules::KeepRelativeTransform;
	auto AttachIfNeeded = [&KeepRelative](USceneComponent* Child, USceneComponent* ExpectedParent)
	{
		if (Child && ExpectedParent && Child->GetAttachParent() != ExpectedParent)
		{
			Child->AttachToComponent(ExpectedParent, KeepRelative);
		}
	};

	AttachIfNeeded(PresentationRoot, RootComponent);
	AttachIfNeeded(FootRoot, PresentationRoot);
	AttachIfNeeded(PresentationMotionRoot, FootRoot);
	AttachIfNeeded(FlipbookRoot, PresentationMotionRoot);
	AttachIfNeeded(GroundRoot, FootRoot);
	AttachIfNeeded(EffectsRoot, PresentationMotionRoot);
	AttachIfNeeded(AttackVfxRoot, EffectsRoot);
	AttachIfNeeded(HurtVfxRoot, EffectsRoot);
	AttachIfNeeded(BossWeaponRoot, EffectsRoot);
	AttachIfNeeded(BossWeaponSprite, BossWeaponRoot);
	AttachIfNeeded(GroundShadow, GroundRoot);
	AttachIfNeeded(SequenceAnimation, FlipbookRoot);
}

void AReEchoEnemyActor::RefreshFootRoot()
{
	if (FootRoot && Collision)
	{
		FootRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -Collision->GetUnscaledBoxExtent().Z));
		FootRoot->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AReEchoEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshPresentationHierarchy();
	RefreshFootRoot();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
	BindComposedComponents();
}

void AReEchoEnemyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EnemyRoster)
	{
		ClearCrowdCollisionIgnores();
		EnemyRoster->UnregisterEnemy(this);
	}
	if (CombatEvents)
	{
		CombatEvents->OnDeath.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AReEchoEnemyActor::BindComposedComponents()
{
	EnemyLogic->BindEventSources(EnemyEvents, CombatEvents);
	EnemyPresentation->BindEventSources(this, EnemyEvents, CombatEvents);
	CombatEvents->OnDeath.RemoveAll(this);
	CombatEvents->OnDeath.AddDynamic(this, &AReEchoEnemyActor::HandleCombatDeath);
}

UAbilitySystemComponent* AReEchoEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AReEchoEnemyActor::Configure(const EReEchoEnemyKind InKind, const int32 SpawnIndex)
{
	BindComposedComponents();
	if (EnemyRoster)
	{
		ClearCrowdCollisionIgnores();
		EnemyRoster->UnregisterEnemy(this);
	}

	const UReEchoBalanceSettings* Settings = GetDefault<UReEchoBalanceSettings>();
	const FReEchoEnemyDefinition Definition =
	    ReEchoEnemyDefinitions::MakeLegacyEquivalent(ReEchoEnemyHost::ToArchetype(InKind),
	                                                 Settings->BomberTriggerRadius,
	                                                 Settings->BomberDamageRadius,
	                                                 Settings->BomberFuseDuration,
	                                                 Settings->BomberDamage);
	check(ConfigureFromDefinition(Definition, SpawnIndex));
}

void AReEchoEnemyActor::SetPresentationCatalog(UReEcho2DPresentationCatalog* InPresentationCatalog)
{
	EnemyPresentation->SetPresentationCatalog(InPresentationCatalog);
}

bool AReEchoEnemyActor::ConfigureFromDefinition(const FReEchoEnemyDefinition& Definition, const int32 SpawnIndex)
{
	BindComposedComponents();
	bDeathSequenceStarted = false;
	SetLifeSpan(0.0f);
	if (EnemyRoster)
	{
		ClearCrowdCollisionIgnores();
		EnemyRoster->UnregisterEnemy(this);
	}
	if (!EnemyLogic->Initialize(Definition, SpawnIndex))
	{
		return false;
	}
	Combatant->ResetElementState();
	FReEchoStatBlock Stats;
	Stats.HpMax = Definition.MaxHealth;
	Combatant->InitializeFromStats(Stats, true);
	// ConfigureFromDefinition is the Encounter Commit boundary for a newly spawned Host. Blueprint defaults or a
	// previous preview state must not leave a visible enemy untargetable after its warning has ended.
	SetCanBeDamaged(true);
	SetActorEnableCollision(true);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetBoxExtent(
	    FVector(Definition.CollisionRadiusCm, Definition.CollisionRadiusCm, Definition.CollisionHalfHeightCm));
	Collision->ClearMoveIgnoreActors();
	AlignToGameplayPlane();
	bVisualPlacementApplied = true;
	EnemyPresentation->ConfigureAppearance(Definition.PresentationId);
	const bool bBoss = Definition.Archetype == EReEchoEnemyArchetype::Boss;
	CombatAudioAdapter->ConfigureRouting(bBoss ? EReEchoCombatAudioSource::Boss : EReEchoCombatAudioSource::Enemy,
	                                     bBoss ? FReEchoAudioEvents::BossAttack : FReEchoAudioEvents::EnemyAttack,
	                                     bBoss ? FReEchoAudioEvents::BossDeath : FReEchoAudioEvents::EnemyDeath);
	if (!bAudioSpawnPosted)
	{
		CombatAudioAdapter->PostConfiguredEvent(bBoss ? FReEchoAudioEvents::BossSpawn : FReEchoAudioEvents::EnemySpawn,
		                                        GetActorLocation());
		bAudioSpawnPosted = true;
	}
	if (EnemyRoster)
	{
		check(EnemyRoster->RegisterEnemy(this, EnemyLogic));
		RefreshCrowdCollisionIgnores();
	}
	if (!IsAlive() || !CanBeDamaged() || !GetActorEnableCollision() || !Collision->IsCollisionEnabled())
	{
		return false;
	}
	// WS4 (Plan 68): arm the blood-depleted second-phase transition. When a lethal hit lands and this boss is
	// configured for a HealthThreshold phase change it has not yet used, convert the kill into a phase transition
	// instead of death.
	Combatant->SetFatalDamageInterceptDelegate(FReEchoFatalDamageIntercept::CreateLambda(
	    [this](float& InOutHealth) -> bool
	    {
		    if (!EnemyLogic || !EnemyLogic->GetDefinition().Phase2.bEnabled)
		    {
			    return false;
		    }
		    if (EnemyLogic->GetDefinition().Phase2.TriggerMode != EReEchoEnemyPhase2TriggerMode::HealthThreshold)
		    {
			    return false;
		    }
		    // Arm only while still in phase one and alive; once transformed, normal death rules apply.
		    const FReEchoEnemyLogicSnapshot& Snapshot = EnemyLogic->GetSnapshot();
		    if (Snapshot.bPhase2Triggered || Snapshot.CurrentPhaseIndex >= 2 || !Snapshot.bAlive)
		    {
			    return false;
		    }
		    FReEchoEnemyActionIntent PhaseIntent;
		    if (!EnemyLogic->TryTriggerPhase2OnFatalWound(PhaseIntent))
		    {
			    return false;
		    }
		    HandlePhaseTransitionIntent(PhaseIntent);
		    return true;
	    }));
	return true;
}

FName AReEchoEnemyActor::GetPresentationId() const
{
	return EnemyLogic ? EnemyLogic->GetDefinition().PresentationId : NAME_None;
}

void AReEchoEnemyActor::SetEnemyRoster(UReEchoEnemyRosterComponent* InRoster)
{
	if (EnemyRoster == InRoster)
	{
		return;
	}
	if (EnemyRoster)
	{
		ClearCrowdCollisionIgnores();
		EnemyRoster->UnregisterEnemy(this);
	}
	EnemyRoster = InRoster;
	if (EnemyRoster && EnemyLogic && EnemyLogic->IsInitialized())
	{
		EnemyRoster->RegisterEnemy(this, EnemyLogic);
		RefreshCrowdCollisionIgnores();
	}
}

bool AReEchoEnemyActor::IsAlive() const
{
	return Combatant && Combatant->IsAlive();
}

int32 AReEchoEnemyActor::GetSpawnIndex() const
{
	return EnemyLogic ? EnemyLogic->GetSnapshot().SpawnIndex : 0;
}

EReEchoEnemyKind AReEchoEnemyActor::GetKind() const
{
	return EnemyLogic ? ReEchoEnemyHost::ToLegacyKind(EnemyLogic->GetSnapshot().Archetype) : EReEchoEnemyKind::Grunt;
}

FReEchoEnemyRuntimeState AReEchoEnemyActor::CaptureRuntimeState() const
{
	FReEchoEnemyRuntimeState Result;
	const FReEchoEnemyLogicSnapshot LogicSnapshot =
	    EnemyLogic ? EnemyLogic->GetSnapshot() : FReEchoEnemyLogicSnapshot{};
	Result.LogicSnapshot = LogicSnapshot;
	Result.bHasLogicSnapshot = EnemyLogic != nullptr;
	Result.Kind = static_cast<uint8>(ReEchoEnemyHost::ToLegacyKind(LogicSnapshot.Archetype));
	Result.EnemyId = EnemyId;
	Result.SpawnIndex = LogicSnapshot.SpawnIndex;
	Result.Transform = GetActorTransform();
	Result.Transform.SetRotation(FQuat::Identity);
	Result.CurrentHealth = Combatant ? Combatant->CurrentHealth : 0.0f;
	Result.ElementState = Combatant ? Combatant->GetElementState() : FReEchoElementState{};
	const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	Result.ElementState.ImmunityUntil = FMath::Max(0.0f, Result.ElementState.ImmunityUntil - CurrentTimeSeconds);
	for (TPair<FName, float>& ActiveStatus : Result.ElementState.ActiveStatusUntilSeconds)
	{
		ActiveStatus.Value = FMath::Max(0.0f, ActiveStatus.Value - CurrentTimeSeconds);
	}
	Result.ElementState.BurnNextTickTimeSeconds =
	    Result.ElementState.bBurnActive
	        ? FMath::Max(0.0f, Result.ElementState.BurnNextTickTimeSeconds - CurrentTimeSeconds)
	        : 0.0f;
	Result.ElementState.BurnAttack = {};
	Result.AttackCooldown = LogicSnapshot.AttackCooldownRemainingSeconds;
	Result.FuseRemaining = LogicSnapshot.FuseRemainingSeconds;
	Result.bBomberFuseActive = LogicSnapshot.bFuseActive;
	Result.HitReactionRemaining = LogicSnapshot.HitReactionRemainingSeconds;
	Result.KnockbackVelocity = LogicSnapshot.KnockbackVelocity;
	Result.ShakeDirection =
	    FVector::CrossProduct(FVector::UpVector, LogicSnapshot.KnockbackVelocity.GetSafeNormal2D()).GetSafeNormal();
	Result.AttackSequence = LogicSnapshot.AttackSequence;
	Result.bSelfDestructCommitted = LogicSnapshot.bSelfDestructCommitted;
	Result.BossProjectiles = BossProjectiles;
	return Result;
}

void AReEchoEnemyActor::RestoreRuntimeState(const FReEchoEnemyRuntimeState& SavedState)
{
	const EReEchoEnemyKind SavedKind = SavedState.Kind <= static_cast<uint8>(EReEchoEnemyKind::Elite)
	                                       ? static_cast<EReEchoEnemyKind>(SavedState.Kind)
	                                       : EReEchoEnemyKind::Grunt;
	if (!EnemyLogic || !EnemyLogic->IsInitialized() ||
	    EnemyLogic->GetDefinition().Archetype != ReEchoEnemyHost::ToArchetype(SavedKind))
	{
		Configure(SavedKind, SavedState.SpawnIndex);
	}
	SetActorLocation(SavedState.Transform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorScale3D(SavedState.Transform.GetScale3D());
	Combatant->RestoreCurrentHealth(SavedState.CurrentHealth);

	FReEchoElementState RestoredElementState = SavedState.ElementState;
	const float CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RestoredElementState.ImmunityUntil = CurrentTimeSeconds + FMath::Max(0.0f, SavedState.ElementState.ImmunityUntil);
	for (TPair<FName, float>& ActiveStatus : RestoredElementState.ActiveStatusUntilSeconds)
	{
		ActiveStatus.Value = CurrentTimeSeconds + FMath::Max(0.0f, ActiveStatus.Value);
	}
	RestoredElementState.BurnNextTickTimeSeconds =
	    RestoredElementState.bBurnActive
	        ? CurrentTimeSeconds + FMath::Max(0.0f, SavedState.ElementState.BurnNextTickTimeSeconds)
	        : 0.0f;
	RestoredElementState.BurnAttack = {};
	Combatant->RestoreElementState(RestoredElementState);

	FReEchoEnemyLogicSnapshot LogicSnapshot = SavedState.LogicSnapshot;
	if (!SavedState.bHasLogicSnapshot)
	{
		LogicSnapshot.Archetype = ReEchoEnemyHost::ToArchetype(SavedKind);
		LogicSnapshot.SpawnIndex = SavedState.SpawnIndex;
		LogicSnapshot.AttackCooldownRemainingSeconds = FMath::Max(0.0f, SavedState.AttackCooldown);
		LogicSnapshot.FuseRemainingSeconds = FMath::Max(0.0f, SavedState.FuseRemaining);
		LogicSnapshot.bFuseActive = SavedState.bBomberFuseActive;
		LogicSnapshot.HitReactionRemainingSeconds = FMath::Max(0.0f, SavedState.HitReactionRemaining);
		LogicSnapshot.KnockbackVelocity = SavedState.KnockbackVelocity;
		LogicSnapshot.FacingDirection = SavedState.Transform.GetRotation().GetForwardVector().GetSafeNormal2D();
		if (LogicSnapshot.FacingDirection.IsNearlyZero())
		{
			LogicSnapshot.FacingDirection = FVector::ForwardVector;
		}
		LogicSnapshot.AttackSequence = FMath::Max<int64>(0, SavedState.AttackSequence);
		LogicSnapshot.bSelfDestructCommitted = SavedState.bSelfDestructCommitted;
		LogicSnapshot.Phase = LogicSnapshot.HitReactionRemainingSeconds > 0.0f ? EReEchoEnemyBehaviorPhase::HitReaction
		                      : LogicSnapshot.bFuseActive                      ? EReEchoEnemyBehaviorPhase::Fuse
		                                                                       : EReEchoEnemyBehaviorPhase::Idle;
	}
	LogicSnapshot.Archetype = ReEchoEnemyHost::ToArchetype(SavedKind);
	LogicSnapshot.SpawnIndex = SavedState.SpawnIndex;
	LogicSnapshot.bAlive = Combatant->IsAlive();
	if (!LogicSnapshot.bAlive)
	{
		LogicSnapshot.Phase = EReEchoEnemyBehaviorPhase::Dead;
	}
	EnemyLogic->RestoreSnapshot(LogicSnapshot);
	BossProjectiles.Reset();
	for (const FReEchoEnemyProjectileRuntimeState& SavedProjectile : SavedState.BossProjectiles)
	{
		FReEchoEnemyProjectileRuntimeState RestoredProjectile = SavedProjectile;
		if (FReEchoEnemyProjectileLogic::RestoreSnapshot(
		        SavedProjectile.Definition, SavedProjectile.Snapshot, RestoredProjectile.Snapshot))
		{
			RestoredProjectile.SpawnDelayRemainingSeconds =
			    FMath::Max(0.0f, RestoredProjectile.SpawnDelayRemainingSeconds);
			BossProjectiles.Add(MoveTemp(RestoredProjectile));
			if (BossProjectiles.Last().bSpawnEventPublished)
			{
				PublishProjectileEvent(EReEchoEnemyProjectileEventType::Spawned, BossProjectiles.Last());
			}
		}
	}
}

void AReEchoEnemyActor::SetEncounterSimulationSuspended(const bool bSuspended)
{
	if (bEncounterSimulationSuspended == bSuspended)
	{
		return;
	}

	if (bSuspended)
	{
		// Element attachments are encounter-scoped by design, even when the same enemy Host survives intermission.
		if (Combatant)
		{
			Combatant->ResetElementState();
		}
		const FReEchoEnemyLogicSnapshot PreviousSnapshot =
		    EnemyLogic ? EnemyLogic->GetSnapshot() : FReEchoEnemyLogicSnapshot{};
		if (EnemyLogic)
		{
			EnemyLogic->ResetEncounterTransientState();
			PublishSpecialActionTransition(PreviousSnapshot, FReEchoEnemyActionIntent{});
		}
		for (const FReEchoEnemyProjectileRuntimeState& Projectile : BossProjectiles)
		{
			if (Projectile.bSpawnEventPublished)
			{
				PublishProjectileEvent(EReEchoEnemyProjectileEventType::Ended, Projectile);
			}
		}
		BossProjectiles.Reset();
	}
	bEncounterSimulationSuspended = bSuspended;
}

bool AReEchoEnemyActor::IntersectsProjectilePath(const FVector& PathStart,
                                                 const FVector& PathEnd,
                                                 const float ProjectileRadius) const
{
	if (!Collision || !Collision->IsCollisionEnabled())
	{
		return false;
	}
	const FBox ExpandedBounds = Collision->Bounds.GetBox().ExpandBy(FMath::Max(0.0f, ProjectileRadius));
	return ExpandedBounds.IsInsideOrOn(PathStart) || ExpandedBounds.IsInsideOrOn(PathEnd) ||
	       FMath::LineBoxIntersection(ExpandedBounds, PathStart, PathEnd, PathEnd - PathStart);
}

void AReEchoEnemyActor::ConfigureGameplayPlane(const float InGameplayPlaneWorldZ)
{
	GameplayPlaneWorldZ = InGameplayPlaneWorldZ;
	AlignToGameplayPlane();
}

void AReEchoEnemyActor::AlignToGameplayPlane()
{
	if (!Collision)
	{
		return;
	}
	FVector CenteredLocation = GetActorLocation();
	CenteredLocation.Z = GameplayPlaneWorldZ + Collision->GetScaledBoxExtent().Z;
	SetActorLocation(CenteredLocation, false, nullptr, ETeleportType::TeleportPhysics);
	RefreshFootRoot();
}

EReEchoElement AReEchoEnemyActor::GetAttachedElement() const
{
	return Combatant ? Combatant->GetElementState().Attached : EReEchoElement::None;
}

const FReEchoElementState& AReEchoEnemyActor::GetElementState() const
{
	static const FReEchoElementState EmptyState;
	return Combatant ? Combatant->GetElementState() : EmptyState;
}

#if WITH_DEV_AUTOMATION_TESTS
FReEchoElementState& AReEchoEnemyActor::EditElementState()
{
	check(Combatant);
	return Combatant->EditElementStateForTests();
}
#endif

float AReEchoEnemyActor::ReceiveElementalDamage(const float Damage,
                                                const EReEchoElement Element,
                                                const FVector& SourceLocation,
                                                const float ReactionEfficiency,
                                                const FReEchoAttackIdentity Attack)
{
	FReEchoElementHitContext Context;
	Context.SourceLocation = SourceLocation;
	Context.Attack = Attack;
	Context.ReactionEfficiency = ReactionEfficiency;
	Context.SourceElementalAttack = Damage;
	Context.SourceEchoEfficiency = 1.0f;
	if (AActor* SourceActor = Attack.Source.Get())
	{
		if (const UReEchoCombatantComponent* SourceCombatant =
		        SourceActor->FindComponentByClass<UReEchoCombatantComponent>())
		{
			Context.SourceElementalAttack = SourceCombatant->Stats.ElementalAttack;
			Context.SourceEchoEfficiency = SourceCombatant->Stats.EchoEfficiency;
		}
	}
	return ReEchoElementReaction::ApplyHitToWorld(*this, Element, Damage, Context).ImmediateDamageApplied;
}

float AReEchoEnemyActor::ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const
{
	if (EnemyLogic && EnemyLogic->GetSnapshot().Phase == EReEchoEnemyBehaviorPhase::Transforming)
	{
		// The first depleted health bar is held at one survivable point until phase completion refills the authored
		// second-phase maximum. The logic phase is the durable gate, so pause/save restore cannot expose that point.
		return 0.0f;
	}
	if (!EnemyLogic || !EnemyLogic->GetDefinition().bUsesDirectionalShield)
	{
		return Intent.RawDamage;
	}
	const FVector ToSource = (Intent.SourceLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = ResolveFacingDirection();
	return FVector::DotProduct(Forward, ToSource) >= 0.0f ? 0.0f : Intent.RawDamage * 2.0f;
}

float AReEchoEnemyActor::ReceiveGrayboxDamage(const float Damage,
                                              const FVector& SourceLocation,
                                              const FLinearColor& DamageNumberColor,
                                              const FReEchoAttackIdentity Attack)
{
	FReEchoHitIntent Intent;
	Intent.Attack = Attack;
	Intent.Target = this;
	Intent.RawDamage = Damage;
	Intent.SourceLocation = SourceLocation;
	Intent.HitLocation = GetActorLocation();
	return ReEchoHitResolver::ResolvePhysicalHit(Intent).AppliedDamage;
}

void AReEchoEnemyActor::AdvanceEnemyProjectiles(const float DeltaSeconds)
{
	AActor* TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
	const UReEchoRunSubsystem* Run =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (Run && Run->GetCardRules().bEchoTaunts)
	{
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AReEchoEchoActor> EchoIt(GetWorld()); EchoIt; ++EchoIt)
		{
			if (EchoIt->IsCombatTargetAlive())
			{
				const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), EchoIt->GetActorLocation());
				if (DistanceSquared < BestDistanceSquared)
				{
					BestDistanceSquared = DistanceSquared;
					TargetActor = *EchoIt;
				}
			}
		}
	}
	IReEchoCombatTarget* Target = TargetActor ? Cast<IReEchoCombatTarget>(TargetActor) : nullptr;
	for (int32 ProjectileIndex = 0; ProjectileIndex < BossProjectiles.Num();)
	{
		FReEchoEnemyProjectileRuntimeState& Projectile = BossProjectiles[ProjectileIndex];
		float ProjectileDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
		if (!Projectile.bSpawnEventPublished)
		{
			Projectile.SpawnDelayRemainingSeconds -= ProjectileDeltaSeconds;
			if (Projectile.SpawnDelayRemainingSeconds > 0.0f)
			{
				++ProjectileIndex;
				continue;
			}
			ProjectileDeltaSeconds = FMath::Max(0.0f, -Projectile.SpawnDelayRemainingSeconds);
			Projectile.SpawnDelayRemainingSeconds = 0.0f;
			Projectile.bSpawnEventPublished = true;
			PublishProjectileEvent(EReEchoEnemyProjectileEventType::Spawned, Projectile);
		}
		if (ProjectileDeltaSeconds <= 0.0f)
		{
			++ProjectileIndex;
			continue;
		}
		const FReEchoEnemyProjectileAdvanceResult AdvanceResult =
		    FReEchoEnemyProjectileLogic::Advance(Projectile.Definition, ProjectileDeltaSeconds, Projectile.Snapshot);
		if (AdvanceResult.bMoved)
		{
			PublishProjectileEvent(EReEchoEnemyProjectileEventType::Moved, Projectile);
		}
		bool bHitTarget = false;
		if (!Projectile.bCollisionConsumed && AdvanceResult.bMoved && Target && Target->IsCombatTargetAlive())
		{
			// Arena combat is authored on XY. Presentation projectiles retain their source height, but actors with
			// different collision half-heights must still share the same authoritative combat plane.
			const FVector TargetLocation = Target->GetCombatTargetLocation();
			FVector CollisionPathStart = AdvanceResult.PreviousLocation;
			FVector CollisionPathEnd = AdvanceResult.NewLocation;
			CollisionPathStart.Z = TargetLocation.Z;
			CollisionPathEnd.Z = TargetLocation.Z;
			if (Target->IntersectsCombatPath(CollisionPathStart, CollisionPathEnd, Projectile.CollisionRadiusCm))
			{
				FReEchoBossIntent HitIntent;
				HitIntent.Attack = Projectile.Attack;
				HitIntent.Origin = AdvanceResult.PreviousLocation;
				HitIntent.RawDamage = Projectile.Damage;
				ApplyBossHit(HitIntent, TargetActor, TargetLocation);
				bHitTarget = true;
				Projectile.bCollisionConsumed = true;
			}
		}
		if (bHitTarget || AdvanceResult.bExpiredByRange || !Projectile.Snapshot.bActive)
		{
			PublishProjectileEvent(EReEchoEnemyProjectileEventType::Ended, Projectile);
			BossProjectiles.RemoveAt(ProjectileIndex, 1, EAllowShrinking::No);
			continue;
		}
		++ProjectileIndex;
	}
}

int32 AReEchoEnemyActor::DestroyRabbitProjectilesInMeleeArc(const FVector& Origin,
	                                                         const FVector& Forward,
	                                                         const float RangeCm,
	                                                         const float ArcDegrees)
{
	int32 RemovedCount = 0;
	for (int32 ProjectileIndex = BossProjectiles.Num() - 1; ProjectileIndex >= 0; --ProjectileIndex)
	{
		const FReEchoEnemyProjectileRuntimeState& Projectile = BossProjectiles[ProjectileIndex];
		if (Projectile.VolleyBallIndex == INDEX_NONE ||
		    !ReEchoWeaponGeometry::IsInsideMeleeArc(
		        Origin, Forward, Projectile.Snapshot.Location, RangeCm + Projectile.CollisionRadiusCm, ArcDegrees))
		{
			continue;
		}
		PublishProjectileEvent(EReEchoEnemyProjectileEventType::Ended, Projectile);
		BossProjectiles.RemoveAtSwap(ProjectileIndex, 1, EAllowShrinking::No);
		++RemovedCount;
	}
	return RemovedCount;
}

void AReEchoEnemyActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bEncounterSimulationSuspended)
	{
		return;
	}
	AdvanceEnemyProjectiles(DeltaSeconds);
	if (bDeathSequenceStarted)
	{
		EnemyPresentation->Advance(BuildPresentationSnapshot(false), DeltaSeconds);
		return;
	}
	if (IsAlive())
	{
		ReEchoElementReaction::TickElementStatuses(*this, GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f);
	}

	FReEchoEnemyActionIntent Intent;
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (IsAlive() && WorldTime >= CardStunnedUntilWorldTime && (!Combatant || !Combatant->IsActionDisabled(WorldTime)))
	{
		FReEchoEnemySenseSnapshot Sense;
		Sense.SelfLocation = GetActorLocation();
		Sense.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		AActor* DesiredTarget = UGameplayStatics::GetPlayerPawn(this, 0);
		const UReEchoRunSubsystem* Run =
		    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
		if (Run && Run->GetCardRules().bEchoTaunts)
		{
			float BestDistanceSquared = TNumericLimits<float>::Max();
			for (TActorIterator<AReEchoEchoActor> EchoIt(GetWorld()); EchoIt; ++EchoIt)
			{
				if (EchoIt->IsCombatTargetAlive())
				{
					const float DistanceSquared =
					    FVector::DistSquared2D(GetActorLocation(), EchoIt->GetActorLocation());
					if (DistanceSquared < BestDistanceSquared)
					{
						BestDistanceSquared = DistanceSquared;
						DesiredTarget = *EchoIt;
					}
				}
			}
		}
		if (AActor* TargetActor = DesiredTarget)
		{
			Sense.Target = TargetActor;
			Sense.TargetLocation = TargetActor->GetActorLocation();
			Sense.bTargetExists = true;
			const UReEchoCombatantComponent* PlayerCombatant =
			    TargetActor->FindComponentByClass<UReEchoCombatantComponent>();
			Sense.bTargetAlive = PlayerCombatant && PlayerCombatant->IsAlive();
			// DesiredTarget is the authoritative aggro selection for this tick. Echoes only reach this branch as
			// transform-range candidates when the run's taunt rule selected them above.
			Sense.bTargetCanAttractAggro =
			    Cast<AReEchoPlayerPawn>(TargetActor) != nullptr ||
			    (Run && Run->GetCardRules().bEchoTaunts && Cast<AReEchoEchoActor>(TargetActor) != nullptr);
			if (const AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(TargetActor))
			{
				Sense.bTargetInvulnerable = ReEchoPlayer->IsWeaponInvulnerable();
			}
			if (GetKind() == EReEchoEnemyKind::Boss)
			{
				Sense.TeleportDestination = ResolveBossTeleportDestination(Sense.TargetLocation);
				Sense.bHasTeleportDestination = !Sense.TeleportDestination.IsNearlyZero();
			}
		}
		// WS5: aggro injection for non-Boss idle wander. HateRangeCm <= 0 disables wander (legacy pursuit-only).
		const float AggroDistanceCm = FVector::Dist2D(GetActorLocation(), Sense.TargetLocation);
		Sense.HateRangeCm = EnemyLogic->GetDefinition().HateRangeCm;
		Sense.bInCombat = Sense.HateRangeCm > 0.0f && AggroDistanceCm <= Sense.HateRangeCm;
		AReEchoGameMode* ReEchoGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AReEchoGameMode>() : nullptr;
		Sense.bSpecialActionPermitted =
		    !ReEchoGameMode || ReEchoGameMode->CanStartEnemySpecial(EnemyId, GetSpawnIndex(), Sense.WorldTimeSeconds);
		// WS4 (Plan 68): sample current health ratio so the logic layer can drive a blood-depleted phase transition
		// without reaching into the combat component itself. Full/unknown defaults keep legacy enemies inert.
		if (Combatant && Combatant->Stats.HpMax > 0.0f)
		{
			Sense.CurrentHealthRatio = FMath::Clamp(Combatant->CurrentHealth / Combatant->Stats.HpMax, 0.0f, 1.0f);
		}
		const FReEchoEnemyLogicSnapshot PreviousLogicSnapshot = EnemyLogic->GetSnapshot();
		Intent = AdvanceBehavior(Sense, DeltaSeconds);
		HandlePhaseTransitionIntent(Intent);
		PublishSpecialActionTransition(PreviousLogicSnapshot, Intent);
		if (ReEchoGameMode && PreviousLogicSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None &&
		    EnemyLogic->GetSnapshot().SpecialActionPhase == EReEchoEnemySpecialActionPhase::Windup)
		{
			ReEchoGameMode->NotifyEnemySpecialStarted(EnemyId, GetSpawnIndex(), Sense.WorldTimeSeconds);
		}
	}
	EnemyPresentation->Advance(BuildPresentationSnapshot(Intent.bHasMovement), DeltaSeconds);

#if !UE_BUILD_SHIPPING
	// GM debug overlay (GMShowEnemyHealth): float remaining HP above the enemy's head when enabled.
	const AReEchoGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AReEchoGameMode>() : nullptr;
	if (GM && GM->IsEnemyHealthDebugEnabled() && IsAlive() && Combatant && Combatant->Stats.HpMax > 0.0f)
	{
		const FVector HeadLocation = GetActorLocation() + FVector(0.0f, 0.0f, 130.0f);
		const FString HealthText = FString::Printf(TEXT("HP %.0f / %.0f (%.0f%%)"),
		                                           Combatant->CurrentHealth,
		                                           Combatant->Stats.HpMax,
		                                           100.0f * Combatant->CurrentHealth / Combatant->Stats.HpMax);
		DrawDebugString(GetWorld(), HeadLocation, HealthText, nullptr, FColor::Green, 0.0f, true, 1.0f);
	}

	// GM debug overlay (GMShowEnemyRange): draw each enemy's damage range.
	// Red = contact/melee damage range (ContactRangeCm); Orange = farthest ranged damage range
	// (largest MaxRangeCm among abilities that deal damage).
	if (GM && GM->IsEnemyRangeDebugEnabled() && IsAlive() && EnemyLogic)
	{
		const FReEchoEnemyDefinition& Def = EnemyLogic->GetDefinition();
		const FVector Center = GetActorLocation();
		if (Def.ContactRangeCm > KINDA_SMALL_NUMBER)
		{
			DrawDebugCircle(GetWorld(),
			                Center,
			                Def.ContactRangeCm,
			                48,
			                FColor::Red,
			                false,
			                0.0f,
			                0,
			                2.0f,
			                FVector::ForwardVector,
			                FVector::RightVector);
			DrawDebugString(GetWorld(),
			                Center + FVector(Def.ContactRangeCm, 0.0f, 30.0f),
			                FString::Printf(TEXT("接触 %.0fcm"), Def.ContactRangeCm),
			                nullptr,
			                FColor::Red,
			                0.0f,
			                true,
			                1.0f);
		}
		float MaxRangedRangeCm = 0.0f;
		for (const FReEchoEnemyAbilityDefinition& Ability : Def.Abilities)
		{
			if (Ability.Damage > 0.0f && Ability.MaxRangeCm > MaxRangedRangeCm)
			{
				MaxRangedRangeCm = Ability.MaxRangeCm;
			}
		}
		if (MaxRangedRangeCm > KINDA_SMALL_NUMBER && MaxRangedRangeCm > Def.ContactRangeCm)
		{
			DrawDebugCircle(GetWorld(),
			                Center,
			                MaxRangedRangeCm,
			                48,
			                FColor(255, 140, 0),
			                false,
			                0.0f,
			                0,
			                2.0f,
			                FVector::ForwardVector,
			                FVector::RightVector);
			DrawDebugString(GetWorld(),
			                Center + FVector(MaxRangedRangeCm, 0.0f, 30.0f),
			                FString::Printf(TEXT("远程 %.0fcm"), MaxRangedRangeCm),
			                nullptr,
			                FColor(255, 140, 0),
			                0.0f,
			                true,
			                1.0f);
		}
	}
#endif
}

FReEchoEnemyActionIntent AReEchoEnemyActor::AdvanceBehavior(const FReEchoEnemySenseSnapshot& Sense,
                                                            const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent = EnemyLogic->Advance(Sense, DeltaSeconds);
	Intent.MovementDelta *= FMath::Clamp(CardMovementMultiplier, 0.0f, 1.0f);
	if (Intent.bHasMovement)
	{
		const FReEchoEnemyLogicSnapshot Snapshot = EnemyLogic->GetSnapshot();
		if (Snapshot.Archetype != EReEchoEnemyArchetype::Boss && Snapshot.Phase == EReEchoEnemyBehaviorPhase::Pursuing)
		{
			Intent.MovementDelta = ResolveCrowdMovement(Intent.MovementDelta, Sense.TargetLocation);
		}
		const FVector PreviousLocation = GetActorLocation();
		FHitResult Hit;
		AddActorWorldOffset(Intent.MovementDelta, true, &Hit);
		const float ExpectedDistance = Intent.MovementDelta.Size2D();
		const float ActualDistance = FVector::Dist2D(PreviousLocation, GetActorLocation());
		CrowdBlockedSeconds = ExpectedDistance > KINDA_SMALL_NUMBER && ActualDistance < ExpectedDistance * 0.2f
		                          ? CrowdBlockedSeconds + DeltaSeconds
		                          : 0.0f;
	}
	else
	{
		CrowdBlockedSeconds = 0.0f;
	}
	ApplyActionIntent(Intent);
	return Intent;
}

FVector AReEchoEnemyActor::ResolveCrowdMovement(const FVector& DesiredMovementDelta,
                                                const FVector& TargetLocation) const
{
	if (!EnemyRoster || !Collision || !EnemyLogic)
	{
		return DesiredMovementDelta;
	}

	FReEchoEnemyCrowdSteeringInput Input;
	Input.SelfLocation = GetActorLocation();
	Input.TargetLocation = TargetLocation;
	Input.DesiredMovementDelta = DesiredMovementDelta;
	Input.SelfRadiusCm = Collision->GetScaledBoxExtent().X;
	Input.PreferredTargetDistanceCm = EnemyLogic->GetDefinition().MovementStopDistanceCm;
	Input.BlockedSeconds = CrowdBlockedSeconds;
	Input.SpawnIndex = GetSpawnIndex();
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Neighbor = Cast<AReEchoEnemyActor>(Entry.Host.Get());
		if (!Entry.bAlive || !Neighbor || Neighbor == this || !Neighbor->Collision)
		{
			continue;
		}
		FReEchoEnemyCrowdNeighbor& Sample = Input.Neighbors.AddDefaulted_GetRef();
		Sample.Location = Neighbor->GetActorLocation();
		Sample.RadiusCm = Neighbor->Collision->GetScaledBoxExtent().X;
		Sample.SpawnIndex = Entry.SpawnIndex;
		Sample.bBoss = Entry.Archetype == EReEchoEnemyArchetype::Boss;
	}
	return FReEchoEnemyCrowdSteering::ResolveMovement(Input);
}

void AReEchoEnemyActor::RefreshCrowdCollisionIgnores()
{
	if (!EnemyRoster || !Collision || !EnemyLogic)
	{
		return;
	}
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Neighbor = Cast<AReEchoEnemyActor>(Entry.Host.Get());
		if (!Entry.bAlive || !Neighbor || Neighbor == this || !Neighbor->Collision || !Neighbor->EnemyLogic ||
		    !FReEchoEnemyCrowdSteering::ShouldIgnoreMovementCollision(EnemyLogic->GetSnapshot().Archetype,
		                                                              Neighbor->EnemyLogic->GetSnapshot().Archetype))
		{
			continue;
		}
		Collision->IgnoreActorWhenMoving(Neighbor, true);
		Neighbor->Collision->IgnoreActorWhenMoving(this, true);
	}
}

void AReEchoEnemyActor::ClearCrowdCollisionIgnores()
{
	if (!EnemyRoster || !Collision)
	{
		return;
	}
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Neighbor = Cast<AReEchoEnemyActor>(Entry.Host.Get());
		if (!Neighbor || Neighbor == this || !Neighbor->Collision)
		{
			continue;
		}
		Collision->IgnoreActorWhenMoving(Neighbor, false);
		Neighbor->Collision->IgnoreActorWhenMoving(this, false);
	}
}

#if WITH_DEV_AUTOMATION_TESTS
bool AReEchoEnemyActor::IsIgnoringEnemyMovementForTests(const AActor* Other) const
{
	return Collision && Collision->GetMoveIgnoreActors().Contains(Other);
}
#endif

void AReEchoEnemyActor::ApplyCardStun(const float DurationSeconds)
{
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	CardStunnedUntilWorldTime = FMath::Max(CardStunnedUntilWorldTime, WorldTime + FMath::Max(0.0f, DurationSeconds));
	if (Combatant)
	{
		FReEchoTimedStatusCommand Command;
		Command.StatusId = TEXT("Z_Vertigo");
		Command.CurrentTimeSeconds = WorldTime;
		Command.DurationSeconds = DurationSeconds;
		Combatant->ApplyTimedStatus(Command);
	}
}

void AReEchoEnemyActor::SetCardMovementMultiplier(const float Multiplier)
{
	CardMovementMultiplier = FMath::Clamp(Multiplier, 0.0f, 1.0f);
}

#if WITH_DEV_AUTOMATION_TESTS
FReEchoEnemyActionIntent AReEchoEnemyActor::AdvanceBehaviorForTests(const FReEchoEnemySenseSnapshot& Sense,
                                                                    const float DeltaSeconds)
{
	return AdvanceBehavior(Sense, DeltaSeconds);
}

void AReEchoEnemyActor::AdvanceEnemyProjectilesForTests(const float DeltaSeconds)
{
	AdvanceEnemyProjectiles(DeltaSeconds);
}
#endif

FVector AReEchoEnemyActor::ResolveBossTeleportDestination(const FVector& TargetLocation)
{
	// Blink Slam lands at the exact authored warning center. Overlap with the locked target is intentional because
	// the circular AOE, ground impact and Boss landing all share this single authoritative point.
	return ResolveBossLandingLocation(TargetLocation, GetActorLocation().Z);
}

FVector AReEchoEnemyActor::ResolveBossLandingLocation(const FVector& LockedTargetLocation, const float BossWorldZ)
{
	FVector Destination = LockedTargetLocation;
	Destination.Z = BossWorldZ;
	return Destination;
}

FVector AReEchoEnemyActor::ResolveFacingDirection() const
{
	const FVector Facing =
	    EnemyLogic ? EnemyLogic->GetSnapshot().FacingDirection.GetSafeNormal2D() : FVector::ZeroVector;
	return Facing.IsNearlyZero() ? FVector::ForwardVector : Facing;
}

void AReEchoEnemyActor::ApplyBossHit(const FReEchoBossIntent& Intent, AActor* Target, const FVector& HitLocation)
{
	if (!Target || Intent.RawDamage <= 0.0f)
	{
		return;
	}
	FReEchoHitIntent HitIntent;
	HitIntent.Attack = Intent.Attack;
	HitIntent.Target = Target;
	HitIntent.RawDamage = Intent.RawDamage;
	HitIntent.DamageSource = EReEchoDamageSource::Enemy;
	HitIntent.SourceLocation = Intent.Origin;
	HitIntent.HitLocation = HitLocation;
	const FReEchoHitResolved Resolved = ReEchoHitResolver::ResolvePhysicalHit(HitIntent);
	if (Resolved.AppliedDamage <= 0.0f)
	{
		return;
	}
	if (AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(Target))
	{
		ReEchoPlayer->PlayHitVisual();
	}
	AReEchoDamageNumberActor::SpawnDamageNumber(GetWorld(), HitLocation, Resolved.AppliedDamage, FLinearColor::White);
	if (EnemyId == TEXT("M_RABBIT"))
	{
		const IReEchoCombatTarget* CombatTarget = Cast<IReEchoCombatTarget>(Target);
		const UReEchoCombatantComponent* TargetCombatant =
		    CombatTarget ? CombatTarget->GetCombatTargetCombatant() : nullptr;
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[RabbitProjectileDamage] TableRaw=%.2f ResolvedRaw=%.2f Applied=%.2f TargetHealth=%.2f"),
		       Intent.RawDamage,
		       Resolved.RawDamage,
		       Resolved.AppliedDamage,
		       TargetCombatant ? TargetCombatant->CurrentHealth : -1.0f);
	}
}

void AReEchoEnemyActor::ApplyBossIntent(const FReEchoBossIntent& Intent)
{
	if (Intent.Type == EReEchoBossIntentType::ElementCleanse)
	{
		FReEchoElementCleanseCommand Command;
		Command.CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;
		Command.ImmunityDurationSeconds = Intent.ElementImmunitySeconds;
		Combatant->ExecuteElementCleanse(Command);
		return;
	}
	if (Intent.Type != EReEchoBossIntentType::AttackWindowStarted)
	{
		return;
	}
	CombatAudioAdapter->PostConfiguredAttack(Intent.Origin, Intent.AbilityId);

	AActor* Target = Intent.Target.Get();
	IReEchoCombatTarget* CombatTarget = Target ? Cast<IReEchoCombatTarget>(Target) : nullptr;
	if (!CombatTarget || !CombatTarget->IsCombatTargetAlive())
	{
		return;
	}
	if (Intent.bRequestTeleport && !Intent.TeleportDestination.IsNearlyZero())
	{
		SetActorLocation(Intent.TeleportDestination, false, nullptr, ETeleportType::TeleportPhysics);
	}

	const FVector TargetLocation = CombatTarget->GetCombatTargetLocation();
	bool bIntersectsAttack = false;
	switch (Intent.AttackShape)
	{
		case EReEchoBossAttackShape::Rectangle:
		case EReEchoBossAttackShape::Beam:
		{
			const FVector Direction = Intent.LockedDirection.GetSafeNormal2D();
			const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal2D();
			const FVector ToTarget = TargetLocation - Intent.Origin;
			const float ForwardDistance = FVector::DotProduct(ToTarget, Direction);
			const float SideDistance = FMath::Abs(FVector::DotProduct(ToTarget, Right));
			bIntersectsAttack =
			    ForwardDistance >= 0.0f && ForwardDistance <= Intent.LengthCm && SideDistance <= Intent.WidthCm * 0.5f;
			break;
		}
		case EReEchoBossAttackShape::Circle:
			bIntersectsAttack =
			    FVector::DistSquared2D(TargetLocation, GetActorLocation()) <= FMath::Square(Intent.RadiusCm);
			break;
		case EReEchoBossAttackShape::Projectile:
		{
			const FReEchoEnemyAbilityDefinition* VolleyAbility = FindAbility(Intent.AbilityId);
			const int32 VolleyCount = VolleyAbility ? FMath::Max(1, VolleyAbility->ProjectileCount) : 1;
			const float VolleySpreadDegrees = VolleyAbility ? VolleyAbility->SpreadAngleDegrees : 0.0f;
			const bool bSequentialVolley = VolleyAbility && !VolleyAbility->bMovementDuringCast && VolleyCount > 1;
			const float VolleyIntervalSeconds = bSequentialVolley
			                                        ? (Intent.RecoverySeconds > KINDA_SMALL_NUMBER
			                                               ? Intent.RecoverySeconds / static_cast<float>(VolleyCount)
			                                               : 0.1f)
			                                        : 0.0f;
			for (int32 BallIndex = 0; BallIndex < VolleyCount; ++BallIndex)
			{
				FReEchoEnemyProjectileRuntimeState Projectile;
				Projectile.Definition.InitialLocation = Intent.Origin;
				Projectile.Definition.Direction = ReEchoRabbitProjectilePattern::ResolveVolleyDirection(
				    Intent.LockedDirection.GetSafeNormal2D(), VolleySpreadDegrees, BallIndex, VolleyCount);
				Projectile.Definition.SpeedCmPerSecond = Intent.ProjectileSpeedCmPerSecond;
				Projectile.Definition.MaxRangeCm = VolleyAbility ? VolleyAbility->MaxRangeCm : Intent.LengthCm;
				Projectile.Attack = Intent.Attack;
				Projectile.Damage = Intent.RawDamage;
				Projectile.CollisionRadiusCm =
				    ReEchoRabbitProjectilePattern::ResolveBallCollisionRadius(Intent.RadiusCm, VolleyCount);
				Projectile.VolleyBallIndex = BallIndex;
				Projectile.SpawnDelayRemainingSeconds = VolleyIntervalSeconds * static_cast<float>(BallIndex);
				Projectile.bSpawnEventPublished = Projectile.SpawnDelayRemainingSeconds <= KINDA_SMALL_NUMBER;
				if (FReEchoEnemyProjectileLogic::Initialize(Projectile.Definition, Projectile.Snapshot))
				{
					BossProjectiles.Add(MoveTemp(Projectile));
					if (BossProjectiles.Last().bSpawnEventPublished)
					{
						PublishProjectileEvent(EReEchoEnemyProjectileEventType::Spawned, BossProjectiles.Last());
					}
				}
			}
			break;
		}
		default:
			break;
	}
	if (Intent.bCanDamageTarget && bIntersectsAttack)
	{
		ApplyBossHit(Intent, Target, TargetLocation);
	}
}

void AReEchoEnemyActor::ApplyActionIntent(const FReEchoEnemyActionIntent& Intent)
{
	if (!Intent.BossIntents.IsEmpty())
	{
		for (const FReEchoBossIntent& BossIntent : Intent.BossIntents)
		{
			ApplyBossIntent(BossIntent);
		}
		return;
	}
	if (!Intent.bAttackCommitted)
	{
		return;
	}
	CombatAudioAdapter->PostConfiguredAttack(Intent.SourceLocation);
	const FReEchoEnemyLogicSnapshot LogicSnapshot = EnemyLogic->GetSnapshot();
	const bool bRangedProjectile = EnemyLogic->GetDefinition().Archetype == EReEchoEnemyArchetype::Ranged;
	if (bRangedProjectile)
	{
		const FReEchoEnemyAbilityDefinition* Ability = FindAbility(LogicSnapshot.SpecialAbilityId);
		if (Ability)
		{
			const float DerivedLegacySpeed =
			    Ability->CooldownSeconds > KINDA_SMALL_NUMBER ? Ability->MaxRangeCm / Ability->CooldownSeconds : 0.0f;
			const int32 VolleyCount = FMath::Max(1, Ability->ProjectileCount);
			const float VolleySpreadDegrees = Ability->SpreadAngleDegrees;
			const bool bSequentialStraightVolley =
			    VolleyCount > 1 && FMath::IsNearlyZero(VolleySpreadDegrees) && Ability->ActiveSeconds > 0.0f;
			const float ShotIntervalSeconds =
			    bSequentialStraightVolley ? Ability->ActiveSeconds / static_cast<float>(VolleyCount - 1) : 0.0f;
			for (int32 BallIndex = 0; BallIndex < VolleyCount; ++BallIndex)
			{
				FReEchoEnemyProjectileRuntimeState Projectile;
				Projectile.Definition.InitialLocation = Intent.SourceLocation;
				Projectile.Definition.Direction = ReEchoRabbitProjectilePattern::ResolveVolleyDirection(
				    LogicSnapshot.SpecialLockedDirection, VolleySpreadDegrees, BallIndex, VolleyCount);
				Projectile.Definition.SpeedCmPerSecond = Ability->ProjectileSpeedCmPerSecond > 0.0f
				                                             ? Ability->ProjectileSpeedCmPerSecond
				                                             : DerivedLegacySpeed;
				Projectile.Definition.MaxRangeCm = Ability->MaxRangeCm;
				Projectile.Attack = Intent.Attack;
				Projectile.Damage = Intent.RawDamage;
				Projectile.CollisionRadiusCm =
				    ReEchoRabbitProjectilePattern::ResolveBallCollisionRadius(Ability->RadiusCm, VolleyCount);
				Projectile.VolleyBallIndex = BallIndex;
				Projectile.SpawnDelayRemainingSeconds = ShotIntervalSeconds * BallIndex;
				Projectile.bSpawnEventPublished = !bSequentialStraightVolley || BallIndex == 0;
				if (FReEchoEnemyProjectileLogic::Initialize(Projectile.Definition, Projectile.Snapshot))
				{
					BossProjectiles.Add(MoveTemp(Projectile));
					if (BossProjectiles.Last().bSpawnEventPublished)
					{
						PublishProjectileEvent(EReEchoEnemyProjectileEventType::Spawned, BossProjectiles.Last());
					}
				}
			}
		}
	}
	else if (Intent.bCanDamageTarget && Intent.Target.IsValid())
	{
		FReEchoHitIntent HitIntent;
		HitIntent.Attack = Intent.Attack;
		HitIntent.Target = Intent.Target.Get();
		HitIntent.RawDamage = Intent.RawDamage;
		HitIntent.DamageSource = EReEchoDamageSource::Enemy;
		HitIntent.SourceLocation = Intent.SourceLocation;
		HitIntent.HitLocation = Intent.HitLocation;
		const FReEchoHitResolved Resolved = ReEchoHitResolver::ResolvePhysicalHit(HitIntent);
		if (Resolved.AppliedDamage > 0.0f)
		{
			if (AReEchoPlayerPawn* ReEchoPlayer = Cast<AReEchoPlayerPawn>(Intent.Target.Get()))
			{
				ReEchoPlayer->PlayHitVisual();
			}
			AReEchoDamageNumberActor::SpawnDamageNumber(
			    GetWorld(), Intent.HitLocation, Resolved.AppliedDamage, FLinearColor::White);
		}
	}
	if (Intent.bSelfDestructAfterAttack && IsAlive())
	{
		FReEchoHitIntent SelfDestruct;
		SelfDestruct.Attack = Intent.Attack;
		SelfDestruct.Target = this;
		SelfDestruct.RawDamage = TNumericLimits<float>::Max();
		SelfDestruct.DamageSource = EReEchoDamageSource::Enemy;
		SelfDestruct.bAllowSameFactionDamage = true;
		SelfDestruct.SourceLocation = GetActorLocation();
		SelfDestruct.HitLocation = GetActorLocation();
		ReEchoHitResolver::ResolvePhysicalHit(SelfDestruct);
	}
}

const FReEchoEnemyAbilityDefinition* AReEchoEnemyActor::FindAbility(const FName AbilityId) const
{
	if (!EnemyLogic || AbilityId.IsNone())
	{
		return nullptr;
	}
	return EnemyLogic->GetDefinition().Abilities.FindByPredicate(
	    [AbilityId](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == AbilityId;
	    });
}

void AReEchoEnemyActor::PublishSpecialActionTransition(const FReEchoEnemyLogicSnapshot& PreviousSnapshot,
                                                       const FReEchoEnemyActionIntent& Intent)
{
	if (!EnemyEvents)
	{
		return;
	}
	const FReEchoEnemyLogicSnapshot CurrentSnapshot = EnemyLogic->GetSnapshot();
	if (PreviousSnapshot.SpecialActionPhase == CurrentSnapshot.SpecialActionPhase)
	{
		return;
	}
	FReEchoEnemySpecialActionEvent Event;
	Event.AbilityId = CurrentSnapshot.SpecialAbilityId.IsNone() ? PreviousSnapshot.SpecialAbilityId
	                                                            : CurrentSnapshot.SpecialAbilityId;
	Event.Attack = Intent.Attack;
	Event.Origin = GetActorLocation();
	Event.LockedDirection = CurrentSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None
	                            ? PreviousSnapshot.SpecialLockedDirection
	                            : CurrentSnapshot.SpecialLockedDirection;
	Event.LockedTargetLocation = CurrentSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None
	                                 ? PreviousSnapshot.SpecialLockedTargetLocation
	                                 : CurrentSnapshot.SpecialLockedTargetLocation;
	if (PreviousSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None &&
	    CurrentSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Windup)
	{
		Event.Type = EReEchoEnemySpecialActionEventType::WindupStarted;
	}
	else if (Intent.bAttackCommitted && CurrentSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Recovery)
	{
		Event.Type = EReEchoEnemySpecialActionEventType::ActionCommitted;
	}
	else if (CurrentSnapshot.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None)
	{
		Event.Type = EReEchoEnemySpecialActionEventType::ActionEnded;
	}
	else
	{
		return;
	}
	EnemyEvents->PublishSpecialAction(Event);
}

void AReEchoEnemyActor::PublishProjectileEvent(const EReEchoEnemyProjectileEventType Type,
                                               const FReEchoEnemyProjectileRuntimeState& Projectile) const
{
	if (!EnemyEvents)
	{
		return;
	}
	FReEchoEnemyProjectileEvent Event;
	Event.Type = Type;
	Event.Attack = Projectile.Attack;
	Event.AbilityId = EnemyId == TEXT("M_RABBIT")  ? FName(TEXT("M_RABBIT_RangedBurst"))
	                  : EnemyId == TEXT("M_SHEEP") ? FName(TEXT("M_SHEEP_Projectile"))
	                                               : NAME_None;
	Event.Location = Projectile.Snapshot.Location;
	Event.Direction = Projectile.Snapshot.Direction;
	Event.VolleyBallIndex = Projectile.VolleyBallIndex;
	Event.CollisionRadiusCm = Projectile.CollisionRadiusCm;
	EnemyEvents->PublishProjectile(Event);
}

FReEchoEnemyPresentationSnapshot AReEchoEnemyActor::BuildPresentationSnapshot(const bool bMoving) const
{
	FReEchoEnemyPresentationSnapshot Result;
	const FReEchoEnemyLogicSnapshot LogicSnapshot = EnemyLogic->GetSnapshot();
	const FReEchoEnemyDefinition& Definition = EnemyLogic->GetDefinition();
	Result.Archetype = LogicSnapshot.Archetype;
	Result.Phase = LogicSnapshot.Phase;
	Result.PresentationId = Definition.PresentationId;
	Result.SpawnIndex = LogicSnapshot.SpawnIndex;
	Result.FacingDirection = LogicSnapshot.FacingDirection;
	Result.KnockbackVelocity = LogicSnapshot.KnockbackVelocity;
	Result.HealthRatio =
	    Combatant && Combatant->Stats.HpMax > 0.0f ? Combatant->CurrentHealth / Combatant->Stats.HpMax : 0.0f;
	Result.FuseRemainingSeconds = LogicSnapshot.FuseRemainingSeconds;
	Result.FuseDurationSeconds = Definition.BomberFuseDurationSeconds;
	Result.HitReactionRemainingSeconds = LogicSnapshot.HitReactionRemainingSeconds;
	Result.HitReactionDurationSeconds = Definition.HitReactionDurationSeconds;
	Result.AttachedElement = GetAttachedElement();
	Result.bMoving = bMoving;
	return Result;
}

void AReEchoEnemyActor::HandleCombatDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target != this || bDeathSequenceStarted)
	{
		return;
	}
	bDeathSequenceStarted = true;
	if (EnemyLogic)
	{
		EnemyLogic->NotifyDeath();
	}
	SetActorEnableCollision(false);
	float ExpectedDurationSeconds = 0.0f;
	const bool bPlayingDeath =
	    EnemyPresentation &&
	    EnemyPresentation->BeginTerminalDeath(
	        FSimpleDelegate::CreateUObject(this, &AReEchoEnemyActor::CompleteDeathSequence), ExpectedDurationSeconds);
	if (!bPlayingDeath)
	{
		// Avoid destroying the owner from inside the OnDeath multicast stack. This is effectively immediate
		// while allowing remaining death-only subscribers to finish deterministically.
		SetLifeSpan(UE_KINDA_SMALL_NUMBER);
		return;
	}
	constexpr float CompletionGraceSeconds = 0.1f;
	constexpr float MaximumDeathLifetimeSeconds = 10.0f;
	SetLifeSpan(FMath::Clamp(
	    ExpectedDurationSeconds + CompletionGraceSeconds, CompletionGraceSeconds, MaximumDeathLifetimeSeconds));
}

void AReEchoEnemyActor::CompleteDeathSequence()
{
	if (!IsActorBeingDestroyed())
	{
		Destroy();
	}
}

void AReEchoEnemyActor::HandlePhaseTransitionIntent(const FReEchoEnemyActionIntent& Intent)
{
	if ((!Intent.bPhaseTransitionStarted && !Intent.bPhaseTransitionCompleted) || !EnemyLogic || !EnemyEvents)
	{
		return;
	}

	FReEchoEnemyPhaseTransitionEvent PhaseEvent;
	PhaseEvent.PhaseId = EnemyLogic->GetDefinition().Phase2.Id;
	PhaseEvent.AnimationSetId = EnemyLogic->GetDefinition().Phase2.AnimationSetId;
	PhaseEvent.TriggerReason = Intent.PhaseTriggerReason;
	PhaseEvent.DurationSeconds = EnemyLogic->GetDefinition().Phase2.TransformSeconds;
	PhaseEvent.bStarted = Intent.bPhaseTransitionStarted;
	EnemyEvents->PublishPhaseTransition(PhaseEvent);

	if (Intent.bPhaseTransitionCompleted &&
	    Intent.PhaseTriggerReason == EReEchoEnemyPhaseTriggerReason::HealthDepleted &&
	    EnemyLogic->GetDefinition().Archetype == EReEchoEnemyArchetype::Boss)
	{
		ApplyBloodDepletedPhase2MaxHealth();
	}
}

void AReEchoEnemyActor::ApplyBloodDepletedPhase2MaxHealth()
{
	if (!Combatant || !EnemyLogic || EnemyLogic->GetDefinition().BossPhases.Num() == 0)
	{
		return;
	}
	// Locate the second-phase definition by PhaseIndex and apply its maximum health only when it requests a refill.
	const FReEchoBossPhaseDefinition* PhaseTwo = nullptr;
	for (const FReEchoBossPhaseDefinition& Phase : EnemyLogic->GetDefinition().BossPhases)
	{
		if (Phase.PhaseIndex == 2)
		{
			PhaseTwo = &Phase;
			break;
		}
	}
	if (!PhaseTwo || PhaseTwo->RefillHealthPolicy != EReEchoBossRefillHealthPolicy::RefillToMaximum ||
	    PhaseTwo->PhaseMaxHealth <= 0.0f)
	{
		return;
	}
	// Resize to the new ceiling and refill to full so the second form starts as a fresh fight.
	Combatant->Stats.HpMax = PhaseTwo->PhaseMaxHealth;
	Combatant->InitializeFromStats(Combatant->Stats, /*bFullHealth=*/true);
}
