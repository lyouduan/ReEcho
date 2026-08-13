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
	FReEchoHitResolved Result;
	Result.Attack = Intent.Attack;
	Result.Target = Intent.Target;
	Result.DamageSource = Intent.DamageSource;
	Result.Element = Intent.Element;
	Result.bCritical = Intent.bCritical;
	Result.HitLocation = Intent.HitLocation;

	IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Intent.Target);
	UReEchoCombatantComponent* TargetCombatant = Target ? Target->GetCombatTargetCombatant() : nullptr;
	if (!Target || !TargetCombatant || !Target->IsCombatTargetAlive() || Intent.RawDamage <= 0.0f)
	{
		Result.bBlocked = true;
		return Result;
	}

	Result.RawDamage = FMath::Max(0.0f, Target->ModifyIncomingRawDamage(Intent));
	const bool bWasAlive = TargetCombatant->IsAlive();
	Result.AppliedDamage = FReEchoHitResolverAccess::ApplyFinalDamage(
	    *TargetCombatant, Result.RawDamage, Intent.Attack, Intent.DamageSource);
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
	Event.SourceWorldLocation = Intent.SourceLocation;

	if (UReEchoCombatEventsComponent* TargetEvents =
	        Intent.Target->FindComponentByClass<UReEchoCombatEventsComponent>())
	{
		TargetEvents->PublishHurt(Event);
		if (Result.bKilled)
		{
			TargetCombatant->ResetElementState();
			TargetEvents->PublishDeath(Event);
		}
	}
	if (AActor* Source = Intent.Attack.Source.Get())
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
			}
		}
	}
	return Result;
}
