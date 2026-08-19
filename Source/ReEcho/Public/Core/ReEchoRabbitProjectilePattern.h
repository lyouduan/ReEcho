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

inline float ResolveBallCollisionRadius(const float VolleyRadiusCm)
{
	return FMath::Max(10.0f, VolleyRadiusCm / static_cast<float>(BallCount));
}
} // namespace ReEchoRabbitProjectilePattern
