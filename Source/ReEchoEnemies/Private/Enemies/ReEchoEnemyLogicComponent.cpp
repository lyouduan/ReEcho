#include "Enemies/ReEchoEnemyLogicComponent.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "GameFramework/Actor.h"

namespace
{
constexpr float MinimumFuseDurationSeconds = 0.1f;
}

FReEchoEnemyDefinition ReEchoEnemyDefinitions::MakeLegacyEquivalent(const EReEchoEnemyArchetype Archetype,
                                                                     const float BomberTriggerRadiusCm,
                                                                     const float BomberDamageRadiusCm,
                                                                     const float BomberFuseDurationSeconds,
                                                                     const float BomberDamage)
{
	FReEchoEnemyDefinition Result;
	Result.Archetype = Archetype;
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
	if (InDefinition.MaxHealth <= 0.0f || InDefinition.MoveSpeedCmPerSecond < 0.0f ||
	    InDefinition.ContactDamage < 0.0f || InDefinition.AttackIntervalSeconds < 0.0f ||
	    InDefinition.ContactRangeCm < 0.0f || InDefinition.MovementStopDistanceCm < 0.0f)
	{
		return false;
	}

	Definition = InDefinition;
	Definition.BomberTriggerRadiusCm = FMath::Max(0.0f, Definition.BomberTriggerRadiusCm);
	Definition.BomberDamageRadiusCm = FMath::Max(0.0f, Definition.BomberDamageRadiusCm);
	Definition.BomberFuseDurationSeconds =
	    FMath::Max(MinimumFuseDurationSeconds, Definition.BomberFuseDurationSeconds);
	Definition.HitReactionDurationSeconds = FMath::Max(0.0f, Definition.HitReactionDurationSeconds);
	Definition.KnockbackSpeedCmPerSecond = FMath::Max(0.0f, Definition.KnockbackSpeedCmPerSecond);
	Definition.KnockbackDrag = FMath::Max(0.0f, Definition.KnockbackDrag);

	State = {};
	State.Archetype = Definition.Archetype;
	State.SpawnIndex = InSpawnIndex;
	State.Phase = EReEchoEnemyBehaviorPhase::Idle;
	State.bAlive = true;
	bInitialized = true;
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
	State.AttackCooldownRemainingSeconds =
	    FMath::Max(0.0f, State.AttackCooldownRemainingSeconds - SafeDeltaSeconds);
	if (State.HitReactionRemainingSeconds > 0.0f)
	{
		return AdvanceHitReaction(SafeDeltaSeconds);
	}

	if (!Sense.bTargetExists || !Sense.bTargetAlive)
	{
		State.Phase = EReEchoEnemyBehaviorPhase::Idle;
		return Intent;
	}

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
			if (State.FuseRemainingSeconds <= 0.0f)
			{
				CommitAttack(Sense, Distance <= Definition.BomberDamageRadiusCm, true, Intent);
				return Intent;
			}
		}
		return Intent;
	}

	if (Distance <= Definition.ContactRangeCm && State.AttackCooldownRemainingSeconds <= 0.0f)
	{
		CommitAttack(Sense, !Sense.bTargetInvulnerable, false, Intent);
	}
	return Intent;
}

FReEchoEnemyActionIntent UReEchoEnemyLogicComponent::AdvanceHitReaction(const float DeltaSeconds)
{
	FReEchoEnemyActionIntent Intent;
	State.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
	Intent.MovementDelta = State.KnockbackVelocity * DeltaSeconds;
	Intent.bHasMovement = !Intent.MovementDelta.IsNearlyZero();
	State.KnockbackVelocity = FMath::VInterpTo(
	    State.KnockbackVelocity, FVector::ZeroVector, DeltaSeconds, Definition.KnockbackDrag);
	State.HitReactionRemainingSeconds = FMath::Max(0.0f, State.HitReactionRemainingSeconds - DeltaSeconds);
	return Intent;
}

void UReEchoEnemyLogicComponent::CommitAttack(const FReEchoEnemySenseSnapshot& Sense,
                                               const bool bCanDamageTarget,
                                               const bool bSelfDestruct,
                                               FReEchoEnemyActionIntent& InOutIntent)
{
	InOutIntent.Attack.Source = GetOwner();
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
                                            const FVector& SourceLocation,
                                            const FVector& SelfLocation)
{
	if (!bInitialized || !State.bAlive || AppliedDamage <= 0.0f)
	{
		return;
	}

	FVector KnockbackDirection = (SelfLocation - SourceLocation).GetSafeNormal2D();
	if (KnockbackDirection.IsNearlyZero())
	{
		KnockbackDirection = -State.FacingDirection.GetSafeNormal2D();
	}
	State.KnockbackVelocity = KnockbackDirection * Definition.KnockbackSpeedCmPerSecond;
	State.HitReactionRemainingSeconds = Definition.HitReactionDurationSeconds;
	State.Phase = EReEchoEnemyBehaviorPhase::HitReaction;
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
	State.Phase = EReEchoEnemyBehaviorPhase::Dead;
}

void UReEchoEnemyLogicComponent::RestoreSnapshot(const FReEchoEnemyLogicSnapshot& InSnapshot)
{
	if (!bInitialized || InSnapshot.Archetype != Definition.Archetype)
	{
		return;
	}
	State = InSnapshot;
	State.AttackCooldownRemainingSeconds = FMath::Max(0.0f, State.AttackCooldownRemainingSeconds);
	State.FuseRemainingSeconds = FMath::Max(0.0f, State.FuseRemainingSeconds);
	State.HitReactionRemainingSeconds = FMath::Max(0.0f, State.HitReactionRemainingSeconds);
	State.AttackSequence = FMath::Max<int64>(0, State.AttackSequence);
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
	NotifyHurt(Event.AppliedDamage, Event.SourceWorldLocation, Event.WorldLocation);
}

void UReEchoEnemyLogicComponent::HandleCombatDeath(const FReEchoDamageEvent& Event)
{
	if (Event.Target == GetOwner())
	{
		NotifyDeath();
	}
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
