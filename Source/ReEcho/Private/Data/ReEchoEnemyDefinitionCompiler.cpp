#include "Data/ReEchoEnemyDefinitionCompiler.h"

#include "Data/ReEchoCsvDataRegistry.h"

namespace
{
bool ParseArchetype(const FName Value, EReEchoEnemyArchetype& OutArchetype)
{
	if (Value == TEXT("Grunt"))
	{
		OutArchetype = EReEchoEnemyArchetype::Grunt;
		return true;
	}
	if (Value == TEXT("Shield"))
	{
		OutArchetype = EReEchoEnemyArchetype::Shield;
		return true;
	}
	if (Value == TEXT("Bomber"))
	{
		OutArchetype = EReEchoEnemyArchetype::Bomber;
		return true;
	}
	if (Value == TEXT("Boss"))
	{
		OutArchetype = EReEchoEnemyArchetype::Boss;
		return true;
	}
	if (Value == TEXT("Slime"))
	{
		OutArchetype = EReEchoEnemyArchetype::Slime;
		return true;
	}
	if (Value == TEXT("Ranged"))
	{
		OutArchetype = EReEchoEnemyArchetype::Ranged;
		return true;
	}
	if (Value == TEXT("Elite"))
	{
		OutArchetype = EReEchoEnemyArchetype::Elite;
		return true;
	}
	return false;
}

bool ParseTargetingMode(const FName Value, EReEchoBossTargetingMode& OutMode)
{
	if (Value == TEXT("LockedLocation"))
	{
		OutMode = EReEchoBossTargetingMode::LockedLocation;
		return true;
	}
	if (Value == TEXT("LockedDirection"))
	{
		OutMode = EReEchoBossTargetingMode::LockedDirection;
		return true;
	}
	if (Value == TEXT("Self"))
	{
		OutMode = EReEchoBossTargetingMode::Self;
		return true;
	}
	return false;
}

bool ParseLockTiming(const FName Value, EReEchoBossLockTiming& OutTiming)
{
	if (Value == TEXT("WindupStart"))
	{
		OutTiming = EReEchoBossLockTiming::WindupStarted;
		return true;
	}
	if (Value == TEXT("WindupEnd"))
	{
		OutTiming = EReEchoBossLockTiming::WindupEnded;
		return true;
	}
	if (Value == TEXT("Interval"))
	{
		OutTiming = EReEchoBossLockTiming::Interval;
		return true;
	}
	return false;
}
} // namespace

bool ReEchoEnemyDefinitionCompiler::Compile(const FReEchoCsvDataSnapshot& Snapshot,
                                            const FName EnemyId,
                                            FReEchoEnemyDefinition& OutDefinition,
                                            FString& OutError)
{
	OutDefinition = FReEchoEnemyDefinition{};
	OutError.Reset();
	const FReEchoCsvEnemyRow* Row = Snapshot.FindEnabledEnemy(EnemyId);
	if (!Row)
	{
		OutError =
		    FString::Printf(TEXT("EnemyId '%s' is unknown or disabled in the run data snapshot."), *EnemyId.ToString());
		return false;
	}

	if (!ParseArchetype(Row->Archetype, OutDefinition.Archetype))
	{
		OutError = FString::Printf(
		    TEXT("Enemy '%s' has unsupported archetype '%s'."), *EnemyId.ToString(), *Row->Archetype.ToString());
		return false;
	}
	OutDefinition.PresentationId = Row->PresentationId;
	OutDefinition.MaxHealth = Row->MaxHealth;
	OutDefinition.MoveSpeedCmPerSecond = Row->MoveSpeedCmPerSecond;
	OutDefinition.CollisionRadiusCm = Row->CollisionRadiusCm;
	OutDefinition.CollisionHalfHeightCm = Row->CollisionHalfHeightCm;
	OutDefinition.ContactDamage = Row->ContactDamage;
	OutDefinition.AttackIntervalSeconds = Row->AttackIntervalSeconds;
	OutDefinition.ContactRangeCm = Row->ContactRangeCm;
	OutDefinition.MovementStopDistanceCm = Row->MovementStopDistanceCm;
	OutDefinition.HitReactionDurationSeconds = Row->HitReactionDurationSeconds;
	OutDefinition.KnockbackSpeedCmPerSecond = Row->KnockbackSpeedCmPerSecond;
	OutDefinition.KnockbackDrag = Row->KnockbackDrag;
	OutDefinition.BomberTriggerRadiusCm = Row->TriggerRadiusCm;
	OutDefinition.BomberDamageRadiusCm = Row->DamageRadiusCm;
	OutDefinition.BomberFuseDurationSeconds = Row->FuseSeconds;
	OutDefinition.bUsesDirectionalShield = OutDefinition.Archetype == EReEchoEnemyArchetype::Shield ||
	                                       OutDefinition.Archetype == EReEchoEnemyArchetype::Elite;

	// Plan 65 temporary bridge until the authoritative EnemyPhases worksheet is available. Every production enemy
	// profile now owns a Phase2 animation set, so they share the same two-health-reductions threshold for PIE tuning.
	OutDefinition.Phase2.Id = FName(*(EnemyId.ToString() + TEXT("_PHASE2")));
	OutDefinition.Phase2.TriggerRangeCm = 300.0f;
	OutDefinition.Phase2.RequiredAttackCount = 2;
	OutDefinition.Phase2.TransformSeconds = 1.0f;
	OutDefinition.Phase2.AnimationSetId = TEXT("Phase2");
	OutDefinition.Phase2.bEnabled = true;

	for (const FReEchoCsvEnemyAbilityRow& AbilityRow : Row->Abilities)
	{
		FReEchoEnemyAbilityDefinition Ability;
		Ability.Id = AbilityRow.Id;
		Ability.BehaviorId = AbilityRow.BehaviorId;
		Ability.SequenceOrder = AbilityRow.SequenceOrder;
		Ability.bEnabled = AbilityRow.bEnabled;
		Ability.Damage = AbilityRow.Damage;
		Ability.WindupSeconds = AbilityRow.WindupSeconds;
		Ability.ActiveSeconds = AbilityRow.ActiveSeconds;
		Ability.RecoverySeconds = AbilityRow.RecoverySeconds;
		Ability.CooldownSeconds = AbilityRow.CooldownSeconds;
		Ability.MinRangeCm = AbilityRow.MinRangeCm;
		Ability.MaxRangeCm = AbilityRow.MaxRangeCm;
		Ability.RadiusCm = AbilityRow.RadiusCm;
		Ability.WidthCm = AbilityRow.WidthCm;
		Ability.LengthCm = AbilityRow.LengthCm;
		Ability.ProjectileSpeedCmPerSecond = AbilityRow.ProjectileSpeedCmPerSecond;
		Ability.TeleportOffsetCm = AbilityRow.TeleportOffsetCm;
		Ability.CleanseIntervalSeconds = AbilityRow.CleanseIntervalSeconds;
		Ability.ImmunitySeconds = AbilityRow.ImmunitySeconds;
		if (!ParseTargetingMode(AbilityRow.TargetingMode, Ability.TargetingMode))
		{
			OutError = FString::Printf(TEXT("Enemy ability '%s' has unsupported targeting mode '%s'."),
			                           *AbilityRow.Id.ToString(),
			                           *AbilityRow.TargetingMode.ToString());
			return false;
		}
		if (!ParseLockTiming(AbilityRow.LockTiming, Ability.LockTiming))
		{
			OutError = FString::Printf(TEXT("Enemy ability '%s' has unsupported lock timing '%s'."),
			                           *AbilityRow.Id.ToString(),
			                           *AbilityRow.LockTiming.ToString());
			return false;
		}
		OutDefinition.Abilities.Add(Ability);
	}
	for (const FReEchoCsvBossPhaseRow& PhaseRow : Row->BossPhases)
	{
		FReEchoBossPhaseDefinition Phase;
		Phase.Id = PhaseRow.Id;
		Phase.PhaseIndex = PhaseRow.PhaseIndex;
		Phase.TriggerSeconds = PhaseRow.TriggerSeconds;
		if (PhaseRow.EchoPolicy == TEXT("DestroyEncounterEchoes"))
		{
			Phase.EchoPolicy = EReEchoBossEchoPolicy::RetireEncounterEchoes;
		}
		else
		{
			OutError = FString::Printf(TEXT("Boss phase '%s' has unsupported echo policy '%s'."),
			                           *PhaseRow.Id.ToString(),
			                           *PhaseRow.EchoPolicy.ToString());
			return false;
		}
		Phase.PhysicalAttackMultiplier = PhaseRow.PhysicalAttackMultiplier;
		Phase.ElementalAttackMultiplier = PhaseRow.ElementalAttackMultiplier;
		Phase.AttackSpeedMultiplier = PhaseRow.AttackSpeedMultiplier;
		Phase.MovementSpeedMultiplier = PhaseRow.MovementSpeedMultiplier;
		if (PhaseRow.RefillHealthPolicy == TEXT("None"))
		{
			Phase.RefillHealthPolicy = EReEchoBossRefillHealthPolicy::None;
		}
		else if (PhaseRow.RefillHealthPolicy == TEXT("RefillToMaximum"))
		{
			Phase.RefillHealthPolicy = EReEchoBossRefillHealthPolicy::RefillToMaximum;
		}
		else
		{
			OutError = FString::Printf(TEXT("Boss phase '%s' has unsupported refill policy '%s'."),
			                           *PhaseRow.Id.ToString(),
			                           *PhaseRow.RefillHealthPolicy.ToString());
			return false;
		}
		Phase.bEnabled = PhaseRow.bEnabled;
		OutDefinition.BossPhases.Add(Phase);
	}

	if (OutDefinition.MaxHealth <= 0.0f || OutDefinition.MoveSpeedCmPerSecond < 0.0f ||
	    OutDefinition.ContactDamage < 0.0f || OutDefinition.AttackIntervalSeconds < 0.0f)
	{
		OutError = FString::Printf(TEXT("Enemy '%s' compiled to an invalid runtime definition."), *EnemyId.ToString());
		return false;
	}
	return true;
}
