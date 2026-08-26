#include "Combat/ReEchoHitResolver.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoCombatantComponent.h"

#if !UE_BUILD_SHIPPING
DEFINE_LOG_CATEGORY_STATIC(LogReEchoRangedCritResolverTrace, Log, All);

namespace
{
bool IsRangedWeaponTrace(const FReEchoAttackIdentity& Attack)
{
	return Attack.WeaponId == TEXT("W_J_08") || Attack.WeaponId == TEXT("W_J_09");
}
} // namespace
#endif

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
#if !UE_BUILD_SHIPPING
	const bool bTraceRangedWeapon = IsRangedWeaponTrace(Candidate.Attack);
	if (bTraceRangedWeapon)
	{
		UE_LOG(LogReEchoRangedCritResolverTrace,
		       Warning,
		       TEXT("[RangedCritTrace] ResolverEnter weapon=%s sequence=%lld source=%s target=%s rawDamage=%.3f "
		            "critical=%d sourceRulesApplied=%d sourceFaction=%d allowSameFaction=%d"),
		       *Candidate.Attack.WeaponId.ToString(),
		       static_cast<long long>(Candidate.Attack.Sequence),
		       *GetNameSafe(Candidate.Attack.Source.Get()),
		       *GetNameSafe(Candidate.Target),
		       Candidate.RawDamage,
		       Candidate.bCritical ? 1 : 0,
		       Candidate.bSourceRulesApplied ? 1 : 0,
		       static_cast<int32>(Candidate.Attack.SourceFaction),
		       Candidate.bAllowSameFactionDamage ? 1 : 0);
	}
#endif
	if (!Candidate.bSourceRulesApplied)
	{
		if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Candidate.Attack.Source.Get()))
		{
			SourceRules->ModifyOutgoingHit(Candidate);
		}
		Candidate.bSourceRulesApplied = true;
	}
#if !UE_BUILD_SHIPPING
	if (bTraceRangedWeapon)
	{
		UE_LOG(LogReEchoRangedCritResolverTrace,
		       Warning,
		       TEXT("[RangedCritTrace] ResolverAfterSourceRules weapon=%s sequence=%lld rawDamage=%.3f critical=%d "
		            "element=%d"),
		       *Candidate.Attack.WeaponId.ToString(),
		       static_cast<long long>(Candidate.Attack.Sequence),
		       Candidate.RawDamage,
		       Candidate.bCritical ? 1 : 0,
		       static_cast<int32>(Candidate.Element));
	}
#endif
	FReEchoHitResolved Result;
	Result.Attack = Candidate.Attack;
	Result.Target = Candidate.Target;
	Result.DamageSource = Candidate.DamageSource;
	Result.Element = Candidate.Element;
	Result.ReactionBehaviorId = Candidate.ReactionBehaviorId;
	Result.bCritical = Candidate.bCritical;
	Result.HitLocation = Candidate.HitLocation;

	IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate.Target);
	UReEchoCombatantComponent* TargetCombatant = Target ? Target->GetCombatTargetCombatant() : nullptr;
	const bool bTargetAlive = Target && Target->IsCombatTargetAlive();
	const bool bCanDamage =
	    Candidate.Target &&
	    ReEchoCombatRelations::CanDamage(Candidate.Attack, *Candidate.Target, Candidate.bAllowSameFactionDamage);
	if (!Candidate.Target || !Target || !TargetCombatant || !bTargetAlive || Candidate.RawDamage <= 0.0f || !bCanDamage)
	{
		Result.bBlocked = true;
#if !UE_BUILD_SHIPPING
		if (bTraceRangedWeapon)
		{
			UE_LOG(LogReEchoRangedCritResolverTrace,
			       Warning,
			       TEXT("[RangedCritTrace] ResolverRejected weapon=%s sequence=%lld hasTarget=%d combatTarget=%d "
			            "combatant=%d alive=%d positiveRaw=%d canDamage=%d rawDamage=%.3f"),
			       *Candidate.Attack.WeaponId.ToString(),
			       static_cast<long long>(Candidate.Attack.Sequence),
			       Candidate.Target ? 1 : 0,
			       Target ? 1 : 0,
			       TargetCombatant ? 1 : 0,
			       bTargetAlive ? 1 : 0,
			       Candidate.RawDamage > 0.0f ? 1 : 0,
			       bCanDamage ? 1 : 0,
			       Candidate.RawDamage);
		}
#endif
		return Result;
	}

	Result.RawDamage = FMath::Max(0.0f, Target->ModifyIncomingRawDamage(Candidate));
	const bool bWasAlive = TargetCombatant->IsAlive();
	const float TargetHealthBefore = TargetCombatant->GetSnapshot().CurrentHealth;
	Result.AppliedDamage = FReEchoHitResolverAccess::ApplyFinalDamage(
	    *TargetCombatant, Result.RawDamage, Candidate.Attack, Candidate.DamageSource);
	Result.bBlocked = Result.AppliedDamage <= 0.0f;
	Result.bKilled = bWasAlive && !TargetCombatant->IsAlive();
#if !UE_BUILD_SHIPPING
	if (bTraceRangedWeapon)
	{
		UE_LOG(LogReEchoRangedCritResolverTrace,
		       Warning,
		       TEXT("[RangedCritTrace] ResolverApplied weapon=%s sequence=%lld target=%s sourceAdjustedRaw=%.3f "
		            "incomingAdjustedRaw=%.3f applied=%.3f healthBefore=%.3f healthAfter=%.3f critical=%d "
		            "blocked=%d killed=%d"),
		       *Candidate.Attack.WeaponId.ToString(),
		       static_cast<long long>(Candidate.Attack.Sequence),
		       *GetNameSafe(Candidate.Target),
		       Candidate.RawDamage,
		       Result.RawDamage,
		       Result.AppliedDamage,
		       TargetHealthBefore,
		       TargetCombatant->GetSnapshot().CurrentHealth,
		       Result.bCritical ? 1 : 0,
		       Result.bBlocked ? 1 : 0,
		       Result.bKilled ? 1 : 0);
	}
#endif

	FReEchoDamageEvent Event;
	Event.Attack = Result.Attack;
	Event.Target = Result.Target;
	Event.RawDamage = Result.RawDamage;
	Event.AppliedDamage = Result.AppliedDamage;
	Event.DamageSource = Result.DamageSource;
	Event.Element = Result.Element;
	Event.ReactionBehaviorId = Result.ReactionBehaviorId;
	Event.bCritical = Result.bCritical;
	Event.bBlocked = Result.bBlocked;
	Event.bFatal = Result.bKilled;
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
