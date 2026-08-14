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
                                                       AActor* Source,
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
		if (!Target || Candidate == Source || !Target->IsCombatTargetAlive() ||
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
