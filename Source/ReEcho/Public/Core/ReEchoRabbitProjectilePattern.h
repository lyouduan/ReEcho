#pragma once

#include "CoreMinimal.h"

/** Gameplay-authoritative shape of the delivered three-ball rabbit volley. */
namespace ReEchoRabbitProjectilePattern
{
inline constexpr int32 BallCount = 3;
inline constexpr int32 CenterBallIndex = 1;
inline constexpr float HalfSpreadDegrees = 32.5f;

inline FVector ResolveDirection(const FVector& CenterDirection, const int32 BallIndex)
{
	const FVector SafeCenter =
	    CenterDirection.IsNearlyZero() ? FVector::ForwardVector : CenterDirection.GetSafeNormal2D();
	const float YawOffset = static_cast<float>(BallIndex - CenterBallIndex) * HalfSpreadDegrees;
	return FRotator(0.0f, YawOffset, 0.0f).RotateVector(SafeCenter).GetSafeNormal2D();
}

/** Data-driven volley direction: fans `BallCount` projectiles across `TotalSpreadDegrees`, centered on CenterDirection. */
inline FVector ResolveVolleyDirection(const FVector& CenterDirection,
                                      const float TotalSpreadDegrees,
                                      const int32 BallIndex,
                                      const int32 InBallCount)
{
	const FVector SafeCenter =
	    CenterDirection.IsNearlyZero() ? FVector::ForwardVector : CenterDirection.GetSafeNormal2D();
	if (InBallCount <= 1)
	{
		return SafeCenter;
	}
	const float StepDegrees = TotalSpreadDegrees / static_cast<float>(InBallCount - 1);
	const float CenterIndex = static_cast<float>(InBallCount - 1) * 0.5f;
	const float YawOffset = (static_cast<float>(BallIndex) - CenterIndex) * StepDegrees;
	return FRotator(0.0f, YawOffset, 0.0f).RotateVector(SafeCenter).GetSafeNormal2D();
}

inline float ResolveBallCollisionRadius(const float VolleyRadiusCm)
{
	return FMath::Max(10.0f, VolleyRadiusCm / static_cast<float>(BallCount));
}

inline float ResolveBallCollisionRadius(const float VolleyRadiusCm, const int32 Count)
{
	return FMath::Max(10.0f, VolleyRadiusCm / static_cast<float>(FMath::Max(1, Count)));
}
} // namespace ReEchoRabbitProjectilePattern
