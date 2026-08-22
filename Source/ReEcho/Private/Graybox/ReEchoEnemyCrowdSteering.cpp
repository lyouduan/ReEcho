#include "Graybox/ReEchoEnemyCrowdSteering.h"

namespace
{
constexpr float GoldenAngleRadians = 2.39996323f;
constexpr float SeparationPaddingCm = 20.0f;
constexpr float NeighborLookAheadScale = 2.5f;
constexpr float SeparationWeight = 1.5f;
constexpr float TangentWeight = 0.65f;
constexpr float BlockedRecoverySeconds = 0.35f;

FVector ResolveStableRadial(const int32 SpawnIndex)
{
	const float Angle = static_cast<float>(FMath::Abs(SpawnIndex)) * GoldenAngleRadians;
	return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
}
}

FVector FReEchoEnemyCrowdSteering::ResolveMovement(const FReEchoEnemyCrowdSteeringInput& Input)
{
	const float DesiredDistance = Input.DesiredMovementDelta.Size2D();
	if (DesiredDistance <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const FVector StableRadial = ResolveStableRadial(Input.SpawnIndex);
	FVector SlotLocation = Input.TargetLocation + StableRadial * FMath::Max(0.0f, Input.PreferredTargetDistanceCm);
	SlotLocation.Z = Input.SelfLocation.Z;
	FVector SeekDirection = (SlotLocation - Input.SelfLocation).GetSafeNormal2D();
	if (SeekDirection.IsNearlyZero())
	{
		SeekDirection = Input.DesiredMovementDelta.GetSafeNormal2D();
	}

	FVector Separation = FVector::ZeroVector;
	FVector BlockingCenter = FVector::ZeroVector;
	int32 BlockingCount = 0;
	for (const FReEchoEnemyCrowdNeighbor& Neighbor : Input.Neighbors)
	{
		FVector Away = Input.SelfLocation - Neighbor.Location;
		Away.Z = 0.0f;
		const float Distance = Away.Size2D();
		const float DesiredSpacing = Input.SelfRadiusCm + Neighbor.RadiusCm + SeparationPaddingCm;
		if (Distance < DesiredSpacing)
		{
			const FVector AwayDirection = Distance > KINDA_SMALL_NUMBER
			                                  ? Away / Distance
			                                  : ResolveStableRadial(Input.SpawnIndex - Neighbor.SpawnIndex);
			Separation += AwayDirection * (1.0f - Distance / FMath::Max(DesiredSpacing, 1.0f));
		}

		const FVector ToNeighbor = Neighbor.Location - Input.SelfLocation;
		const float ForwardAlignment = FVector::DotProduct(ToNeighbor.GetSafeNormal2D(), SeekDirection);
		if (ForwardAlignment > 0.45f && ToNeighbor.Size2D() < DesiredSpacing * NeighborLookAheadScale)
		{
			BlockingCenter += Neighbor.Location;
			++BlockingCount;
		}
	}

	FVector Steering = SeekDirection + Separation.GetClampedToMaxSize(1.0f) * SeparationWeight;
	if (BlockingCount > 0)
	{
		const FVector ToBlocker =
		    (BlockingCenter / static_cast<float>(BlockingCount) - Input.SelfLocation).GetSafeNormal2D();
		FVector Tangent(-ToBlocker.Y, ToBlocker.X, 0.0f);
		const int32 RecoveryStep = FMath::FloorToInt(Input.BlockedSeconds / BlockedRecoverySeconds);
		if (((Input.SpawnIndex + RecoveryStep) & 1) != 0)
		{
			Tangent *= -1.0f;
		}
		const float RecoveryScale = Input.BlockedSeconds >= BlockedRecoverySeconds ? 1.5f : 1.0f;
		Steering += Tangent * TangentWeight * RecoveryScale;
	}

	const FVector Direction = Steering.GetSafeNormal2D();
	return (Direction.IsNearlyZero() ? SeekDirection : Direction) * DesiredDistance;
}

bool FReEchoEnemyCrowdSteering::ShouldIgnoreMovementCollision(const EReEchoEnemyArchetype Self,
                                                              const EReEchoEnemyArchetype Other)
{
	return Self != EReEchoEnemyArchetype::Boss && Other != EReEchoEnemyArchetype::Boss;
}
