#include "Combat/ReEchoCombatTarget.h"

#include "EngineUtils.h"

EReEchoCombatFaction ReEchoCombatRelations::ResolveActorFaction(const AActor* Actor)
{
	const IReEchoCombatAffiliation* Affiliation = Actor ? Cast<IReEchoCombatAffiliation>(Actor) : nullptr;
	return Affiliation ? Affiliation->GetCombatFaction() : EReEchoCombatFaction::Unaligned;
}

EReEchoDamageSource ReEchoCombatRelations::ResolveActorDamageSource(const AActor* Actor,
                                                                    const EReEchoDamageSource Fallback)
{
	const IReEchoCombatAffiliation* Affiliation = Actor ? Cast<IReEchoCombatAffiliation>(Actor) : nullptr;
	return Affiliation ? Affiliation->GetCombatDamageSource() : Fallback;
}

bool ReEchoCombatRelations::CanDamage(const EReEchoCombatFaction SourceFaction,
                                      const EReEchoCombatFaction TargetFaction,
                                      const bool bAllowSameFactionDamage)
{
	if (bAllowSameFactionDamage)
	{
		return true;
	}
	if (SourceFaction == EReEchoCombatFaction::Unaligned || TargetFaction == EReEchoCombatFaction::Unaligned)
	{
		return true;
	}
	return SourceFaction != TargetFaction;
}

bool ReEchoCombatRelations::CanDamage(const FReEchoAttackIdentity& Attack,
                                      const AActor& Target,
                                      const bool bAllowSameFactionDamage)
{
	if (bAllowSameFactionDamage)
	{
		return true;
	}
	if (Attack.Source.Get() == &Target)
	{
		return false;
	}
	const EReEchoCombatFaction SourceFaction =
	    Attack.SourceFaction != EReEchoCombatFaction::Unaligned ? Attack.SourceFaction
	                                                          : ResolveActorFaction(Attack.Source.Get());
	return CanDamage(SourceFaction, ResolveActorFaction(&Target));
}

AActor* UReEchoTargetingComponent::FindNearestTarget(const float RangeCm)
{
	CurrentTarget.Reset();
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || RangeCm <= 0.0f)
	{
		return nullptr;
	}

	const float MaxDistanceSquared = FMath::Square(RangeCm);
	float BestDistanceSquared = MaxDistanceSquared;
	int32 BestTieBreak = MAX_int32;
	FReEchoAttackIdentity TargetingIdentity;
	TargetingIdentity.Source = Owner;
	TargetingIdentity.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(Owner);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || !Target->IsCombatTargetAlive() ||
		    !ReEchoCombatRelations::CanDamage(TargetingIdentity, *Candidate))
		{
			continue;
		}
		const float DistanceSquared =
		    FVector::DistSquared2D(Owner->GetActorLocation(), Target->GetCombatTargetLocation());
		const int32 TieBreak = Target->GetCombatTargetTieBreakIndex();
		if (DistanceSquared > BestDistanceSquared + KINDA_SMALL_NUMBER ||
		    (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared) && TieBreak >= BestTieBreak))
		{
			continue;
		}
		CurrentTarget = Candidate;
		BestDistanceSquared = DistanceSquared;
		BestTieBreak = TieBreak;
	}
	return CurrentTarget.Get();
}
