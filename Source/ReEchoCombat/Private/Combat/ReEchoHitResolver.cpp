#include "Combat/ReEchoHitResolver.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoCombatantComponent.h"

class FReEchoHitResolverAccess
{
public:
	static float ApplyFinalDamage(UReEchoCombatantComponent& Combatant,
	                              const float Damage,
	                              const FReEchoAttackIdentity& Attack,
	                              const EReEchoDamageSource DamageSource)
	{
		return Combatant.ApplyFinalDamage(Damage, Attack, DamageSource);
	}
};

FReEchoHitResolved ReEchoHitResolver::ResolvePhysicalHit(const FReEchoHitIntent& Intent)
{
	FReEchoHitIntent Candidate = Intent;
	if (!Candidate.bSourceRulesApplied)
	{
		if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Candidate.Attack.Source.Get()))
		{
			SourceRules->ModifyOutgoingHit(Candidate);
		}
		Candidate.bSourceRulesApplied = true;
	}
	FReEchoHitResolved Result;
	Result.Attack = Candidate.Attack;
	Result.Target = Candidate.Target;
	Result.DamageSource = Candidate.DamageSource;
	Result.Element = Candidate.Element;
	Result.bCritical = Candidate.bCritical;
	Result.HitLocation = Candidate.HitLocation;

	IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate.Target);
	UReEchoCombatantComponent* TargetCombatant = Target ? Target->GetCombatTargetCombatant() : nullptr;
	if (!Candidate.Target || !Target || !TargetCombatant || !Target->IsCombatTargetAlive() ||
	    Candidate.RawDamage <= 0.0f ||
	    !ReEchoCombatRelations::CanDamage(Candidate.Attack, *Candidate.Target, Candidate.bAllowSameFactionDamage))
	{
		Result.bBlocked = true;
		return Result;
	}

	Result.RawDamage = FMath::Max(0.0f, Target->ModifyIncomingRawDamage(Candidate));
	const bool bWasAlive = TargetCombatant->IsAlive();
	Result.AppliedDamage = FReEchoHitResolverAccess::ApplyFinalDamage(
	    *TargetCombatant, Result.RawDamage, Candidate.Attack, Candidate.DamageSource);
	Result.bBlocked = Result.AppliedDamage <= 0.0f;
	Result.bKilled = bWasAlive && !TargetCombatant->IsAlive();

	FReEchoDamageEvent Event;
	Event.Attack = Result.Attack;
	Event.Target = Result.Target;
	Event.RawDamage = Result.RawDamage;
	Event.AppliedDamage = Result.AppliedDamage;
	Event.DamageSource = Result.DamageSource;
	Event.Element = Result.Element;
	Event.bCritical = Result.bCritical;
	Event.bBlocked = Result.bBlocked;
	Event.WorldLocation = Result.HitLocation;
	Event.SourceWorldLocation = Candidate.SourceLocation;

	if (UReEchoCombatEventsComponent* TargetEvents =
	        Candidate.Target->FindComponentByClass<UReEchoCombatEventsComponent>())
	{
		TargetEvents->PublishHurt(Event);
		if (Result.bKilled)
		{
			TargetCombatant->ResetElementState();
			TargetEvents->PublishDeath(Event);
			Target->NotifyDefeated(Candidate.DamageSource);
		}
	}
	// Resolve the weak source exactly once. Delayed projectiles remain authoritative after their source leaves the
	// world, but source-side feedback is intentionally skipped once that source is no longer valid.
	if (AActor* Source = Candidate.Attack.Source.Get())
	{
		if (UReEchoCombatEventsComponent* SourceEvents = Source->FindComponentByClass<UReEchoCombatEventsComponent>())
		{
			if (Result.AppliedDamage > 0.0f)
			{
				SourceEvents->PublishHit(Event);
			}
			if (Result.bKilled)
			{
				SourceEvents->PublishKill(Event);
				if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Source))
				{
					SourceRules->NotifyKillResolved();
				}
			}
		}
	}
	return Result;
}
