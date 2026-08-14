#include "Combat/ReEchoCombatTarget.h"

#include "EngineUtils.h"

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
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || Candidate == Owner || !Target->IsCombatTargetAlive())
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
