#include "Weapons/ReEchoWeaponGeometry.h"

#include "Combat/ReEchoCombatTarget.h"
#include "EngineUtils.h"

bool ReEchoWeaponGeometry::IsInsideMeleeArc(
    const FVector& Origin, const FVector& Forward, const FVector& Target, const float RangeCm, const float ArcDegrees)
{
	const FVector ToTarget = (Target - Origin).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return true;
	}
	if (FVector::Dist2D(Origin, Target) > RangeCm)
	{
		return false;
	}
	if (ArcDegrees >= 359.9f)
	{
		return true;
	}
	const FVector Forward2D =
	    Forward.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal2D();
	const float Dot = FMath::Clamp(FVector::DotProduct(Forward2D, ToTarget), -1.0f, 1.0f);
	return FMath::RadiansToDegrees(FMath::Acos(Dot)) <= ArcDegrees * 0.5f + KINDA_SMALL_NUMBER;
}

bool ReEchoWeaponGeometry::IsInsideMeleeSphere(const FVector& Origin, const FVector& Target, const float RangeCm)
{
	return RangeCm >= 0.0f && FVector::DistSquared(Origin, Target) <= FMath::Square(RangeCm);
}

TArray<FVector> ReEchoWeaponGeometry::BuildProjectileDirections(const FVector& Forward,
                                                                const int32 ProjectileCount,
                                                                const float SpreadDegrees)
{
	const int32 Count = FMath::Max(1, ProjectileCount);
	const FVector Forward2D =
	    Forward.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal2D();
	TArray<FVector> Directions;
	Directions.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = Count == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(Count - 1);
		Directions.Add(Forward2D.RotateAngleAxis((Alpha - 0.5f) * SpreadDegrees, FVector::UpVector).GetSafeNormal());
	}
	return Directions;
}

TArray<AActor*> ReEchoWeaponGeometry::FindMeleeTargets(UWorld& World,
                                                       const FReEchoAttackIdentity& Attack,
                                                       const FVector& Origin,
                                                       const FVector& Forward,
                                                       const float RangeCm,
                                                       const float ArcDegrees)
{
	TArray<AActor*> Result;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || !Target->IsCombatTargetAlive() || !ReEchoCombatRelations::CanDamage(Attack, *Candidate) ||
		    !IsInsideMeleeArc(Origin, Forward, Target->GetCombatTargetLocation(), RangeCm, ArcDegrees))
		{
			continue;
		}
		Result.Add(Candidate);
	}
	Result.Sort(
	    [](const AActor& Left, const AActor& Right)
	    {
		    const IReEchoCombatTarget* LeftTarget = Cast<IReEchoCombatTarget>(&Left);
		    const IReEchoCombatTarget* RightTarget = Cast<IReEchoCombatTarget>(&Right);
		    return LeftTarget && RightTarget &&
		           LeftTarget->GetCombatTargetTieBreakIndex() < RightTarget->GetCombatTargetTieBreakIndex();
	    });
	return Result;
}

TArray<AActor*> ReEchoWeaponGeometry::FindMeleeTargetsInSphere(UWorld& World,
                                                               const FReEchoAttackIdentity& Attack,
                                                               const FVector& Origin,
                                                               const float RangeCm)
{
	TArray<AActor*> Result;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate);
		if (!Target || !Target->IsCombatTargetAlive() || !ReEchoCombatRelations::CanDamage(Attack, *Candidate) ||
		    !IsInsideMeleeSphere(Origin, Target->GetCombatTargetLocation(), RangeCm))
		{
			continue;
		}
		Result.Add(Candidate);
	}
	Result.Sort(
	    [](const AActor& Left, const AActor& Right)
	    {
		    const IReEchoCombatTarget* LeftTarget = Cast<IReEchoCombatTarget>(&Left);
		    const IReEchoCombatTarget* RightTarget = Cast<IReEchoCombatTarget>(&Right);
		    return LeftTarget && RightTarget &&
		           LeftTarget->GetCombatTargetTieBreakIndex() < RightTarget->GetCombatTargetTieBreakIndex();
	    });
	return Result;
}
