#include "Enemies/ReEchoEnemyLogicComponent.h"

#include "Combat/ReEchoCombatTarget.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "GameFramework/Actor.h"

namespace
{
constexpr float MinimumFuseDurationSeconds = 0.1f;
constexpr float BossFixedDeltaSeconds = 1.0f / 60.0f;

/** Idle-wander tuning. Wander speed is a fraction of the enemy's pursuit speed; direction re-rolls every period. */
constexpr float IdleWanderSpeedFraction = 0.35f;
constexpr float IdleWanderPeriodSeconds = 3.0f;
constexpr float IdleWanderDirectionJitter = 0.78539816339744831f; // +/-45 degrees

FReEchoEnemyActionIntent ApplyMovementPermit(const FReEchoEnemySenseSnapshot& Sense, FReEchoEnemyActionIntent Intent)
{
	if (!Sense.bMovementPermitted)
	{
		Intent.MovementDelta = FVector::ZeroVector;
		Intent.bHasMovement = false;
		Intent.bSpecialDashMovement = false;
		for (FReEchoBossIntent& BossIntent : Intent.BossIntents)
		{
			BossIntent.bRequestTeleport = false;
			BossIntent.TeleportDestination = FVector::ZeroVector;
		}
	}
	return Intent;
}

/**
 * Deterministic wander heading. Uses only WorldTime + SpawnIndex so the same sample produces the same
 * movement across runs, saves, and replay (no FMath::Rand —录像/回放 must stay bit-identical).
 */
FVector MakeIdleWanderDirection(const FReEchoEnemySenseSnapshot& Sense, const int32 SpawnIndex, const float Elapsed)
{
	const double Phase = static_cast<double>(Sense.WorldTimeSeconds) * 0.36787944117149756 +
	                     static_cast<double>(SpawnIndex) * 2.3999632297286531 + Elapsed * 1.1314029747174113;
	const double Angle = FMath::Fmod(Phase, 2.0 * PI);
	return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
}

const FName BossMeleeSweepBehaviorId(TEXT("Boss.MeleeSweep"));
const FName BossProjectileBehaviorId(TEXT("Boss.Projectile"));
const FName BossBlinkSlamBehaviorId(TEXT("Boss.BlinkSlam"));
const FName BossBlinkSlamComboBehaviorId(TEXT("Boss.BlinkSlamCombo"));
const FName BossPrayerBeamBehaviorId(TEXT("Boss.PrayerBeam"));
const FName BossElementCleanseBehaviorId(TEXT("Boss.ElementCleanse"));

EReEchoBossAbilityKind ResolveBossAbilityKind(const FName BehaviorId)
{
	if (BehaviorId == BossMeleeSweepBehaviorId)
	{
		return EReEchoBossAbilityKind::MeleeSweep;
	}
	if (BehaviorId == BossProjectileBehaviorId)
	{
		return EReEchoBossAbilityKind::Projectile;
	}
	if (BehaviorId == BossBlinkSlamBehaviorId)
	{
		return EReEchoBossAbilityKind::BlinkSlam;
	}
	if (BehaviorId == BossBlinkSlamComboBehaviorId)
	{
		return EReEchoBossAbilityKind::BlinkSlamMoving;
	}
	if (BehaviorId == BossPrayerBeamBehaviorId)
	{
		return EReEchoBossAbilityKind::PrayerBeam;
	}
	if (BehaviorId == BossElementCleanseBehaviorId)
	{
		return EReEchoBossAbilityKind::ElementCleanse;
	}
	return EReEchoBossAbilityKind::None;
}

EReEchoBossAttackShape ResolveBossAttackShape(const EReEchoBossAbilityKind AbilityKind)
{
	switch (AbilityKind)
	{
		case EReEchoBossAbilityKind::MeleeSweep:
			return EReEchoBossAttackShape::Rectangle;
		case EReEchoBossAbilityKind::Projectile:
			return EReEchoBossAttackShape::Projectile;
		case EReEchoBossAbilityKind::BlinkSlam:
		case EReEchoBossAbilityKind::BlinkSlamMoving:
			return EReEchoBossAttackShape::Circle;
		case EReEchoBossAbilityKind::PrayerBeam:
			return EReEchoBossAttackShape::Beam;
		default:
			return EReEchoBossAttackShape::None;
	}
}

bool IsActiveBossAbility(const EReEchoBossAbilityKind AbilityKind)
{
	return AbilityKind == EReEchoBossAbilityKind::MeleeSweep || AbilityKind == EReEchoBossAbilityKind::Projectile ||
	       AbilityKind == EReEchoBossAbilityKind::BlinkSlam || AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving ||
	       AbilityKind == EReEchoBossAbilityKind::PrayerBeam;
}
}

FReEchoEnemyDefinition ReEchoEnemyDefinitions::MakeLegacyEquivalent(const EReEchoEnemyArchetype Archetype,
                                                                    const float BomberTriggerRadiusCm,
                                                                    const float BomberDamageRadiusCm,
                                                                    const float BomberFuseDurationSeconds,
                                                                    const float BomberDamage)
{
	FReEchoEnemyDefinition Result;
	Result.Archetype = Archetype;
	switch (Archetype)
	{
		case EReEchoEnemyArchetype::Shield:
			Result.PresentationId = TEXT("Enemy.Shield");
			break;
		case EReEchoEnemyArchetype::Bomber:
			Result.PresentationId = TEXT("Enemy.Bomber");
			break;
		case EReEchoEnemyArchetype::Boss:
			Result.PresentationId = TEXT("Enemy.TimeGuard");
			break;
		case EReEchoEnemyArchetype::Slime:
			Result.PresentationId = TEXT("Enemy.Slime");
			break;
		case EReEchoEnemyArchetype::Ranged:
			Result.PresentationId = TEXT("Enemy.Rabbit");
			break;
		case EReEchoEnemyArchetype::Elite:
			Result.PresentationId = TEXT("Enemy.Fox");
			break;
		default:
			Result.PresentationId = TEXT("Enemy.Grunt");
			break;
	}
	Result.BomberTriggerRadiusCm = FMath::Max(0.0f, BomberTriggerRadiusCm);
	Result.BomberDamageRadiusCm = FMath::Max(0.0f, BomberDamageRadiusCm);
	Result.BomberFuseDurationSeconds = FMath::Max(MinimumFuseDurationSeconds, BomberFuseDurationSeconds);

	switch (Archetype)
	{
		case EReEchoEnemyArchetype::Shield:
			Result.MaxHealth = 55.0f;
			Result.MoveSpeedCmPerSecond = 50.0f;
			Result.ContactDamage = 14.0f;
			Result.AttackIntervalSeconds = 1.7f;
			Result.bUsesDirectionalShield = true;
			break;
		case EReEchoEnemyArchetype::Bomber:
			Result.MaxHealth = 16.0f;
			Result.MoveSpeedCmPerSecond = 175.0f;
			Result.ContactDamage = FMath::Max(0.0f, BomberDamage);
			break;
		case EReEchoEnemyArchetype::Boss:
			Result.MaxHealth = 650.0f;
			Result.MoveSpeedCmPerSecond = 45.0f;
			Result.ContactDamage = 18.0f;
			Result.AttackIntervalSeconds = 2.0f;
			Result.KnockbackSpeedCmPerSecond = 140.0f;
			Result.HateRangeCm = 0.0f;
			break;
		default:
			break;
	}
	return Result;
}

UReEchoEnemyLogicComponent::UReEchoEnemyLogicComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UReEchoEnemyLogicComponent::Initialize(const FReEchoEnemyDefinition& InDefinition, const int32 InSpawnIndex)
{
	bInitialized = false;
	SpecialActiveAbilityIndices.Reset();
	BossActiveAbilityIndices.Reset();
	BossPhaseIndices.Reset();
	BossCleanseAbilityIndex = INDEX_NONE;
	DebugQueuedBossAbilityId = NAME_None;
	DebugForcedComboCount = 0;
	if (InDefinition.MaxHealth <= 0.0f || InDefinition.MoveSpeedCmPerSecond < 0.0f ||
	    InDefinition.CollisionRadiusCm <= 0.0f || InDefinition.CollisionHalfHeightCm <= 0.0f ||
	    InDefinition.ContactDamage < 0.0f || InDefinition.AttackIntervalSeconds < 0.0f ||
	    InDefinition.ContactRangeCm < 0.0f || InDefinition.MovementStopDistanceCm < 0.0f)
	{
		return false;
	}

	Definition = InDefinition;
	Definition.BomberTriggerRadiusCm = FMath::Max(0.0f, Definition.BomberTriggerRadiusCm);
	Definition.BomberDamageRadiusCm = FMath::Max(0.0f, Definition.BomberDamageRadiusCm);
	Definition.BomberFuseDurationSeconds = FMath::Max(MinimumFuseDurationSeconds, Definition.BomberFuseDurationSeconds);
	Definition.HitReactionDurationSeconds = FMath::Max(0.0f, Definition.HitReactionDurationSeconds);
	Definition.KnockbackSpeedCmPerSecond = FMath::Max(0.0f, Definition.KnockbackSpeedCmPerSecond);
	Definition.KnockbackDrag = FMath::Max(0.0f, Definition.KnockbackDrag);
	const bool bHasHealthThresholdTrigger =
	    Definition.Phase2.TriggerMode == EReEchoEnemyPhase2TriggerMode::HealthThreshold &&
	    Definition.Phase2.HealthThresholdRatio >= 0.0f && Definition.Phase2.HealthThresholdRatio <= 1.0f;
	const bool bHasLegacyPhaseTrigger =
	    Definition.Phase2.TriggerRangeCm > 0.0f || Definition.Phase2.RequiredAttackCount > 0;
	if (Definition.Phase2.bEnabled && (Definition.Phase2.Id.IsNone() || Definition.Phase2.TransformSeconds < 0.0f ||
	                                   (!bHasHealthThresholdTrigger && !bHasLegacyPhaseTrigger)))
	{
		Definition = {};
		State = {};
		return false;
	}

	State = {};
	State.Archetype = Definition.Archetype;
	State.SpawnIndex = InSpawnIndex;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	State.bAlive = true;
	if ((Definition.Archetype == EReEchoEnemyArchetype::Ranged ||
	     Definition.Archetype == EReEchoEnemyArchetype::Elite) &&
	    !BuildSpecialRuntime())
	{
		Definition = {};
		State = {};
		return false;
	}
	if (Definition.Archetype == EReEchoEnemyArchetype::Boss && !BuildBossRuntime())
	{
		Definition = {};
		State = {};
		return false;
	}
	bInitialized = true;
	return true;
}

bool UReEchoEnemyLogicComponent::BuildSpecialRuntime()
{
	SpecialActiveAbilityIndices.Reset();
	const FName RequiredBehavior = Definition.Archetype == EReEchoEnemyArchetype::Ranged
	                                   ? FName(TEXT("Enemy.RangedBurst"))
	                                   : FName(TEXT("Enemy.EliteDash"));
	for (int32 AbilityIndex = 0; AbilityIndex < Definition.Abilities.Num(); ++AbilityIndex)
	{
		const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
		if (Ability.bEnabled && Ability.BehaviorId == RequiredBehavior && !Ability.Id.IsNone())
		{
			SpecialActiveAbilityIndices.Add(AbilityIndex);
		}
	}
	SpecialActiveAbilityIndices.Sort(
	    [this](const int32 LeftIndex, const int32 RightIndex)
	    {
		    const FReEchoEnemyAbilityDefinition& Left = Definition.Abilities[LeftIndex];
		    const FReEchoEnemyAbilityDefinition& Right = Definition.Abilities[RightIndex];
		    return Left.SequenceOrder == Right.SequenceOrder ? Left.Id.LexicalLess(Right.Id)
		                                                     : Left.SequenceOrder < Right.SequenceOrder;
	    });
	return !SpecialActiveAbilityIndices.IsEmpty();
}

bool UReEchoEnemyLogicComponent::BuildBossRuntime()
{
	BossActiveAbilityIndices.Reset();
	BossPhaseIndices.Reset();
	BossCleanseAbilityIndex = INDEX_NONE;
	State.BossAbilityCooldowns.Reset();

	TSet<FName> AbilityIds;
	bool bHasMeleeSweep = false;
	bool bHasProjectile = false;
	bool bHasBlinkSlam = false;
	bool bHasPrayerBeam = false;
	for (int32 AbilityIndex = 0; AbilityIndex < Definition.Abilities.Num(); ++AbilityIndex)
	{
		const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
		if (!Ability.bEnabled)
		{
			continue;
		}
		if (Ability.Id.IsNone() || AbilityIds.Contains(Ability.Id))
		{
			return false;
		}
		AbilityIds.Add(Ability.Id);

		const EReEchoBossAbilityKind AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
		if (AbilityKind == EReEchoBossAbilityKind::ElementCleanse)
		{
			if (BossCleanseAbilityIndex != INDEX_NONE || Ability.CleanseIntervalSeconds <= 0.0f ||
			    Ability.ImmunitySeconds < 0.0f)
			{
				return false;
			}
			BossCleanseAbilityIndex = AbilityIndex;
			continue;
		}
		if (!IsActiveBossAbility(AbilityKind) || Ability.Damage < 0.0f || Ability.WindupSeconds < 0.0f ||
		    Ability.ActiveSeconds < 0.0f || Ability.RecoverySeconds < 0.0f || Ability.CooldownSeconds < 0.0f ||
		    Ability.MinRangeCm < 0.0f || Ability.MaxRangeCm < Ability.MinRangeCm || Ability.MaxRangeCm <= 0.0f ||
		    Ability.MinPhaseIndex < 1 || Ability.MaxPhaseIndex < Ability.MinPhaseIndex || Ability.ComboMin < 1 ||
		    Ability.ComboMax < Ability.ComboMin)
		{
			return false;
		}
		if (AbilityKind == EReEchoBossAbilityKind::Projectile && Ability.ProjectileSpeedCmPerSecond <= 0.0f)
		{
			return false;
		}
		if ((AbilityKind == EReEchoBossAbilityKind::BlinkSlam ||
		     AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving) &&
		    (Ability.TeleportOffsetCm <= 0.0f || Ability.RadiusCm <= 0.0f))
		{
			return false;
		}

		bHasMeleeSweep |= AbilityKind == EReEchoBossAbilityKind::MeleeSweep;
		bHasProjectile |= AbilityKind == EReEchoBossAbilityKind::Projectile;
		bHasBlinkSlam |= AbilityKind == EReEchoBossAbilityKind::BlinkSlam;
		bHasPrayerBeam |= AbilityKind == EReEchoBossAbilityKind::PrayerBeam;
		BossActiveAbilityIndices.Add(AbilityIndex);

		FReEchoBossAbilityCooldownSnapshot Cooldown;
		Cooldown.AbilityId = Ability.Id;
		State.BossAbilityCooldowns.Add(Cooldown);
	}
	if (!bHasMeleeSweep || !bHasProjectile || !bHasBlinkSlam || !bHasPrayerBeam)
	{
		return false;
	}

	BossActiveAbilityIndices.Sort(
	    [this](const int32 Left, const int32 Right)
	    {
		    const FReEchoEnemyAbilityDefinition& LeftAbility = Definition.Abilities[Left];
		    const FReEchoEnemyAbilityDefinition& RightAbility = Definition.Abilities[Right];
		    if (LeftAbility.SequenceOrder != RightAbility.SequenceOrder)
		    {
			    return LeftAbility.SequenceOrder < RightAbility.SequenceOrder;
		    }
		    return LeftAbility.Id.LexicalLess(RightAbility.Id);
	    });

	TSet<int32> PhaseNumbers;
	for (int32 PhaseDefinitionIndex = 0; PhaseDefinitionIndex < Definition.BossPhases.Num(); ++PhaseDefinitionIndex)
	{
		const FReEchoBossPhaseDefinition& PhaseDefinition = Definition.BossPhases[PhaseDefinitionIndex];
		if (!PhaseDefinition.bEnabled)
		{
			continue;
		}
		if (PhaseDefinition.Id.IsNone() || PhaseDefinition.PhaseIndex < 0 ||
		    PhaseNumbers.Contains(PhaseDefinition.PhaseIndex) || PhaseDefinition.TriggerSeconds < 0.0f ||
		    PhaseDefinition.PhysicalAttackMultiplier <= 0.0f || PhaseDefinition.ElementalAttackMultiplier <= 0.0f ||
		    PhaseDefinition.AttackSpeedMultiplier <= 0.0f || PhaseDefinition.MovementSpeedMultiplier <= 0.0f)
		{
			return false;
		}
		if (PhaseDefinition.RefillHealthPolicy == EReEchoBossRefillHealthPolicy::RefillToMaximum &&
		    PhaseDefinition.PhaseMaxHealth <= 0.0f)
		{
			return false;
		}
		PhaseNumbers.Add(PhaseDefinition.PhaseIndex);
		BossPhaseIndices.Add(PhaseDefinitionIndex);
	}
	if (BossPhaseIndices.IsEmpty())
	{
		return false;
	}
	BossPhaseIndices.Sort(
	    [this](const int32 Left, const int32 Right)
	    {
		    const FReEchoBossPhaseDefinition& LeftPhase = Definition.BossPhases[Left];
		    const FReEchoBossPhaseDefinition& RightPhase = Definition.BossPhases[Right];
		    if (!FMath::IsNearlyEqual(LeftPhase.TriggerSeconds, RightPhase.TriggerSeconds))
		    {
			    return LeftPhase.TriggerSeconds < RightPhase.TriggerSeconds;
		    }
		    return LeftPhase.PhaseIndex < RightPhase.PhaseIndex;
	    });

	State.BossCleanseRemainingSeconds = BossCleanseAbilityIndex != INDEX_NONE
	                                        ? Definition.Abilities[BossCleanseAbilityIndex].CleanseIntervalSeconds
	                                        : 0.0f;
	return true;
}

void UReEchoEnemyLogicComponent::BindEventSources(UReEchoEnemyEventsComponent* InEnemyEvents,
                                                  UReEchoCombatEventsComponent* InCombatEvents)
{
	if (CombatEvents)
	{
		CombatEvents->OnHurt.RemoveAll(this);
		CombatEvents->OnDeath.RemoveAll(this);
	}

	EnemyEvents = InEnemyEvents;
	CombatEvents = InCombatEvents;
	if (CombatEvents)
	{
		CombatEvents->OnHurt.AddDynamic(this, &UReEchoEnemyLogicComponent::HandleCombatHurt);
		CombatEvents->OnDeath.AddDynamic(this, &UReEchoEnemyLogicComponent::HandleCombatDeath);
	}
}

void UReEchoEnemyLogicComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindEventSources(nullptr, nullptr);
	Super::EndPlay(EndPlayReason);
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::Advance(const FReEchoEnemySenseSnapshot& Sense,
                                                             const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	if (!bInitialized || !State.bAlive)
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Dead;
		return Intent;
	}
	if (State.bSelfDestructCommitted)
	{
		return Intent;
	}

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	if (State.Phase == EReEchoEnemyBehaviorPhase::Transforming)
	{
		return ApplyMovementPermit(Sense, AdvancePhaseTransition(SafeDeltaSeconds));
	}
	if (TryBeginPhaseTransition(Sense, Intent))
	{
		return Intent;
	}
	if (Definition.Archetype == EReEchoEnemyArchetype::Boss)
	{
		return ApplyMovementPermit(Sense, AdvanceBoss(Sense, SafeDeltaSeconds));
	}
	if (State.HitReactionRemainingSeconds > 0.0f)
	{
		return ApplyMovementPermit(Sense, AdvanceHitReaction(SafeDeltaSeconds));
	}
	if (Definition.Archetype == EReEchoEnemyArchetype::Ranged || Definition.Archetype == EReEchoEnemyArchetype::Elite)
	{
		return ApplyMovementPermit(Sense, AdvanceSpecial(Sense, SafeDeltaSeconds));
	}

	const bool bHasLiveTarget = Sense.bTargetExists && Sense.bTargetAlive;
	const bool bAggroEnabled = Sense.HateRangeCm > 0.0f;
	const bool bInAggroRange = bAggroEnabled && Sense.bInCombat;
	if (!bHasLiveTarget || (bAggroEnabled && !bInAggroRange))
	{
		if (!State.bHasEngaged)
		{
			return ApplyMovementPermit(Sense, AdvanceIdleWander(Sense, SafeDeltaSeconds));
		}
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		return Intent;
	}
	State.bHasEngaged = true;
	State.AttackCooldownRemainingSeconds = FMath::Max(0.0f, State.AttackCooldownRemainingSeconds - SafeDeltaSeconds);

	FVector ToTarget = Sense.TargetLocation - Sense.SelfLocation;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();
	const FVector Direction = ToTarget.GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		State.FacingDirection = Direction;
		Intent.FacingDirection = Direction;
		Intent.bHasFacing = true;
	}
	if (Distance > Definition.MovementStopDistanceCm && !Direction.IsNearlyZero())
	{
		Intent.MovementDelta = Direction * Definition.MoveSpeedCmPerSecond * SafeDeltaSeconds;
		Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
		State.Phase = EReEchoEnemyBehaviorPhase::Pursuing;
	}
	else
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	}

	if (Definition.Archetype == EReEchoEnemyArchetype::Bomber)
	{
		if (!State.bFuseActive && Distance <= Definition.BomberTriggerRadiusCm)
		{
			State.bFuseActive = true;
			State.FuseRemainingSeconds = Definition.BomberFuseDurationSeconds;
			PublishFuse(true);
		}
		if (State.bFuseActive)
		{
			State.Phase = EReEchoEnemyBehaviorPhase::Fuse;
			State.FuseRemainingSeconds = FMath::Max(0.0f, State.FuseRemainingSeconds - SafeDeltaSeconds);
			PublishFuse(false);
			if (State.FuseRemainingSeconds <= 0.0f && Sense.bAttackPermitted)
			{
				CommitAttack(
				    Sense, Distance <= Definition.BomberDamageRadiusCm && !Sense.bTargetInvulnerable, true, Intent);
				return ApplyMovementPermit(Sense, MoveTemp(Intent));
			}
		}
		return ApplyMovementPermit(Sense, MoveTemp(Intent));
	}

	if (Sense.bAttackPermitted && Distance <= Definition.ContactRangeCm && State.AttackCooldownRemainingSeconds <= 0.0f)
	{
		CommitAttack(Sense, !Sense.bTargetInvulnerable, false, Intent);
	}
	return ApplyMovementPermit(Sense, MoveTemp(Intent));
}

const FReEchoEnemyAbilityDefinition* UReEchoEnemyLogicComponent::GetNextSpecialAbility() const
{
	if (SpecialActiveAbilityIndices.IsEmpty())
	{
		return nullptr;
	}
	const int32 SequenceIndex = FMath::Clamp(State.SpecialNextSequenceIndex, 0, SpecialActiveAbilityIndices.Num() - 1);
	return &Definition.Abilities[SpecialActiveAbilityIndices[SequenceIndex]];
}

const FReEchoEnemyAbilityDefinition* UReEchoEnemyLogicComponent::FindSpecialAbility(const FName AbilityId) const
{
	const int32* AbilityIndex = SpecialActiveAbilityIndices.FindByPredicate(
	    [this, AbilityId](const int32 CandidateIndex)
	    {
		    return Definition.Abilities[CandidateIndex].Id == AbilityId;
	    });
	return AbilityIndex ? &Definition.Abilities[*AbilityIndex] : nullptr;
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvanceSpecial(const FReEchoEnemySenseSnapshot& Sense,
                                                                    const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	const FReEchoEnemyAbilityDefinition* Ability = State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None
	                                                   ? GetNextSpecialAbility()
	                                                   : FindSpecialAbility(State.SpecialAbilityId);
	if (!Ability)
	{
		ClearSpecialAction();
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		return Intent;
	}

	State.AttackCooldownRemainingSeconds = FMath::Max(0.0f, State.AttackCooldownRemainingSeconds - DeltaSeconds);
	if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Active)
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Attacking;
		State.FacingDirection = State.SpecialLockedDirection;
		Intent.FacingDirection = State.SpecialLockedDirection;
		Intent.bHasFacing = !Intent.FacingDirection.IsNearlyZero();
		Intent.Attack = State.SpecialAttack;
		Intent.Target = Sense.Target;
		Intent.RawDamage = Ability->Damage;
		Intent.DamageRadiusCm = FMath::Max(0.0f, Ability->WidthCm * 0.5f);
		Intent.SourceLocation = Sense.SelfLocation;
		Intent.HitLocation = Sense.TargetLocation;
		Intent.bCanDamageTarget =
		    !State.bSpecialDamageConsumed && Sense.bTargetExists && Sense.bTargetAlive && !Sense.bTargetInvulnerable;
		Intent.bSpecialDashMovement = true;

		const float StepSeconds = FMath::Min(DeltaSeconds, State.SpecialActionRemainingSeconds);
		float StepDistanceCm = 0.0f;
		if (StepSeconds > 0.0f && Ability->ActiveSeconds > KINDA_SMALL_NUMBER)
		{
			StepDistanceCm = Ability->LengthCm * StepSeconds / Ability->ActiveSeconds;
			if (StepSeconds + KINDA_SMALL_NUMBER >= State.SpecialActionRemainingSeconds)
			{
				StepDistanceCm = State.SpecialDashRemainingDistanceCm;
			}
			StepDistanceCm = FMath::Min(FMath::Max(0.0f, StepDistanceCm), State.SpecialDashRemainingDistanceCm);
		}
		Intent.MovementDelta = State.SpecialLockedDirection * StepDistanceCm;
		Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
		State.SpecialActionRemainingSeconds = FMath::Max(0.0f, State.SpecialActionRemainingSeconds - StepSeconds);
		State.SpecialDashRemainingDistanceCm = FMath::Max(0.0f, State.SpecialDashRemainingDistanceCm - StepDistanceCm);
		if (State.SpecialActionRemainingSeconds <= KINDA_SMALL_NUMBER ||
		    State.SpecialDashRemainingDistanceCm <= KINDA_SMALL_NUMBER)
		{
			BeginSpecialRecovery(*Ability);
		}
		return Intent;
	}

	if ((!Sense.bTargetExists || !Sense.bTargetAlive) &&
	    State.SpecialActionPhase != EReEchoEnemySpecialActionPhase::Recovery)
	{
		ClearSpecialAction();
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		return Intent;
	}

	FVector ToTargetVec = Sense.TargetLocation - Sense.SelfLocation;
	ToTargetVec.Z = 0.0f;
	const float Distance = ToTargetVec.Size();
	const FVector Direction = ToTargetVec.GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		State.FacingDirection = Direction;
		Intent.FacingDirection = Direction;
		Intent.bHasFacing = true;
	}

	if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None)
	{
		if (Sense.bAttackPermitted && Sense.bSpecialActionPermitted && State.AttackCooldownRemainingSeconds <= 0.0f &&
		    Distance >= Ability->MinRangeCm && Distance <= Ability->MaxRangeCm)
		{
			State.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Windup;
			State.SpecialAbilityId = Ability->Id;
			State.SpecialActionRemainingSeconds = Ability->WindupSeconds;
			State.SpecialLockedTargetLocation = Sense.TargetLocation;
			State.SpecialLockedDirection = Direction.IsNearlyZero() ? State.FacingDirection : Direction;
			State.SpecialNextSequenceIndex = (State.SpecialNextSequenceIndex + 1) % SpecialActiveAbilityIndices.Num();
			State.Phase = EReEchoEnemyBehaviorPhase::Attacking;
			return Intent;
		}
		if (Distance > Definition.MovementStopDistanceCm && !Direction.IsNearlyZero())
		{
			Intent.MovementDelta = Direction * Definition.MoveSpeedCmPerSecond * DeltaSeconds;
			Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
			State.Phase = EReEchoEnemyBehaviorPhase::Pursuing;
		}
		return Intent;
	}

	State.SpecialActionRemainingSeconds = FMath::Max(0.0f, State.SpecialActionRemainingSeconds - DeltaSeconds);
	State.Phase = EReEchoEnemyBehaviorPhase::Attacking;
	if (State.SpecialActionRemainingSeconds > 0.0f)
	{
		// During the recovery of a move-while-casting ability, keep pursuing the target instead of freezing.
		if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Recovery)
		{
			const FReEchoEnemyAbilityDefinition* SpecialAbility = FindSpecialAbility(State.SpecialAbilityId);
			if (SpecialAbility && SpecialAbility->bMovementDuringCast)
			{
				const FVector TowardTarget = (Sense.TargetLocation - Sense.SelfLocation).GetSafeNormal2D();
				Intent.MovementDelta = TowardTarget * Definition.MoveSpeedCmPerSecond * DeltaSeconds;
				Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
				return Intent;
			}
		}
		return Intent;
	}
	if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Windup)
	{
		if (!Sense.bAttackPermitted)
		{
			return Intent;
		}
		if (Definition.Archetype == EReEchoEnemyArchetype::Ranged)
		{
			const bool bCanDamage = FVector::DistSquared2D(Sense.TargetLocation, State.SpecialLockedTargetLocation) <=
			                        FMath::Square(Ability->RadiusCm);
			CommitAttack(Sense, bCanDamage && !Sense.bTargetInvulnerable, false, Intent);
			Intent.RawDamage = Ability->Damage;
			Intent.HitLocation = State.SpecialLockedTargetLocation;
			State.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Recovery;
			State.SpecialActionRemainingSeconds = Ability->ActiveSeconds + Ability->RecoverySeconds;
		}
		else
		{
			CommitAttack(Sense, false, false, Intent);
			Intent.RawDamage = Ability->Damage;
			State.SpecialAttack = Intent.Attack;
			State.SpecialDashRemainingDistanceCm = FMath::Max(0.0f, Ability->LengthCm);
			State.bSpecialDamageConsumed = false;
			State.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Active;
			State.SpecialActionRemainingSeconds = FMath::Max(0.0f, Ability->ActiveSeconds);
			if (State.SpecialActionRemainingSeconds <= KINDA_SMALL_NUMBER ||
			    State.SpecialDashRemainingDistanceCm <= KINDA_SMALL_NUMBER)
			{
				BeginSpecialRecovery(*Ability);
			}
		}
		State.AttackCooldownRemainingSeconds = Ability->CooldownSeconds;
		return Intent;
	}

	ClearSpecialAction();
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	return Intent;
}

void UReEchoEnemyLogicComponent::BeginSpecialRecovery(const FReEchoEnemyAbilityDefinition& Ability)
{
	State.SpecialActionPhase = EReEchoEnemySpecialActionPhase::Recovery;
	State.SpecialActionRemainingSeconds = FMath::Max(0.0f, Ability.RecoverySeconds);
	State.SpecialDashRemainingDistanceCm = 0.0f;
}

void UReEchoEnemyLogicComponent::ClearSpecialAction()
{
	State.SpecialActionPhase = EReEchoEnemySpecialActionPhase::None;
	State.SpecialAbilityId = NAME_None;
	State.SpecialActionRemainingSeconds = 0.0f;
	State.SpecialLockedTargetLocation = FVector::ZeroVector;
	State.SpecialLockedDirection = State.FacingDirection.GetSafeNormal2D();
	State.SpecialDashRemainingDistanceCm = 0.0f;
	State.SpecialAttack = {};
	State.bSpecialDamageConsumed = false;
}

void UReEchoEnemyLogicComponent::CancelSpecialAction()
{
	if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None)
	{
		return;
	}
	if (EnemyEvents)
	{
		FReEchoEnemySpecialActionEvent Event;
		Event.AbilityId = State.SpecialAbilityId;
		Event.Type = EReEchoEnemySpecialActionEventType::ActionCancelled;
		Event.Attack = State.SpecialAttack;
		Event.Origin = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		Event.LockedDirection = State.SpecialLockedDirection;
		Event.LockedTargetLocation = State.SpecialLockedTargetLocation;
		EnemyEvents->PublishSpecialAction(Event);
	}
	ClearSpecialAction();
}

void UReEchoEnemyLogicComponent::ResolveSpecialDashStep(const bool bMovementBlocked, const bool bDamageContactConsumed)
{
	if (!bInitialized || Definition.Archetype != EReEchoEnemyArchetype::Elite)
	{
		return;
	}
	State.bSpecialDamageConsumed = State.bSpecialDamageConsumed || bDamageContactConsumed;
	if (bMovementBlocked && State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Active)
	{
		if (const FReEchoEnemyAbilityDefinition* Ability = FindSpecialAbility(State.SpecialAbilityId))
		{
			BeginSpecialRecovery(*Ability);
		}
		else
		{
			ClearSpecialAction();
		}
	}
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvanceBoss(const FReEchoEnemySenseSnapshot& Sense,
                                                                 const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	State.BossSimulationAccumulatorSeconds += DeltaSeconds;
	while (State.BossSimulationAccumulatorSeconds + KINDA_SMALL_NUMBER >= BossFixedDeltaSeconds)
	{
		State.BossSimulationAccumulatorSeconds =
		    FMath::Max(0.0f, State.BossSimulationAccumulatorSeconds - BossFixedDeltaSeconds);
		AdvanceBossFixedStep(Sense, BossFixedDeltaSeconds, Intent);
	}
	return Intent;
}

void UReEchoEnemyLogicComponent::AdvanceBossFixedStep(const FReEchoEnemySenseSnapshot& Sense,
                                                      const float FixedDeltaSeconds,
                                                      FReEchoEnemyActionIntent& InOutIntent)
{
	AdvanceBossAmbientTimers(Sense, FixedDeltaSeconds, InOutIntent);

	const bool bHadHitReaction = State.HitReactionRemainingSeconds > 0.0f;
	if (bHadHitReaction)
	{
		ApplyBossHitReaction(FixedDeltaSeconds, InOutIntent);
	}

	bool bAbilityOccupiedStep = State.BossActionPhase != EReEchoBossActionPhase::None;
	if (!bAbilityOccupiedStep && !bHadHitReaction && Sense.bAttackPermitted)
	{
		const int32 AbilityIndex = SelectBossAbility(Sense);
		if (AbilityIndex != INDEX_NONE)
		{
			BeginBossAbility(Sense, AbilityIndex, InOutIntent);
			bAbilityOccupiedStep = true;
		}
	}
	if (State.BossActionPhase != EReEchoBossActionPhase::None)
	{
		AdvanceBossAbility(Sense, FixedDeltaSeconds, InOutIntent);
		InOutIntent.FacingDirection = State.BossLockedDirection;
		InOutIntent.bHasFacing = !State.BossLockedDirection.IsNearlyZero();
		if (!bHadHitReaction)
		{
			switch (State.BossActionPhase)
			{
				case EReEchoBossActionPhase::Windup:
					State.Phase = EReEchoEnemyBehaviorPhase::BossWindup;
					break;
				case EReEchoBossActionPhase::Active:
					State.Phase = EReEchoEnemyBehaviorPhase::BossActive;
					break;
				case EReEchoBossActionPhase::Recovery:
					State.Phase = EReEchoEnemyBehaviorPhase::BossRecovery;
					break;
				default:
					break;
			}
		}
	}
	else if (!bAbilityOccupiedStep && !bHadHitReaction)
	{
		ApplyStandardMovement(Sense, FixedDeltaSeconds, InOutIntent);
	}
}

void UReEchoEnemyLogicComponent::AdvanceBossAmbientTimers(const FReEchoEnemySenseSnapshot& Sense,
                                                          const float FixedDeltaSeconds,
                                                          FReEchoEnemyActionIntent& InOutIntent)
{
	for (FReEchoBossAbilityCooldownSnapshot& Cooldown : State.BossAbilityCooldowns)
	{
		Cooldown.RemainingSeconds = FMath::Max(0.0f, Cooldown.RemainingSeconds - FixedDeltaSeconds);
	}

	// The hidden Phase3 deadline measures the playable Phase1+Phase2 fight. Born keeps attack permission disabled,
	// so its non-interactive presentation time must not consume the 15-second fast-kill window. Phase2 transform keeps
	// attack permission enabled and therefore remains part of the cumulative time instead of resetting the clock.
	if (Sense.bAttackPermitted)
	{
		State.BossEncounterElapsedSeconds += FixedDeltaSeconds;
	}
	while (State.BossNextPhaseIndex < BossPhaseIndices.Num())
	{
		const FReEchoBossPhaseDefinition& PhaseDefinition =
		    Definition.BossPhases[BossPhaseIndices[State.BossNextPhaseIndex]];
		// Phase3 uses TriggerSeconds as a fatal-wound deadline, never as an ambient time trigger.
		if (PhaseDefinition.PhaseIndex >= 3)
		{
			++State.BossNextPhaseIndex;
			continue;
		}
		if (State.BossEncounterElapsedSeconds + KINDA_SMALL_NUMBER < PhaseDefinition.TriggerSeconds)
		{
			break;
		}
		FReEchoBossIntent BossIntent;
		BossIntent.Type = EReEchoBossIntentType::EncounterPhase;
		BossIntent.PhaseDefinition = PhaseDefinition;
		AppendBossIntent(MoveTemp(BossIntent), InOutIntent);
		++State.BossNextPhaseIndex;
	}

	if (BossCleanseAbilityIndex != INDEX_NONE)
	{
		const FReEchoEnemyAbilityDefinition& CleanseAbility = Definition.Abilities[BossCleanseAbilityIndex];
		State.BossCleanseRemainingSeconds -= FixedDeltaSeconds;
		while (State.BossCleanseRemainingSeconds <= KINDA_SMALL_NUMBER)
		{
			FReEchoBossIntent BossIntent;
			BossIntent.Type = EReEchoBossIntentType::ElementCleanse;
			BossIntent.AbilityKind = EReEchoBossAbilityKind::ElementCleanse;
			BossIntent.AbilityId = CleanseAbility.Id;
			BossIntent.BehaviorId = CleanseAbility.BehaviorId;
			BossIntent.ElementImmunitySeconds = CleanseAbility.ImmunitySeconds;
			AppendBossIntent(MoveTemp(BossIntent), InOutIntent);
			State.BossCleanseRemainingSeconds += CleanseAbility.CleanseIntervalSeconds;
		}
	}
}

void UReEchoEnemyLogicComponent::AdvanceBossAbility(const FReEchoEnemySenseSnapshot& Sense,
                                                    const float FixedDeltaSeconds,
                                                    FReEchoEnemyActionIntent& InOutIntent)
{
	const int32 AbilityIndex = FindBossAbilityIndex(State.BossCurrentAbilityId);
	if (!Definition.Abilities.IsValidIndex(AbilityIndex))
	{
		State.BossActionPhase = EReEchoBossActionPhase::None;
		State.BossCurrentAbilityId = NAME_None;
		State.BossActionPhaseRemainingSeconds = 0.0f;
		return;
	}
	const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
	State.BossActionPhaseRemainingSeconds = FMath::Max(0.0f, State.BossActionPhaseRemainingSeconds - FixedDeltaSeconds);
	if (State.BossActionPhaseRemainingSeconds > KINDA_SMALL_NUMBER)
	{
		return;
	}

	switch (State.BossActionPhase)
	{
		case EReEchoBossActionPhase::Windup:
			if (!Sense.bAttackPermitted)
			{
				return;
			}
			if (Ability.LockTiming == EReEchoBossLockTiming::WindupEnded)
			{
				LockBossTarget(Sense);
			}
			CommitBossAbility(Sense, Ability, InOutIntent);
			State.BossActionPhase = EReEchoBossActionPhase::Active;
			State.BossActionPhaseRemainingSeconds = Ability.ActiveSeconds;
			State.Phase = EReEchoEnemyBehaviorPhase::BossActive;
			break;
		case EReEchoBossActionPhase::Active:
			if (IsBossComboAbility(Ability) && State.BossComboStrikeIndex < State.BossComboStrikeCount)
			{
				BeginNextBossComboStrike(Sense, Ability, InOutIntent);
			}
			else
			{
				State.BossActionPhase = EReEchoBossActionPhase::Recovery;
				State.BossActionPhaseRemainingSeconds = Ability.RecoverySeconds;
				State.Phase = EReEchoEnemyBehaviorPhase::BossRecovery;
			}
			break;
		case EReEchoBossActionPhase::Recovery:
			EndBossAbility(Ability, InOutIntent);
			break;
		default:
			break;
	}
}

void UReEchoEnemyLogicComponent::BeginBossAbility(const FReEchoEnemySenseSnapshot& Sense,
                                                  const int32 AbilityIndex,
                                                  FReEchoEnemyActionIntent& InOutIntent)
{
	const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
	State.BossCurrentAbilityId = Ability.Id;
	State.BossActionPhase = EReEchoBossActionPhase::Windup;
	State.BossActionPhaseRemainingSeconds = Ability.WindupSeconds;
	State.BossCurrentAttackSequence = ++State.AttackSequence;
	const bool bCombo = IsBossComboAbility(Ability);
	const FReEchoBossPhaseDefinition* OpeningPhase = GetCurrentBossPhaseDefinition();
	State.bBossOpeningActive = State.bBossOpeningQueued && OpeningPhase && OpeningPhase->OpeningAbilityId == Ability.Id;
	if (State.bBossOpeningActive)
	{
		State.bBossOpeningQueued = false;
	}
	if (bCombo)
	{
		const int32 DeterministicCount =
		    Ability.ComboMin + static_cast<int32>((State.BossCurrentAttackSequence + State.SpawnIndex) %
		                                          (Ability.ComboMax - Ability.ComboMin + 1));
		State.BossComboStrikeCount = DebugForcedComboCount > 0
		                                 ? FMath::Clamp(DebugForcedComboCount, Ability.ComboMin, Ability.ComboMax)
		                                 : DeterministicCount;
		State.BossComboStrikeIndex = 1;
		if (State.bBossOpeningActive)
		{
			State.BossComboStrikeCount =
			    FMath::Clamp(OpeningPhase->OpeningStrikeCount, Ability.ComboMin, Ability.ComboMax);
		}
	}
	else
	{
		State.BossComboStrikeIndex = 0;
		State.BossComboStrikeCount = 0;
	}
	DebugForcedComboCount = 0;
	State.bBossCurrentAbilityCommitted = false;
	State.bBossHasLockedTarget = false;
	State.bBossHasLockedTeleportDestination = false;

	const int32 SelectedSequenceIndex = BossActiveAbilityIndices.IndexOfByKey(AbilityIndex);
	State.BossNextSequenceIndex =
	    BossActiveAbilityIndices.IsEmpty() ? 0 : (SelectedSequenceIndex + 1) % BossActiveAbilityIndices.Num();
	if (Ability.LockTiming == EReEchoBossLockTiming::WindupStarted)
	{
		LockBossTarget(Sense);
	}

	FReEchoBossIntent BossIntent;
	BossIntent.Type = EReEchoBossIntentType::TelegraphStarted;
	BossIntent.AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
	BossIntent.AttackShape = ResolveBossAttackShape(BossIntent.AbilityKind);
	BossIntent.AbilityId = Ability.Id;
	BossIntent.BehaviorId = Ability.BehaviorId;
	BossIntent.Attack.Source = GetOwner();
	BossIntent.Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
	BossIntent.Attack.Sequence = State.BossCurrentAttackSequence;
	BossIntent.Target = Sense.Target;
	BossIntent.Origin = Sense.SelfLocation;
	BossIntent.LockedTargetLocation = State.BossLockedTargetLocation;
	BossIntent.LockedDirection = State.BossLockedDirection;
	BossIntent.TeleportDestination = State.BossLockedTeleportDestination;
	BossIntent.RadiusCm = Ability.RadiusCm;
	BossIntent.WidthCm = Ability.WidthCm;
	BossIntent.LengthCm = Ability.LengthCm;
	BossIntent.WindupSeconds = Ability.WindupSeconds;
	BossIntent.ActiveSeconds = Ability.ActiveSeconds;
	BossIntent.RecoverySeconds = Ability.RecoverySeconds;
	BossIntent.TargetingMode = Ability.TargetingMode;
	BossIntent.LockTiming = Ability.LockTiming;
	BossIntent.ComboStrikeIndex = State.BossComboStrikeIndex;
	BossIntent.ComboStrikeCount = State.BossComboStrikeCount;
	AppendBossIntent(MoveTemp(BossIntent), InOutIntent);
	State.Phase = EReEchoEnemyBehaviorPhase::BossWindup;
}

void UReEchoEnemyLogicComponent::BeginNextBossComboStrike(const FReEchoEnemySenseSnapshot& Sense,
                                                          const FReEchoEnemyAbilityDefinition& Ability,
                                                          FReEchoEnemyActionIntent& InOutIntent)
{
	++State.BossComboStrikeIndex;
	State.BossCurrentAttackSequence = ++State.AttackSequence;
	State.bBossCurrentAbilityCommitted = false;
	LockBossTarget(Sense);
	State.BossActionPhase = EReEchoBossActionPhase::Windup;
	State.BossActionPhaseRemainingSeconds = Ability.WindupSeconds;
	State.Phase = EReEchoEnemyBehaviorPhase::BossWindup;

	FReEchoBossIntent BossIntent;
	BossIntent.Type = EReEchoBossIntentType::TelegraphStarted;
	BossIntent.AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
	BossIntent.AttackShape = EReEchoBossAttackShape::Circle;
	BossIntent.AbilityId = Ability.Id;
	BossIntent.BehaviorId = Ability.BehaviorId;
	BossIntent.Attack.Source = GetOwner();
	BossIntent.Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
	BossIntent.Attack.Sequence = State.BossCurrentAttackSequence;
	BossIntent.Target = Sense.Target;
	BossIntent.Origin = Sense.SelfLocation;
	BossIntent.LockedTargetLocation = State.BossLockedTargetLocation;
	BossIntent.LockedDirection = State.BossLockedDirection;
	BossIntent.TeleportDestination = State.BossLockedTeleportDestination;
	BossIntent.RadiusCm = Ability.RadiusCm;
	BossIntent.WindupSeconds = Ability.WindupSeconds;
	BossIntent.ActiveSeconds = Ability.ActiveSeconds;
	BossIntent.RecoverySeconds = Ability.RecoverySeconds;
	BossIntent.TargetingMode = Ability.TargetingMode;
	BossIntent.LockTiming = Ability.LockTiming;
	BossIntent.ComboStrikeIndex = State.BossComboStrikeIndex;
	BossIntent.ComboStrikeCount = State.BossComboStrikeCount;
	AppendBossIntent(MoveTemp(BossIntent), InOutIntent);
}

void UReEchoEnemyLogicComponent::LockBossTarget(const FReEchoEnemySenseSnapshot& Sense)
{
	const int32 CurrentAbilityIndex = FindBossAbilityIndex(State.BossCurrentAbilityId);
	const bool bGrounded = State.bBossOpeningActive || (Definition.Abilities.IsValidIndex(CurrentAbilityIndex) &&
	                                                    Definition.Abilities[CurrentAbilityIndex].bGroundedSlam);
	State.BossLockedTargetLocation = bGrounded ? Sense.SelfLocation : Sense.TargetLocation;
	State.BossLockedDirection = (Sense.TargetLocation - Sense.SelfLocation).GetSafeNormal2D();
	if (State.BossLockedDirection.IsNearlyZero())
	{
		State.BossLockedDirection = State.FacingDirection.GetSafeNormal2D();
	}
	if (State.BossLockedDirection.IsNearlyZero())
	{
		State.BossLockedDirection = FVector::ForwardVector;
	}
	State.BossLockedTeleportDestination = bGrounded ? Sense.SelfLocation : Sense.TeleportDestination;
	State.bBossHasLockedTarget = Sense.bTargetExists && Sense.bTargetAlive;
	State.bBossHasLockedTeleportDestination = !bGrounded && Sense.bHasTeleportDestination;
	State.FacingDirection = State.BossLockedDirection;
}

void UReEchoEnemyLogicComponent::CommitBossAbility(const FReEchoEnemySenseSnapshot& Sense,
                                                   const FReEchoEnemyAbilityDefinition& Ability,
                                                   FReEchoEnemyActionIntent& InOutIntent)
{
	const EReEchoBossAbilityKind AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
	const float ResolvedDamage = ResolveCurrentBossPhysicalDamage(Ability.Damage);
	FReEchoBossIntent BossIntent;
	BossIntent.Type = EReEchoBossIntentType::AttackWindowStarted;
	BossIntent.AbilityKind = AbilityKind;
	BossIntent.AttackShape = ResolveBossAttackShape(AbilityKind);
	BossIntent.AbilityId = Ability.Id;
	BossIntent.BehaviorId = Ability.BehaviorId;
	BossIntent.Attack.Source = GetOwner();
	BossIntent.Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
	BossIntent.Attack.Sequence = State.BossCurrentAttackSequence;
	BossIntent.Target = Sense.Target;
	BossIntent.Origin =
	    (AbilityKind == EReEchoBossAbilityKind::BlinkSlam || AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving)
	        ? State.BossLockedTeleportDestination
	    : AbilityKind == EReEchoBossAbilityKind::PrayerBeam ? State.BossLockedTargetLocation
	                                                        : Sense.SelfLocation;
	BossIntent.LockedTargetLocation = State.BossLockedTargetLocation;
	BossIntent.LockedDirection = State.BossLockedDirection;
	BossIntent.TeleportDestination = State.BossLockedTeleportDestination;
	BossIntent.RawDamage = ResolvedDamage;
	BossIntent.RadiusCm = Ability.RadiusCm;
	BossIntent.WidthCm = Ability.WidthCm;
	BossIntent.LengthCm = Ability.LengthCm;
	BossIntent.ProjectileSpeedCmPerSecond = Ability.ProjectileSpeedCmPerSecond;
	BossIntent.WindupSeconds = Ability.WindupSeconds;
	BossIntent.ActiveSeconds = Ability.ActiveSeconds;
	BossIntent.RecoverySeconds = Ability.RecoverySeconds;
	BossIntent.TargetingMode = Ability.TargetingMode;
	BossIntent.LockTiming = Ability.LockTiming;
	BossIntent.bCanDamageTarget = State.bBossHasLockedTarget && !Sense.bTargetInvulnerable;
	BossIntent.bRequestTeleport =
	    (AbilityKind == EReEchoBossAbilityKind::BlinkSlam || AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving) &&
	    State.bBossHasLockedTeleportDestination;
	BossIntent.ComboStrikeIndex = State.BossComboStrikeIndex;
	BossIntent.ComboStrikeCount = State.BossComboStrikeCount;

	InOutIntent.Attack = BossIntent.Attack;
	InOutIntent.Target = Sense.Target;
	InOutIntent.RawDamage = ResolvedDamage;
	InOutIntent.DamageRadiusCm = Ability.RadiusCm;
	InOutIntent.SourceLocation = BossIntent.Origin;
	InOutIntent.HitLocation = State.BossLockedTargetLocation;
	InOutIntent.FacingDirection = State.BossLockedDirection;
	InOutIntent.bHasFacing = true;
	InOutIntent.bAttackCommitted = true;
	InOutIntent.bCanDamageTarget = BossIntent.bCanDamageTarget;

	SetBossAbilityCooldown(Ability.Id, ResolveCurrentBossCooldown(Ability.CooldownSeconds));
	State.bBossCurrentAbilityCommitted = true;
	AppendBossIntent(MoveTemp(BossIntent), InOutIntent);
	PublishAction(InOutIntent);
}

const FReEchoBossPhaseDefinition* UReEchoEnemyLogicComponent::GetCurrentBossPhaseDefinition() const
{
	if (State.CurrentPhaseIndex < 2)
	{
		return nullptr;
	}
	return Definition.BossPhases.FindByPredicate(
	    [this](const FReEchoBossPhaseDefinition& Phase)
	    {
		    return Phase.bEnabled && Phase.PhaseIndex == State.CurrentPhaseIndex;
	    });
}

float UReEchoEnemyLogicComponent::ResolveCurrentBossPhysicalDamage(const float BaseDamage) const
{
	const FReEchoBossPhaseDefinition* Phase = GetCurrentBossPhaseDefinition();
	return FMath::Max(0.0f, BaseDamage) * (Phase ? FMath::Max(0.0f, Phase->PhysicalAttackMultiplier) : 1.0f);
}

float UReEchoEnemyLogicComponent::ResolveCurrentBossCooldown(const float BaseCooldownSeconds) const
{
	const FReEchoBossPhaseDefinition* Phase = GetCurrentBossPhaseDefinition();
	const float AttackSpeedMultiplier = Phase ? FMath::Max(KINDA_SMALL_NUMBER, Phase->AttackSpeedMultiplier) : 1.0f;
	return FMath::Max(0.0f, BaseCooldownSeconds) / AttackSpeedMultiplier;
}

void UReEchoEnemyLogicComponent::EndBossAbility(const FReEchoEnemyAbilityDefinition& Ability,
                                                FReEchoEnemyActionIntent& InOutIntent)
{
	FReEchoBossIntent BossIntent;
	BossIntent.Type = EReEchoBossIntentType::AbilityEnded;
	BossIntent.AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
	BossIntent.AttackShape = ResolveBossAttackShape(BossIntent.AbilityKind);
	BossIntent.AbilityId = Ability.Id;
	BossIntent.BehaviorId = Ability.BehaviorId;
	BossIntent.Attack.Source = GetOwner();
	BossIntent.Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
	BossIntent.Attack.Sequence = State.BossCurrentAttackSequence;
	BossIntent.LockedTargetLocation = State.BossLockedTargetLocation;
	BossIntent.LockedDirection = State.BossLockedDirection;
	BossIntent.WindupSeconds = Ability.WindupSeconds;
	BossIntent.ActiveSeconds = Ability.ActiveSeconds;
	BossIntent.RecoverySeconds = Ability.RecoverySeconds;
	BossIntent.TargetingMode = Ability.TargetingMode;
	BossIntent.LockTiming = Ability.LockTiming;
	BossIntent.ComboStrikeIndex = State.BossComboStrikeIndex;
	BossIntent.ComboStrikeCount = State.BossComboStrikeCount;
	AppendBossIntent(MoveTemp(BossIntent), InOutIntent);

	State.BossActionPhase = EReEchoBossActionPhase::None;
	State.BossCurrentAbilityId = NAME_None;
	State.BossCurrentAttackSequence = 0;
	State.BossActionPhaseRemainingSeconds = 0.0f;
	State.bBossHasLockedTarget = false;
	State.bBossHasLockedTeleportDestination = false;
	State.bBossCurrentAbilityCommitted = false;
	State.BossComboStrikeIndex = 0;
	State.BossComboStrikeCount = 0;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	State.bBossOpeningActive = false;
}

bool UReEchoEnemyLogicComponent::IsBossComboAbility(const FReEchoEnemyAbilityDefinition& Ability) const
{
	const EReEchoBossAbilityKind AbilityKind = ResolveBossAbilityKind(Ability.BehaviorId);
	return State.CurrentPhaseIndex >= 3 && Ability.ComboMax > 1 &&
	       (AbilityKind == EReEchoBossAbilityKind::BlinkSlam || AbilityKind == EReEchoBossAbilityKind::BlinkSlamMoving);
}

void UReEchoEnemyLogicComponent::AppendBossIntent(FReEchoBossIntent&& BossIntent, FReEchoEnemyActionIntent& InOutIntent)
{
	BossIntent.bPhaseOpening = State.bBossOpeningActive;
	const int32 AbilityIndex = FindBossAbilityIndex(BossIntent.AbilityId);
	BossIntent.bGroundedSlam =
	    Definition.Abilities.IsValidIndex(AbilityIndex) && Definition.Abilities[AbilityIndex].bGroundedSlam;
	PublishBossIntent(BossIntent);
	InOutIntent.BossIntents.Add(MoveTemp(BossIntent));
}

int32 UReEchoEnemyLogicComponent::SelectBossAbility(const FReEchoEnemySenseSnapshot& Sense)
{
	if (!Sense.bTargetExists || !Sense.bTargetAlive || BossActiveAbilityIndices.IsEmpty())
	{
		return INDEX_NONE;
	}
	if (State.bBossOpeningQueued)
	{
		const FReEchoBossPhaseDefinition* Phase = GetCurrentBossPhaseDefinition();
		const int32 Index = Phase ? FindBossAbilityIndex(Phase->OpeningAbilityId) : INDEX_NONE;
		return BossActiveAbilityIndices.Contains(Index) ? Index : INDEX_NONE;
	}
	if (!DebugQueuedBossAbilityId.IsNone())
	{
		const int32 QueuedAbilityIndex = FindBossAbilityIndex(DebugQueuedBossAbilityId);
		if (!BossActiveAbilityIndices.Contains(QueuedAbilityIndex))
		{
			DebugQueuedBossAbilityId = NAME_None;
		}
		else if ((ResolveBossAbilityKind(Definition.Abilities[QueuedAbilityIndex].BehaviorId) ==
		              EReEchoBossAbilityKind::BlinkSlam ||
		          ResolveBossAbilityKind(Definition.Abilities[QueuedAbilityIndex].BehaviorId) ==
		              EReEchoBossAbilityKind::BlinkSlamMoving) &&
		         !Definition.Abilities[QueuedAbilityIndex].bGroundedSlam && !Sense.bHasTeleportDestination)
		{
			return INDEX_NONE;
		}
		else
		{
			DebugQueuedBossAbilityId = NAME_None;
			return QueuedAbilityIndex;
		}
	}
	const float Distance = FVector::Dist2D(Sense.SelfLocation, Sense.TargetLocation);
	for (int32 Offset = 0; Offset < BossActiveAbilityIndices.Num(); ++Offset)
	{
		const int32 SequenceIndex = (State.BossNextSequenceIndex + Offset) % BossActiveAbilityIndices.Num();
		const int32 AbilityIndex = BossActiveAbilityIndices[SequenceIndex];
		const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
		if (State.CurrentPhaseIndex < Ability.MinPhaseIndex || State.CurrentPhaseIndex > Ability.MaxPhaseIndex ||
		    GetBossAbilityCooldown(Ability.Id) > KINDA_SMALL_NUMBER || Distance < Ability.MinRangeCm ||
		    Distance > Ability.MaxRangeCm)
		{
			continue;
		}
		if ((ResolveBossAbilityKind(Ability.BehaviorId) == EReEchoBossAbilityKind::BlinkSlam ||
		     ResolveBossAbilityKind(Ability.BehaviorId) == EReEchoBossAbilityKind::BlinkSlamMoving) &&
		    !Ability.bGroundedSlam && !Sense.bHasTeleportDestination)
		{
			continue;
		}
		return AbilityIndex;
	}
	return INDEX_NONE;
}

bool UReEchoEnemyLogicComponent::DebugQueueBossAbility(const FName AbilityId, const int32 ForcedComboCount)
{
	if (!bInitialized || Definition.Archetype != EReEchoEnemyArchetype::Boss)
	{
		return false;
	}
	const int32 AbilityIndex = FindBossAbilityIndex(AbilityId);
	if (!BossActiveAbilityIndices.Contains(AbilityIndex))
	{
		return false;
	}
	const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[AbilityIndex];
	if (State.CurrentPhaseIndex < Ability.MinPhaseIndex || State.CurrentPhaseIndex > Ability.MaxPhaseIndex)
	{
		return false;
	}
	DebugQueuedBossAbilityId = AbilityId;
	DebugForcedComboCount = ForcedComboCount;
	return true;
}

bool UReEchoEnemyLogicComponent::QueueBossPhaseOpening()
{
	if (!bInitialized || !State.bAlive || Definition.Archetype != EReEchoEnemyArchetype::Boss ||
	    State.CurrentPhaseIndex != 3 || State.bBossOpeningConsumed)
	{
		return false;
	}
	const FReEchoBossPhaseDefinition* Phase = GetCurrentBossPhaseDefinition();
	const int32 Index = Phase ? FindBossAbilityIndex(Phase->OpeningAbilityId) : INDEX_NONE;
	if (!BossActiveAbilityIndices.Contains(Index) || Phase->OpeningStrikeCount <= 0)
	{
		return false;
	}
	State.bBossOpeningConsumed = true;
	State.bBossOpeningQueued = true;
	return true;
}

bool UReEchoEnemyLogicComponent::DebugSetBossEncounterElapsedSeconds(const float Seconds)
{
	if (!bInitialized || Definition.Archetype != EReEchoEnemyArchetype::Boss || Seconds < 0.0f)
	{
		return false;
	}
	State.BossEncounterElapsedSeconds = Seconds;
	return true;
}

int32 UReEchoEnemyLogicComponent::FindBossAbilityIndex(const FName AbilityId) const
{
	return Definition.Abilities.IndexOfByPredicate(
	    [AbilityId](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == AbilityId;
	    });
}

float UReEchoEnemyLogicComponent::GetBossAbilityCooldown(const FName AbilityId) const
{
	const FReEchoBossAbilityCooldownSnapshot* Cooldown = State.BossAbilityCooldowns.FindByPredicate(
	    [AbilityId](const FReEchoBossAbilityCooldownSnapshot& Candidate)
	    {
		    return Candidate.AbilityId == AbilityId;
	    });
	return Cooldown ? Cooldown->RemainingSeconds : 0.0f;
}

void UReEchoEnemyLogicComponent::SetBossAbilityCooldown(const FName AbilityId, const float RemainingSeconds)
{
	if (FReEchoBossAbilityCooldownSnapshot* Cooldown = State.BossAbilityCooldowns.FindByPredicate(
	        [AbilityId](const FReEchoBossAbilityCooldownSnapshot& Candidate)
	        {
		        return Candidate.AbilityId == AbilityId;
	        }))
	{
		Cooldown->RemainingSeconds = FMath::Max(0.0f, RemainingSeconds);
	}
}

void UReEchoEnemyLogicComponent::ApplyBossHitReaction(const float FixedDeltaSeconds,
                                                      FReEchoEnemyActionIntent& InOutIntent)
{
	InOutIntent.MovementDelta += State.KnockbackVelocity * FixedDeltaSeconds;
	InOutIntent.bHasMovement = !InOutIntent.MovementDelta.IsNearlyZero();
	State.KnockbackVelocity =
	    FMath::VInterpTo(State.KnockbackVelocity, FVector::ZeroVector, FixedDeltaSeconds, Definition.KnockbackDrag);
	State.HitReactionRemainingSeconds = FMath::Max(0.0f, State.HitReactionRemainingSeconds - FixedDeltaSeconds);
	State.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
}

void UReEchoEnemyLogicComponent::ApplyStandardMovement(const FReEchoEnemySenseSnapshot& Sense,
                                                       const float DeltaSeconds,
                                                       FReEchoEnemyActionIntent& InOutIntent)
{
	if (!Sense.bTargetExists || !Sense.bTargetAlive)
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		return;
	}
	FVector ToTarget = Sense.TargetLocation - Sense.SelfLocation;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();
	const FVector Direction = ToTarget.GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		State.FacingDirection = Direction;
		InOutIntent.FacingDirection = Direction;
		InOutIntent.bHasFacing = true;
	}
	if (Distance > Definition.MovementStopDistanceCm && !Direction.IsNearlyZero())
	{
		InOutIntent.MovementDelta += Direction * Definition.MoveSpeedCmPerSecond * DeltaSeconds;
		InOutIntent.bHasMovement = !InOutIntent.MovementDelta.IsNearlyZero();
		State.Phase = EReEchoEnemyBehaviorPhase::Pursuing;
	}
	else
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	}
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvanceIdleWander(const FReEchoEnemySenseSnapshot& Sense,
                                                                       const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;

	const float WanderSpeed = Definition.MoveSpeedCmPerSecond * IdleWanderSpeedFraction;
	if (WanderSpeed <= 0.0f || DeltaSeconds <= 0.0f)
	{
		return Intent;
	}

	State.IdleWanderElapsedSeconds += DeltaSeconds;
	if (State.IdleWanderElapsedSeconds >= IdleWanderPeriodSeconds)
	{
		State.IdleWanderElapsedSeconds = FMath::Fmod(State.IdleWanderElapsedSeconds, IdleWanderPeriodSeconds);
		State.IdleWanderDirection = MakeIdleWanderDirection(Sense, State.SpawnIndex, State.IdleWanderElapsedSeconds);
	}

	const FVector Direction = State.IdleWanderDirection.GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return Intent;
	}
	State.FacingDirection = Direction;
	Intent.FacingDirection = Direction;
	Intent.bHasFacing = true;
	Intent.MovementDelta = Direction * WanderSpeed * DeltaSeconds;
	Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
	return Intent;
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvanceHitReaction(const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	State.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
	const float StepSeconds = FMath::Min(FMath::Max(0.0f, DeltaSeconds), State.HitReactionRemainingSeconds);
	State.KnockbackVelocity = FVector::ZeroVector;
	State.HitReactionRemainingSeconds = FMath::Max(0.0f, State.HitReactionRemainingSeconds - StepSeconds);
	if (State.HitReactionRemainingSeconds <= KINDA_SMALL_NUMBER)
	{
		State.HitReactionRemainingSeconds = 0.0f;
		State.KnockbackVelocity = FVector::ZeroVector;
	}
	return Intent;
}

void UReEchoEnemyLogicComponent::CommitAttack(const FReEchoEnemySenseSnapshot& Sense,
                                              const bool bCanDamageTarget,
                                              const bool bSelfDestruct,
                                              FReEchoEnemyActionIntent& InOutIntent)
{
	InOutIntent.Attack.Source = GetOwner();
	InOutIntent.Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
	InOutIntent.Attack.Sequence = ++State.AttackSequence;
	InOutIntent.Target = Sense.Target;
	InOutIntent.RawDamage = Definition.ContactDamage;
	InOutIntent.DamageRadiusCm = bSelfDestruct ? Definition.BomberDamageRadiusCm : 0.0f;
	InOutIntent.SourceLocation = Sense.SelfLocation;
	InOutIntent.HitLocation = Sense.TargetLocation;
	if (!InOutIntent.bHasFacing)
	{
		InOutIntent.FacingDirection = State.FacingDirection;
		InOutIntent.bHasFacing = !State.FacingDirection.IsNearlyZero();
	}
	InOutIntent.bAttackCommitted = true;
	InOutIntent.bCanDamageTarget = bCanDamageTarget;
	InOutIntent.bSelfDestructAfterAttack = bSelfDestruct;

	State.AttackCooldownRemainingSeconds = Definition.AttackIntervalSeconds;
	State.Phase = EReEchoEnemyBehaviorPhase::Attacking;
	if (bSelfDestruct)
	{
		State.bFuseActive = false;
		State.bSelfDestructCommitted = true;
	}
	PublishAction(InOutIntent);
}

void UReEchoEnemyLogicComponent::NotifyHurt(const float AppliedDamage,
                                            const FVector& /*SourceLocation*/,
                                            const FVector& /*SelfLocation*/)
{
	if (!bInitialized || !State.bAlive || AppliedDamage <= 0.0f)
	{
		return;
	}
	if (Definition.Archetype == EReEchoEnemyArchetype::Boss)
	{
		return;
	}
	CancelSpecialAction();
	State.KnockbackVelocity = FVector::ZeroVector;
	State.HitReactionRemainingSeconds = Definition.HitReactionDurationSeconds;
	State.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
}

void UReEchoEnemyLogicComponent::NotifyReceivedAttack(const FReEchoDamageEvent& Event)
{
	if (!bInitialized || !State.bAlive || Event.Target != GetOwner() || Event.AppliedDamage <= 0.0f)
	{
		return;
	}
	// A fatal hit starts the phase transition inside Combat's fatal-damage interception, then the same resolved hit
	// is published as Hurt. That trailing Hurt must not replace Transforming with HitReaction or the transition timer
	// can never complete.
	if (State.Phase == EReEchoEnemyBehaviorPhase::Transforming)
	{
		return;
	}

	const bool bCountsForPhase = Event.DamageSource != EReEchoDamageSource::Enemy;
	if (Definition.Phase2.bEnabled && !State.bPhase2Triggered && bCountsForPhase)
	{
		++State.ReceivedDamageCount;
	}

	NotifyHurt(Event.AppliedDamage, Event.SourceWorldLocation, Event.WorldLocation);
}

bool UReEchoEnemyLogicComponent::IsHealthAtOrBelowPhase2Threshold(float CurrentHealthRatio) const
{
	// The host samples health and passes it in as a sense input; enemy logic never reads the owner's combat component
	// directly. A ratio at or below the configured threshold (0 = depleted to zero) fires the transition.
	return CurrentHealthRatio <= Definition.Phase2.HealthThresholdRatio;
}

bool UReEchoEnemyLogicComponent::TryBeginPhaseTransition(const FReEchoEnemySenseSnapshot& Sense,
                                                         FReEchoEnemyActionIntent& InOutIntent)
{
	if (!Sense.bPhase2TransitionPermitted || !Definition.Phase2.bEnabled || State.bPhase2Triggered)
	{
		return false;
	}

	const bool bAttackCountReached =
	    Definition.Phase2.RequiredAttackCount > 0 && State.ReceivedDamageCount >= Definition.Phase2.RequiredAttackCount;
	const bool bRangeEntered =
	    Definition.Phase2.TriggerRangeCm > 0.0f && Sense.bTargetExists && Sense.bTargetAlive &&
	    Sense.bTargetCanAttractAggro &&
	    FVector::Dist2D(Sense.SelfLocation, Sense.TargetLocation) <= Definition.Phase2.TriggerRangeCm;
	// WS4 (Plan 68): blood-bar depleted trigger. Only meaningful in HealthThreshold mode; fires when the current
	// health ratio is at or below the configured threshold (ratio == 0 means "depleted to zero").
	const bool bHealthDepleted = Definition.Phase2.TriggerMode == EReEchoEnemyPhase2TriggerMode::HealthThreshold &&
	                             IsHealthAtOrBelowPhase2Threshold(Sense.CurrentHealthRatio);
	if (!bAttackCountReached && !bRangeEntered && !bHealthDepleted)
	{
		return false;
	}

	State.bPhase2Triggered = true;
	if (bHealthDepleted)
	{
		State.PhaseTriggerReason = EReEchoEnemyPhaseTriggerReason::HealthDepleted;
	}
	else
	{
		State.PhaseTriggerReason = bAttackCountReached ? EReEchoEnemyPhaseTriggerReason::AttackCountReached
		                                               : EReEchoEnemyPhaseTriggerReason::RangeEntered;
	}
	State.PhaseTransitionRemainingSeconds = FMath::Max(0.0f, Definition.Phase2.TransformSeconds);
	State.Phase = EReEchoEnemyBehaviorPhase::Transforming;
	CancelUncommittedActionsForPhaseTransition();
	InOutIntent.bPhaseTransitionStarted = true;
	InOutIntent.PhaseTriggerReason = State.PhaseTriggerReason;
	if (const FReEchoBossPhaseDefinition* PhaseDefinition = Definition.BossPhases.FindByPredicate(
	        [](const FReEchoBossPhaseDefinition& Candidate)
	        {
		        return Candidate.bEnabled && Candidate.PhaseIndex == 2;
	        }))
	{
		InOutIntent.PhaseDefinition = *PhaseDefinition;
	}
	if (State.PhaseTransitionRemainingSeconds <= 0.0f)
	{
		State.CurrentPhaseIndex = 2;
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		InOutIntent.bPhaseTransitionCompleted = true;
	}
	return true;
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvancePhaseTransition(const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	State.PhaseTransitionRemainingSeconds =
	    FMath::Max(0.0f, State.PhaseTransitionRemainingSeconds - FMath::Max(0.0f, DeltaSeconds));
	if (State.PhaseTransitionRemainingSeconds <= 0.0f)
	{
		State.CurrentPhaseIndex = 2;
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		Intent.bPhaseTransitionCompleted = true;
		Intent.PhaseTriggerReason = State.PhaseTriggerReason;
	}
	return Intent;
}

void UReEchoEnemyLogicComponent::CancelUncommittedActionsForPhaseTransition()
{
	State.HitReactionRemainingSeconds = 0.0f;
	State.KnockbackVelocity = FVector::ZeroVector;
	CancelSpecialAction();
	if (!State.bBossCurrentAbilityCommitted)
	{
		State.BossActionPhase = EReEchoBossActionPhase::None;
		State.BossCurrentAbilityId = NAME_None;
		State.BossCurrentAttackSequence = 0;
		State.BossActionPhaseRemainingSeconds = 0.0f;
		State.bBossHasLockedTarget = false;
		State.bBossHasLockedTeleportDestination = false;
	}
}

bool UReEchoEnemyLogicComponent::CompleteCinematicPhase2(FReEchoEnemyActionIntent& OutIntent)
{
	OutIntent = {};
	if (!bInitialized || !State.bAlive || Definition.Archetype == EReEchoEnemyArchetype::Boss ||
	    !Definition.Phase2.bEnabled)
	{
		return false;
	}
	if (State.CurrentPhaseIndex >= 2)
	{
		return true;
	}
	CancelUncommittedActionsForPhaseTransition();
	State.bPhase2Triggered = true;
	State.PhaseTransitionRemainingSeconds = 0.0f;
	State.CurrentPhaseIndex = 2;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	OutIntent.bPhaseTransitionCompleted = true;
	return true;
}

void UReEchoEnemyLogicComponent::NotifyDeath()
{
	if (!bInitialized)
	{
		return;
	}
	State.bAlive = false;
	State.bFuseActive = false;
	State.FuseRemainingSeconds = 0.0f;
	State.HitReactionRemainingSeconds = 0.0f;
	State.KnockbackVelocity = FVector::ZeroVector;
	CancelSpecialAction();
	State.BossActionPhase = EReEchoBossActionPhase::None;
	State.BossCurrentAbilityId = NAME_None;
	State.BossCurrentAttackSequence = 0;
	State.BossActionPhaseRemainingSeconds = 0.0f;
	State.bBossHasLockedTarget = false;
	State.bBossHasLockedTeleportDestination = false;
	State.bBossCurrentAbilityCommitted = false;
	State.Phase = EReEchoEnemyBehaviorPhase::Dead;
}

void UReEchoEnemyLogicComponent::CancelActiveActionsForStun()
{
	if (!bInitialized || !State.bAlive)
	{
		return;
	}

	const bool bHadSpecialAction = State.SpecialActionPhase != EReEchoEnemySpecialActionPhase::None;
	CancelSpecialAction();
	if (bHadSpecialAction)
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	}

	if (State.BossActionPhase != EReEchoBossActionPhase::None)
	{
		const int32 AbilityIndex = FindBossAbilityIndex(State.BossCurrentAbilityId);
		if (Definition.Abilities.IsValidIndex(AbilityIndex))
		{
			FReEchoEnemyActionIntent Intent;
			EndBossAbility(Definition.Abilities[AbilityIndex], Intent);
		}
		else
		{
			State.BossActionPhase = EReEchoBossActionPhase::None;
			State.BossCurrentAbilityId = NAME_None;
			State.BossCurrentAttackSequence = 0;
			State.BossActionPhaseRemainingSeconds = 0.0f;
			State.BossLockedTargetLocation = FVector::ZeroVector;
			State.BossLockedDirection = State.FacingDirection.GetSafeNormal2D();
			State.BossLockedTeleportDestination = FVector::ZeroVector;
			State.bBossHasLockedTarget = false;
			State.bBossHasLockedTeleportDestination = false;
			State.bBossCurrentAbilityCommitted = false;
			State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		}
	}
}

void UReEchoEnemyLogicComponent::ResetEncounterTransientState()
{
	if (!bInitialized || !State.bAlive)
	{
		return;
	}

	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	State.FuseRemainingSeconds = 0.0f;
	State.HitReactionRemainingSeconds = 0.0f;
	State.KnockbackVelocity = FVector::ZeroVector;
	State.bFuseActive = false;
	CancelSpecialAction();
	State.BossActionPhase = EReEchoBossActionPhase::None;
	State.BossCurrentAbilityId = NAME_None;
	State.BossCurrentAttackSequence = 0;
	State.BossActionPhaseRemainingSeconds = 0.0f;
	State.BossSimulationAccumulatorSeconds = 0.0f;
	State.BossLockedTargetLocation = FVector::ZeroVector;
	State.BossLockedDirection = State.FacingDirection.GetSafeNormal2D();
	State.BossLockedTeleportDestination = FVector::ZeroVector;
	State.bBossHasLockedTarget = false;
	State.bBossHasLockedTeleportDestination = false;
	State.bBossCurrentAbilityCommitted = false;
	DebugQueuedBossAbilityId = NAME_None;
}

void UReEchoEnemyLogicComponent::RestoreSnapshot(const FReEchoEnemyLogicSnapshot& InSnapshot)
{
	DebugQueuedBossAbilityId = NAME_None;
	if (!bInitialized || InSnapshot.Archetype != Definition.Archetype)
	{
		return;
	}
	State = InSnapshot;
	State.AttackCooldownRemainingSeconds = FMath::Max(0.0f, State.AttackCooldownRemainingSeconds);
	State.FuseRemainingSeconds = FMath::Max(0.0f, State.FuseRemainingSeconds);
	State.HitReactionRemainingSeconds = FMath::Max(0.0f, State.HitReactionRemainingSeconds);
	// Legacy saves may contain gameplay knockback velocity. HitReaction is now a complete movement lock.
	State.KnockbackVelocity = FVector::ZeroVector;
	State.AttackSequence = FMath::Max<int64>(0, State.AttackSequence);
	State.CurrentPhaseIndex = FMath::Clamp(State.CurrentPhaseIndex, 1, 3);
	State.ReceivedDamageCount = FMath::Max(0, State.ReceivedDamageCount);
	State.PhaseTransitionRemainingSeconds = FMath::Max(0.0f, State.PhaseTransitionRemainingSeconds);
	State.SpecialNextSequenceIndex =
	    SpecialActiveAbilityIndices.IsEmpty()
	        ? 0
	        : FMath::Clamp(State.SpecialNextSequenceIndex, 0, SpecialActiveAbilityIndices.Num() - 1);
	State.SpecialActionRemainingSeconds = FMath::Max(0.0f, State.SpecialActionRemainingSeconds);
	State.SpecialLockedDirection = State.SpecialLockedDirection.GetSafeNormal2D();
	if (State.SpecialLockedDirection.IsNearlyZero())
	{
		State.SpecialLockedDirection = FVector::ForwardVector;
	}
	if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::None || !FindSpecialAbility(State.SpecialAbilityId))
	{
		ClearSpecialAction();
	}
	else if (State.SpecialActionPhase == EReEchoEnemySpecialActionPhase::Active)
	{
		const FReEchoEnemyAbilityDefinition* Ability = FindSpecialAbility(State.SpecialAbilityId);
		State.SpecialActionRemainingSeconds =
		    FMath::Min(State.SpecialActionRemainingSeconds, Ability ? FMath::Max(0.0f, Ability->ActiveSeconds) : 0.0f);
		State.SpecialDashRemainingDistanceCm = FMath::Min(FMath::Max(0.0f, State.SpecialDashRemainingDistanceCm),
		                                                  Ability ? FMath::Max(0.0f, Ability->LengthCm) : 0.0f);
		State.SpecialAttack.Sequence = FMath::Clamp<int64>(State.SpecialAttack.Sequence, 0, State.AttackSequence);
		if (State.SpecialAttack.Sequence > 0)
		{
			State.SpecialAttack.Source = GetOwner();
			State.SpecialAttack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(GetOwner());
		}
		const bool bSafeActiveRestore = Ability && Definition.Archetype == EReEchoEnemyArchetype::Elite &&
		                                State.SpecialActionRemainingSeconds > KINDA_SMALL_NUMBER &&
		                                State.SpecialDashRemainingDistanceCm > KINDA_SMALL_NUMBER &&
		                                State.SpecialAttack.Sequence > 0;
		if (!bSafeActiveRestore)
		{
			State.bSpecialDamageConsumed = true;
			if (Ability)
			{
				BeginSpecialRecovery(*Ability);
			}
			else
			{
				ClearSpecialAction();
			}
		}
	}
	else
	{
		State.SpecialDashRemainingDistanceCm = 0.0f;
		State.SpecialAttack = {};
		State.bSpecialDamageConsumed = false;
	}
	if (!Definition.Phase2.bEnabled)
	{
		State.CurrentPhaseIndex = 1;
		State.ReceivedDamageCount = 0;
		State.PhaseTransitionRemainingSeconds = 0.0f;
		State.PhaseTriggerReason = EReEchoEnemyPhaseTriggerReason::None;
		State.bPhase2Triggered = false;
	}
	else if (State.CurrentPhaseIndex >= 2)
	{
		State.bPhase2Triggered = true;
		State.PhaseTransitionRemainingSeconds = 0.0f;
		if (State.Phase == EReEchoEnemyBehaviorPhase::Transforming)
		{
			State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		}
	}
	if (Definition.Archetype == EReEchoEnemyArchetype::Boss)
	{
		State.HitReactionRemainingSeconds = 0.0f;
		State.KnockbackVelocity = FVector::ZeroVector;
		TArray<FReEchoBossAbilityCooldownSnapshot> RestoredCooldowns;
		for (const int32 AbilityIndex : BossActiveAbilityIndices)
		{
			const FName AbilityId = Definition.Abilities[AbilityIndex].Id;
			const FReEchoBossAbilityCooldownSnapshot* SavedCooldown = InSnapshot.BossAbilityCooldowns.FindByPredicate(
			    [AbilityId](const FReEchoBossAbilityCooldownSnapshot& Candidate)
			    {
				    return Candidate.AbilityId == AbilityId;
			    });
			FReEchoBossAbilityCooldownSnapshot& Restored = RestoredCooldowns.AddDefaulted_GetRef();
			Restored.AbilityId = AbilityId;
			Restored.RemainingSeconds = SavedCooldown ? FMath::Max(0.0f, SavedCooldown->RemainingSeconds) : 0.0f;
		}
		State.BossAbilityCooldowns = MoveTemp(RestoredCooldowns);
		State.BossNextSequenceIndex =
		    BossActiveAbilityIndices.IsEmpty()
		        ? 0
		        : FMath::Clamp(State.BossNextSequenceIndex, 0, BossActiveAbilityIndices.Num() - 1);
		State.BossNextPhaseIndex = FMath::Clamp(State.BossNextPhaseIndex, 0, BossPhaseIndices.Num());
		State.BossCurrentAttackSequence = FMath::Clamp<int64>(State.BossCurrentAttackSequence, 0, State.AttackSequence);
		State.BossComboStrikeCount = FMath::Max(0, State.BossComboStrikeCount);
		State.BossComboStrikeIndex = FMath::Clamp(State.BossComboStrikeIndex, 0, State.BossComboStrikeCount);
		State.BossActionPhaseRemainingSeconds = FMath::Max(0.0f, State.BossActionPhaseRemainingSeconds);
		State.BossCleanseRemainingSeconds = FMath::Max(0.0f, State.BossCleanseRemainingSeconds);
		State.BossEncounterElapsedSeconds = FMath::Max(0.0f, State.BossEncounterElapsedSeconds);
		State.BossSimulationAccumulatorSeconds =
		    FMath::Fmod(FMath::Max(0.0f, State.BossSimulationAccumulatorSeconds), BossFixedDeltaSeconds);
		State.BossLockedDirection = State.BossLockedDirection.GetSafeNormal2D();
		if (State.BossLockedDirection.IsNearlyZero())
		{
			State.BossLockedDirection = FVector::ForwardVector;
		}
		const int32 CurrentAbilityIndex = FindBossAbilityIndex(State.BossCurrentAbilityId);
		if (State.BossActionPhase == EReEchoBossActionPhase::None ||
		    !BossActiveAbilityIndices.Contains(CurrentAbilityIndex))
		{
			State.BossActionPhase = EReEchoBossActionPhase::None;
			State.BossCurrentAbilityId = NAME_None;
			State.BossCurrentAttackSequence = 0;
			State.BossActionPhaseRemainingSeconds = 0.0f;
			State.bBossHasLockedTarget = false;
			State.bBossHasLockedTeleportDestination = false;
			State.bBossCurrentAbilityCommitted = false;
			State.BossComboStrikeIndex = 0;
			State.BossComboStrikeCount = 0;
		}
		else
		{
			const FReEchoEnemyAbilityDefinition& Ability = Definition.Abilities[CurrentAbilityIndex];
			if (State.CurrentPhaseIndex < Ability.MinPhaseIndex || State.CurrentPhaseIndex > Ability.MaxPhaseIndex ||
			    (IsBossComboAbility(Ability) &&
			     (State.BossComboStrikeCount < Ability.ComboMin || State.BossComboStrikeCount > Ability.ComboMax ||
			      State.BossComboStrikeIndex < 1)))
			{
				State.BossActionPhase = EReEchoBossActionPhase::None;
				State.BossCurrentAbilityId = NAME_None;
				State.BossCurrentAttackSequence = 0;
				State.BossComboStrikeIndex = 0;
				State.BossComboStrikeCount = 0;
			}
		}
		if (State.Phase == EReEchoEnemyBehaviorPhase::HitReaction)
		{
			switch (State.BossActionPhase)
			{
				case EReEchoBossActionPhase::Windup:
					State.Phase = EReEchoEnemyBehaviorPhase::BossWindup;
					break;
				case EReEchoBossActionPhase::Active:
					State.Phase = EReEchoEnemyBehaviorPhase::BossActive;
					break;
				case EReEchoBossActionPhase::Recovery:
					State.Phase = EReEchoEnemyBehaviorPhase::BossRecovery;
					break;
				default:
					State.Phase = EReEchoEnemyBehaviorPhase::Idle;
					break;
			}
		}
	}
	if (!State.bAlive)
	{
		NotifyDeath();
	}
}

FReEchoEnemyLogicSnapshot UReEchoEnemyLogicComponent::GetSnapshot() const
{
	return State;
}

const FReEchoEnemyDefinition& UReEchoEnemyLogicComponent::GetDefinition() const
{
	return Definition;
}

void UReEchoEnemyLogicComponent::HandleCombatHurt(const FReEchoDamageEvent& Event)
{
	if (Event.Target != GetOwner())
	{
		return;
	}
	NotifyReceivedAttack(Event);
}

void UReEchoEnemyLogicComponent::HandleCombatDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target == GetOwner())
	{
		NotifyDeath();
	}
}

bool UReEchoEnemyLogicComponent::TryTriggerPhase2OnFatalWound(FReEchoEnemyActionIntent& OutIntent)
{
	OutIntent = {};
	if (!bInitialized || !State.bAlive)
	{
		return false;
	}
	// Build a minimal sense for the transition check. Only the health-threshold predicate and target existence matter
	// here; location is unused by that path. A real target keeps bTargetExists consistent with aggro rules.
	FReEchoEnemySenseSnapshot Sense;
	Sense.SelfLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	// This is called from the combatant fatal-damage hook, i.e. exactly when health has been driven to zero or below,
	// so the blood-depleted threshold is trivially satisfied. The target/range branches stay inert here.
	Sense.CurrentHealthRatio = 0.0f;
	return TryBeginPhaseTransition(Sense, OutIntent);
}

int32 UReEchoEnemyLogicComponent::ResolveFatalBossTransitionPhase(const bool bHasEasterEggCard) const
{
	if (!bInitialized || !State.bAlive || Definition.Archetype != EReEchoEnemyArchetype::Boss)
	{
		return 0;
	}
	if (State.CurrentPhaseIndex == 1 && Definition.Phase2.bEnabled && !State.bPhase2Triggered &&
	    Definition.Phase2.TriggerMode == EReEchoEnemyPhase2TriggerMode::HealthThreshold &&
	    IsHealthAtOrBelowPhase2Threshold(0.0f))
	{
		return 2;
	}
	const FReEchoBossPhaseDefinition* Phase3 = Definition.BossPhases.FindByPredicate(
	    [](const FReEchoBossPhaseDefinition& Phase)
	    {
		    return Phase.bEnabled && Phase.PhaseIndex == 3;
	    });
	return State.CurrentPhaseIndex == 2 && Phase3 &&
	               (bHasEasterEggCard || State.BossEncounterElapsedSeconds <= Phase3->TriggerSeconds)
	           ? 3
	           : 0;
}

bool UReEchoEnemyLogicComponent::TryTriggerPhase3OnFatalWound(FReEchoEnemyActionIntent& OutIntent,
                                                              const bool bHasEasterEggCard)
{
	OutIntent = {};
	if (!bInitialized || !State.bAlive || Definition.Archetype != EReEchoEnemyArchetype::Boss ||
	    State.CurrentPhaseIndex != 2)
	{
		return false;
	}
	const FReEchoBossPhaseDefinition* Phase3 = Definition.BossPhases.FindByPredicate(
	    [](const FReEchoBossPhaseDefinition& Candidate)
	    {
		    return Candidate.bEnabled && Candidate.PhaseIndex == 3;
	    });
	if (!Phase3 || (!bHasEasterEggCard && State.BossEncounterElapsedSeconds > Phase3->TriggerSeconds))
	{
		return false;
	}
	return DebugForceBossPhase(3, OutIntent);
}

bool UReEchoEnemyLogicComponent::DebugForceBossPhase(const int32 PhaseIndex, FReEchoEnemyActionIntent& OutIntent)
{
	OutIntent = {};
	if (!bInitialized || !State.bAlive || Definition.Archetype != EReEchoEnemyArchetype::Boss)
	{
		return false;
	}
	const FReEchoBossPhaseDefinition* PhaseDefinition = Definition.BossPhases.FindByPredicate(
	    [PhaseIndex](const FReEchoBossPhaseDefinition& Candidate)
	    {
		    return Candidate.bEnabled && Candidate.PhaseIndex == PhaseIndex;
	    });
	if (!PhaseDefinition)
	{
		return false;
	}
	CancelUncommittedActionsForPhaseTransition();
	State.BossActionPhase = EReEchoBossActionPhase::None;
	State.BossCurrentAbilityId = NAME_None;
	State.BossCurrentAttackSequence = 0;
	State.BossComboStrikeIndex = 0;
	State.BossComboStrikeCount = 0;
	State.BossActionPhaseRemainingSeconds = 0.0f;
	State.CurrentPhaseIndex = PhaseIndex;
	State.bBossOpeningActive = false;
	State.bBossOpeningQueued = false;
	State.bPhase2Triggered = State.bPhase2Triggered || PhaseIndex >= 2;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	DebugQueuedBossAbilityId = NAME_None;
	DebugForcedComboCount = 0;
	OutIntent.bPhaseTransitionStarted = true;
	OutIntent.bPhaseTransitionCompleted = true;
	OutIntent.PhaseTriggerReason = EReEchoEnemyPhaseTriggerReason::HealthDepleted;
	OutIntent.PhaseDefinition = *PhaseDefinition;
	FReEchoBossIntent BossIntent;
	BossIntent.Type = EReEchoBossIntentType::EncounterPhase;
	BossIntent.PhaseDefinition = *PhaseDefinition;
	AppendBossIntent(MoveTemp(BossIntent), OutIntent);
	return true;
}

void UReEchoEnemyLogicComponent::PublishFuse(const bool bStarted) const
{
	if (!EnemyEvents)
	{
		return;
	}
	FReEchoEnemyFuseEvent Event;
	Event.RemainingSeconds = State.FuseRemainingSeconds;
	Event.DurationSeconds = Definition.BomberFuseDurationSeconds;
	Event.bStarted = bStarted;
	EnemyEvents->PublishFuseChanged(Event);
}

void UReEchoEnemyLogicComponent::PublishAction(const FReEchoEnemyActionIntent& Intent) const
{
	if (!EnemyEvents)
	{
		return;
	}
	FReEchoEnemyActionCommittedEvent Event;
	Event.Attack = Intent.Attack;
	Event.Target = Intent.Target.Get();
	Event.Archetype = Definition.Archetype;
	Event.Origin = Intent.SourceLocation;
	Event.bCanDamageTarget = Intent.bCanDamageTarget;
	Event.bSelfDestructAfterAttack = Intent.bSelfDestructAfterAttack;
	EnemyEvents->PublishActionCommitted(Event);
}

void UReEchoEnemyLogicComponent::PublishBossIntent(const FReEchoBossIntent& Intent) const
{
	if (EnemyEvents)
	{
		EnemyEvents->PublishBossIntent(Intent);
	}
}
