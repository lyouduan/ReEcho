#include "Combat/ReEchoCombatContracts.h"

bool FReEchoWeaponCadence::TryCommit(const float IntervalSeconds, FReEchoAttackCommitId& OutCommitId)
{
	if (!IsReady())
	{
		return false;
	}
	RemainingSeconds = FMath::Max(0.01f, IntervalSeconds);
	OutCommitId.Value = ++LastCommitId;
	return true;
}

void FReEchoWeaponCadence::Tick(const float DeltaSeconds)
{
	RemainingSeconds = FMath::Max(0.0f, RemainingSeconds - FMath::Max(0.0f, DeltaSeconds));
}

void FReEchoWeaponCadence::Reset()
{
	RemainingSeconds = 0.0f;
	LastCommitId = 0;
}
