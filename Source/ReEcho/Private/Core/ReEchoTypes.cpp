#include "Core/ReEchoTypes.h"

FVector FReEchoRecording::EvaluatePosition(const float Time) const
{
	if (Positions.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	if (Time <= Positions[0].Time)
	{
		return Positions[0].Position;
	}
	if (Time >= Positions.Last().Time)
	{
		return Positions.Last().Position;
	}

	const int32 Upper = Algo::LowerBoundBy(Positions, Time, &FReEchoPositionSample::Time);
	const FReEchoPositionSample& B = Positions[Upper];
	const FReEchoPositionSample& A = Positions[Upper - 1];
	const float Alpha = FMath::IsNearlyEqual(A.Time, B.Time) ? 0.f : (Time - A.Time) / (B.Time - A.Time);
	return FMath::Lerp(A.Position, B.Position, Alpha);
}

