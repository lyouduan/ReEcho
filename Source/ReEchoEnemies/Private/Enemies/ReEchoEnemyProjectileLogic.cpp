#include "Enemies/ReEchoEnemyProjectileLogic.h"

namespace
{
bool IsValidProjectileDefinition(const FReEchoEnemyProjectileDefinition& Definition)
{
	return !Definition.InitialLocation.ContainsNaN() && !Definition.Direction.ContainsNaN() &&
	       FMath::IsFinite(Definition.SpeedCmPerSecond) && Definition.SpeedCmPerSecond > 0.0f &&
	       FMath::IsFinite(Definition.MaxRangeCm) && Definition.MaxRangeCm > 0.0f &&
	       !Definition.Direction.IsNearlyZero();
}
} // namespace

bool FReEchoEnemyProjectileLogic::Initialize(const FReEchoEnemyProjectileDefinition& Definition,
                                             FReEchoEnemyProjectileSnapshot& OutSnapshot)
{
	OutSnapshot = {};
	if (!IsValidProjectileDefinition(Definition))
	{
		return false;
	}
	OutSnapshot.Location = Definition.InitialLocation;
	OutSnapshot.Direction = Definition.Direction.GetSafeNormal();
	OutSnapshot.bActive = true;
	return true;
}

bool FReEchoEnemyProjectileLogic::RestoreSnapshot(const FReEchoEnemyProjectileDefinition& Definition,
                                                  const FReEchoEnemyProjectileSnapshot& SavedSnapshot,
                                                  FReEchoEnemyProjectileSnapshot& OutSnapshot)
{
	OutSnapshot = {};
	if (!IsValidProjectileDefinition(Definition) || SavedSnapshot.Location.ContainsNaN() ||
	    SavedSnapshot.Direction.ContainsNaN() || SavedSnapshot.Direction.IsNearlyZero() ||
	    !FMath::IsFinite(SavedSnapshot.DistanceTravelledCm) || SavedSnapshot.DistanceTravelledCm < 0.0f ||
	    SavedSnapshot.DistanceTravelledCm > Definition.MaxRangeCm + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	OutSnapshot = SavedSnapshot;
	OutSnapshot.Direction = SavedSnapshot.Direction.GetSafeNormal();
	OutSnapshot.DistanceTravelledCm =
	    FMath::Clamp(SavedSnapshot.DistanceTravelledCm, 0.0f, Definition.MaxRangeCm);
	OutSnapshot.bActive = SavedSnapshot.bActive &&
	                      OutSnapshot.DistanceTravelledCm < Definition.MaxRangeCm - KINDA_SMALL_NUMBER;
	return true;
}

FReEchoEnemyProjectileAdvanceResult FReEchoEnemyProjectileLogic::Advance(
    const FReEchoEnemyProjectileDefinition& Definition,
    const float DeltaSeconds,
    FReEchoEnemyProjectileSnapshot& InOutSnapshot)
{
	FReEchoEnemyProjectileAdvanceResult Result;
	Result.PreviousLocation = InOutSnapshot.Location;
	Result.NewLocation = InOutSnapshot.Location;
	if (!IsValidProjectileDefinition(Definition) || !InOutSnapshot.bActive ||
	    !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
	{
		return Result;
	}

	const float RemainingRangeCm =
	    FMath::Max(0.0f, Definition.MaxRangeCm - InOutSnapshot.DistanceTravelledCm);
	const float RequestedDistanceCm = Definition.SpeedCmPerSecond * DeltaSeconds;
	const float SegmentDistanceCm = FMath::Min(RequestedDistanceCm, RemainingRangeCm);
	InOutSnapshot.Direction = InOutSnapshot.Direction.GetSafeNormal();
	InOutSnapshot.Location += InOutSnapshot.Direction * SegmentDistanceCm;
	InOutSnapshot.DistanceTravelledCm += SegmentDistanceCm;
	Result.NewLocation = InOutSnapshot.Location;
	Result.SegmentDistanceCm = SegmentDistanceCm;
	Result.bMoved = SegmentDistanceCm > KINDA_SMALL_NUMBER;

	if (InOutSnapshot.DistanceTravelledCm >= Definition.MaxRangeCm - KINDA_SMALL_NUMBER)
	{
		InOutSnapshot.DistanceTravelledCm = Definition.MaxRangeCm;
		InOutSnapshot.bActive = false;
		Result.bExpiredByRange = true;
	}
	return Result;
}
