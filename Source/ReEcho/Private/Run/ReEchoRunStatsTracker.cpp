#include "Run/ReEchoRunStatsTracker.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTypes.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Player/ReEchoPlayerPawn.h"

void UReEchoRunStatsSubsystem::ResetRunStats()
{
	Stats.Reset();
}

void UReEchoRunStatsSubsystem::RecordEnemyHurt(const FReEchoDamageEvent& Event)
{
	const float Applied = FMath::Max(0.0f, Event.AppliedDamage);
	if (Applied <= 0.0f)
	{
		return;
	}

	Stats.MaxSingleHitDamage = FMath::Max(Stats.MaxSingleHitDamage, Applied);

	if (Event.DamageSource == EReEchoDamageSource::Echo)
	{
		Stats.EchoDamageTotal += Applied;
		return;
	}
	if (Event.DamageSource == EReEchoDamageSource::Player)
	{
		Stats.PlayerDamageTotal += Applied;
		return;
	}
	if (Event.DamageSource != EReEchoDamageSource::Reaction)
	{
		return;
	}

	// Element reaction damage is attributed to whoever started it; unattributed reactions only
	// contribute to the highest single hit.
	const AActor* Source = Event.Attack.Source.Get();
	if (Cast<AReEchoEchoActor>(Source) != nullptr)
	{
		Stats.EchoDamageTotal += Applied;
		return;
	}
	if (Cast<AReEchoPlayerPawn>(Source) != nullptr)
	{
		Stats.PlayerDamageTotal += Applied;
	}
}

void UReEchoRunStatsSubsystem::RecordEnemyDeath(const FReEchoDamageEvent& Event)
{
	if (Event.DamageSource == EReEchoDamageSource::Enemy)
	{
		return;
	}
	++Stats.KillTotal;
}

void UReEchoRunStatsSubsystem::RecordElementReaction()
{
	++Stats.ReactionTotal;
}
