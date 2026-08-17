#include "Graybox/ReEchoEnemyActor.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatAudioAdapterComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DFrameCollisionDriver.h"
#include "Presentation/Animation2D/ReEcho2DPresentationController.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "ReEchoAudioEvents.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "Graybox/ReEchoAttackEffects.h"

namespace ReEchoEnemyHost
{
constexpr float ScaleMultiplier = 1.5f;
constexpr float CharacterWorldHeight = 244.8f * ScaleMultiplier;
constexpr float CollisionRadius = 34.56f * ScaleMultiplier;
constexpr float CollisionHalfHeight = 122.4f * ScaleMultiplier;
constexpr float ShadowScaleX = 0.512f * ScaleMultiplier;
constexpr float ShadowScaleY = 0.5376f * ScaleMultiplier;

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

	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitCapsuleSize(ReEchoEnemyHost::CollisionRadius, ReEchoEnemyHost::CollisionHalfHeight);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	Collision->SetVisibility(false);

	UStaticMeshComponent* GroundShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundShadow"));
	GroundShadow->SetupAttachment(RootComponent);
	GroundShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundShadow->SetCastShadow(false);
	GroundShadow->SetTranslucentSortPriority(-1);
	GroundShadow->SetAbsolute(false, false, true);
	GroundShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -ReEchoEnemyHost::CharacterWorldHeight * 0.28f));
	GroundShadow->SetRelativeScale3D(FVector(ReEchoEnemyHost::ShadowScaleX, ReEchoEnemyHost::ShadowScaleY, 1.0f));

	USceneComponent* VisualEffectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualEffectRoot"));
	VisualEffectRoot->SetupAttachment(RootComponent);
	UBillboardComponent* CharacterSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CharacterSprite"));
	CharacterSprite->SetupAttachment(VisualEffectRoot);
	CharacterSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterSprite->SetHiddenInGame(false);
	CharacterSprite->SetVisibility(false);
	CharacterSprite->SetAbsolute(false, false, true);
	CharacterSprite->bIsScreenSizeScaled = false;
	UReEcho2DAnimationComponent* SequenceAnimation =
	    CreateDefaultSubobject<UReEcho2DAnimationComponent>(TEXT("SequenceAnimation"));
	SequenceAnimation->SetupAttachment(VisualEffectRoot);
	UReEcho2DPresentationController* PresentationController =
	    CreateDefaultSubobject<UReEcho2DPresentationController>(TEXT("PresentationController"));
	UReEcho2DFrameCollisionDriver* FrameCollisionDriver =
	    CreateDefaultSubobject<UReEcho2DFrameCollisionDriver>(TEXT("FrameCollisionDriver"));

	UTextRenderComponent* ElementAuraRing = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementAuraRing"));
	ElementAuraRing->SetupAttachment(RootComponent);
	ElementAuraRing->SetHorizontalAlignment(EHTA_Center);
	ElementAuraRing->SetVerticalAlignment(EVRTA_TextCenter);
	ElementAuraRing->SetWorldSize(270.0f);
	ElementAuraRing->SetText(FText::FromString(TEXT("O")));
	ElementAuraRing->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	ElementAuraRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementAuraRing->SetCastShadow(false);
	ElementAuraRing->SetTranslucentSortPriority(-2);
	ElementAuraRing->SetVisibility(false);

	UTextRenderComponent* ElementAttachmentLabel =
	    CreateDefaultSubobject<UTextRenderComponent>(TEXT("ElementAttachmentLabel"));
	ElementAttachmentLabel->SetupAttachment(RootComponent);
	ElementAttachmentLabel->SetHorizontalAlignment(EHTA_Center);
	ElementAttachmentLabel->SetVerticalAlignment(EVRTA_TextCenter);
	ElementAttachmentLabel->SetWorldSize(30.0f);
	ElementAttachmentLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 205.0f));
	ElementAttachmentLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ElementAttachmentLabel->SetCastShadow(false);
	ElementAttachmentLabel->SetTranslucentSortPriority(23);
	ElementAttachmentLabel->SetVisibility(false);

	if (UMaterialInterface* UnlitTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/UnlitText.UnlitText")))
	{
		ElementAuraRing->SetTextMaterial(UnlitTextMaterial);
		ElementAttachmentLabel->SetTextMaterial(UnlitTextMaterial);
	}

	UPointLightComponent* ElementAuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ElementAuraLight"));
	ElementAuraLight->SetupAttachment(RootComponent);
	ElementAuraLight->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	ElementAuraLight->SetIntensity(900.0f);
	ElementAuraLight->SetAttenuationRadius(240.0f);
	ElementAuraLight->SetSourceRadius(35.0f);
	ElementAuraLight->SetCastShadows(false);
	ElementAuraLight->SetVisibility(false);

	Combatant = CreateDefaultSubobject<UReEchoCombatantComponent>(TEXT("Combatant"));
	CombatEvents = CreateDefaultSubobject<UReEchoCombatEventsComponent>(TEXT("CombatEvents"));
	CombatAudioAdapter = CreateDefaultSubobject<UReEchoCombatAudioAdapterComponent>(TEXT("CombatAudioAdapter"));
	EnemyLogic = CreateDefaultSubobject<UReEchoEnemyLogicComponent>(TEXT("EnemyLogic"));
	EnemyEvents = CreateDefaultSubobject<UReEchoEnemyEventsComponent>(TEXT("EnemyEvents"));
	EnemyPresentation = CreateDefaultSubobject<UReEchoEnemyPresentationComponent>(TEXT("EnemyPresentation"));
	EnemyPresentation->ConfigureComponents(VisualEffectRoot,
	                                       CharacterSprite,
	                                       SequenceAnimation,
	                                       PresentationController,
	                                       FrameCollisionDriver,
	                                       GroundShadow,
	                                       ElementAuraRing,
	                                       ElementAttachmentLabel,
	                                       ElementAuraLight,
	                                       Collision);

	Tags.Add(TEXT("ReEchoEnemy"));
}

void AReEchoEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Combatant->BindToAbilitySystem(AbilitySystem);
	BindComposedComponents();
}

void AReEchoEnemyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EnemyRoster)
	{
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
	EnemyPresentation->BindEventSources(this, Combatant, EnemyEvents, CombatEvents);
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

bool AReEchoEnemyActor::ConfigureFromDefinition(const FReEchoEnemyDefinition& Definition, const int32 SpawnIndex)
{
	BindComposedComponents();
	if (EnemyRoster)
	{
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
	if (!bVisualPlacementApplied)
	{
		SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, ReEchoEnemyHost::CharacterWorldHeight * 0.5f),
		                 false,
		                 nullptr,
		                 ETeleportType::TeleportPhysics);
		bVisualPlacementApplied = true;
	}
	EnemyPresentation->ConfigureAppearance(Definition.Archetype, SpawnIndex);
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
	}
	return true;
}

void AReEchoEnemyActor::SetEnemyRoster(UReEchoEnemyRosterComponent* InRoster)
{
	if (EnemyRoster == InRoster)
	{
		return;
	}
	if (EnemyRoster)
	{
		EnemyRoster->UnregisterEnemy(this);
	}
	EnemyRoster = InRoster;
	if (EnemyRoster && EnemyLogic && EnemyLogic->IsInitialized())
	{
		EnemyRoster->RegisterEnemy(this, EnemyLogic);
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
	Result.SpawnIndex = LogicSnapshot.SpawnIndex;
	Result.Transform = GetActorTransform();
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
	const EReEchoEnemyKind SavedKind = SavedState.Kind <= static_cast<uint8>(EReEchoEnemyKind::Boss)
	                                       ? static_cast<EReEchoEnemyKind>(SavedState.Kind)
	                                       : EReEchoEnemyKind::Grunt;
	if (!EnemyLogic || !EnemyLogic->IsInitialized() ||
	    EnemyLogic->GetDefinition().Archetype != ReEchoEnemyHost::ToArchetype(SavedKind))
	{
		Configure(SavedKind, SavedState.SpawnIndex);
	}
	SetActorTransform(SavedState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
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
		LogicSnapshot.FacingDirection = GetActorForwardVector().GetSafeNormal2D();
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
			BossProjectiles.Add(MoveTemp(RestoredProjectile));
		}
	}
	EnemyPresentation->RefreshElementAttachmentVisual();
}

bool AReEchoEnemyActor::IntersectsProjectilePath(const FVector& PathStart,
                                                 const FVector& PathEnd,
                                                 const float ProjectileRadius) const
{
	if (!Collision || !Collision->IsCollisionEnabled())
	{
		return false;
	}
	const FVector CapsuleCenter = Collision->GetComponentLocation();
	const FVector CapsuleAxis = Collision->GetUpVector();
	const float CapsuleRadius = Collision->GetScaledCapsuleRadius();
	const float CapsuleSegmentHalfLength = FMath::Max(0.0f, Collision->GetScaledCapsuleHalfHeight() - CapsuleRadius);
	const FVector CapsuleStart = CapsuleCenter - CapsuleAxis * CapsuleSegmentHalfLength;
	const FVector CapsuleEnd = CapsuleCenter + CapsuleAxis * CapsuleSegmentHalfLength;
	FVector ClosestOnProjectile;
	FVector ClosestOnCapsule;
	FMath::SegmentDistToSegmentSafe(
	    PathStart, PathEnd, CapsuleStart, CapsuleEnd, ClosestOnProjectile, ClosestOnCapsule);
	const float CombinedRadius = CapsuleRadius + FMath::Max(0.0f, ProjectileRadius);
	return FVector::DistSquared(ClosestOnProjectile, ClosestOnCapsule) <= FMath::Square(CombinedRadius);
}

void AReEchoEnemyActor::RefreshElementAttachmentVisual()
{
	EnemyPresentation->RefreshElementAttachmentVisual();
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
	if (!EnemyLogic || EnemyLogic->GetSnapshot().Archetype != EReEchoEnemyArchetype::Shield)
	{
		return Intent.RawDamage;
	}
	const FVector ToSource = (Intent.SourceLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
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

void AReEchoEnemyActor::AdvanceBossProjectiles(const float DeltaSeconds)
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
	for (int32 ProjectileIndex = BossProjectiles.Num() - 1; ProjectileIndex >= 0; --ProjectileIndex)
	{
		FReEchoEnemyProjectileRuntimeState& Projectile = BossProjectiles[ProjectileIndex];
		const FReEchoEnemyProjectileAdvanceResult AdvanceResult =
		    FReEchoEnemyProjectileLogic::Advance(Projectile.Definition, DeltaSeconds, Projectile.Snapshot);
		bool bHitTarget = false;
		if (AdvanceResult.bMoved && Target && Target->IsCombatTargetAlive() &&
		    Target->IntersectsCombatPath(
		        AdvanceResult.PreviousLocation, AdvanceResult.NewLocation, Projectile.CollisionRadiusCm))
		{
			FReEchoBossIntent HitIntent;
			HitIntent.Attack = Projectile.Attack;
			HitIntent.Origin = AdvanceResult.PreviousLocation;
			HitIntent.RawDamage = Projectile.Damage;
			ApplyBossHit(HitIntent, TargetActor, Target->GetCombatTargetLocation());
			bHitTarget = true;
		}
		if (bHitTarget || AdvanceResult.bExpiredByRange || !Projectile.Snapshot.bActive)
		{
			BossProjectiles.RemoveAtSwap(ProjectileIndex, 1, EAllowShrinking::No);
		}
	}
}

void AReEchoEnemyActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceBossProjectiles(DeltaSeconds);
	if (IsAlive())
	{
		ReEchoElementReaction::TickElementStatuses(*this, GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f);
	}

	FReEchoEnemyActionIntent Intent;
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (IsAlive() && WorldTime >= CardStunnedUntilWorldTime)
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
		Intent = AdvanceBehavior(Sense, DeltaSeconds);
	}
	EnemyPresentation->Advance(BuildPresentationSnapshot(Intent.bHasMovement), DeltaSeconds);
}

FReEchoEnemyActionIntent AReEchoEnemyActor::AdvanceBehavior(const FReEchoEnemySenseSnapshot& Sense,
                                                            const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent = EnemyLogic->Advance(Sense, DeltaSeconds);
	Intent.MovementDelta *= FMath::Clamp(CardMovementMultiplier, 0.0f, 1.0f);
	if (Intent.bHasFacing)
	{
		SetActorRotation(Intent.FacingDirection.Rotation());
	}
	if (Intent.bHasMovement)
	{
		AddActorWorldOffset(Intent.MovementDelta, true);
	}
	ApplyActionIntent(Intent);
	return Intent;
}

void AReEchoEnemyActor::ApplyCardStun(const float DurationSeconds)
{
	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	CardStunnedUntilWorldTime = FMath::Max(CardStunnedUntilWorldTime, WorldTime + FMath::Max(0.0f, DurationSeconds));
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
#endif

FVector AReEchoEnemyActor::ResolveBossTeleportDestination(const FVector& TargetLocation)
{
	float TeleportOffsetCm = 0.0f;
	if (EnemyLogic)
	{
		for (const FReEchoEnemyAbilityDefinition& Ability : EnemyLogic->GetDefinition().BossAbilities)
		{
			if (Ability.BehaviorId == TEXT("Boss.BlinkSlam") && Ability.bEnabled)
			{
				TeleportOffsetCm = Ability.TeleportOffsetCm;
				break;
			}
		}
	}
	if (TeleportOffsetCm <= 0.0f || !GetWorld())
	{
		return FVector::ZeroVector;
	}

	FVector AwayFromTarget = (GetActorLocation() - TargetLocation).GetSafeNormal2D();
	if (AwayFromTarget.IsNearlyZero())
	{
		AwayFromTarget = -GetActorForwardVector().GetSafeNormal2D();
	}
	FVector Candidate = TargetLocation + AwayFromTarget * TeleportOffsetCm;
	Candidate.Z = GetActorLocation().Z;
	FRotator CandidateRotation = (TargetLocation - Candidate).GetSafeNormal2D().Rotation();
	return GetWorld()->FindTeleportSpot(this, Candidate, CandidateRotation) ? Candidate : FVector::ZeroVector;
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
	ReEchoAttackEffects::SpawnHitImpact(GetWorld(), HitLocation);
	AReEchoDamageNumberActor::SpawnDamageNumber(GetWorld(), HitLocation, Resolved.AppliedDamage, FLinearColor::White);
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
			FReEchoEnemyProjectileRuntimeState Projectile;
			Projectile.Definition.InitialLocation = Intent.Origin;
			Projectile.Definition.Direction = Intent.LockedDirection.GetSafeNormal2D();
			Projectile.Definition.SpeedCmPerSecond = Intent.ProjectileSpeedCmPerSecond;
			Projectile.Definition.MaxRangeCm = Intent.LengthCm;
			Projectile.Attack = Intent.Attack;
			Projectile.Damage = Intent.RawDamage;
			Projectile.CollisionRadiusCm = FMath::Max(10.0f, Intent.WidthCm * 0.5f);
			if (FReEchoEnemyProjectileLogic::Initialize(Projectile.Definition, Projectile.Snapshot))
			{
				BossProjectiles.Add(MoveTemp(Projectile));
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
	if (Intent.bCanDamageTarget && Intent.Target.IsValid())
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
			ReEchoAttackEffects::SpawnHitImpact(GetWorld(), Intent.HitLocation);
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
		SelfDestruct.SourceLocation = GetActorLocation();
		SelfDestruct.HitLocation = GetActorLocation();
		ReEchoHitResolver::ResolvePhysicalHit(SelfDestruct);
	}
}

FReEchoEnemyPresentationSnapshot AReEchoEnemyActor::BuildPresentationSnapshot(const bool bMoving) const
{
	FReEchoEnemyPresentationSnapshot Result;
	const FReEchoEnemyLogicSnapshot LogicSnapshot = EnemyLogic->GetSnapshot();
	const FReEchoEnemyDefinition& Definition = EnemyLogic->GetDefinition();
	Result.Archetype = LogicSnapshot.Archetype;
	Result.Phase = LogicSnapshot.Phase;
	Result.AppearanceId = LogicSnapshot.SpawnIndex;
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
	if (Event.Target != this)
	{
		return;
	}
	SetActorEnableCollision(false);
	SetLifeSpan(0.45f);
}
