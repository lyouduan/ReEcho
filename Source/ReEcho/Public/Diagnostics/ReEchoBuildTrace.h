#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "Weapons/ReEchoWeaponTypes.h"

class AActor;

/**
 * Read-only diagnostics for reconstructing build-dependent failures from a Development session log.
 * The fingerprint is diagnostic correlation only; gameplay, persistence, and deduplication must never consume it.
 */
namespace ReEchoBuildTrace
{
REECHO_API FString BuildCanonical(const FReEchoBuildSnapshot& Build);
REECHO_API FString BuildSummary(const FReEchoBuildSnapshot& Build);
REECHO_API FString ComputeFingerprint(const FReEchoBuildSnapshot& Build);

REECHO_API void LogSnapshot(const TCHAR* Event,
                            int32 EncounterIndex,
                            EReEchoRunPhase Phase,
                            const FReEchoBuildSnapshot& Build,
                            const FString& Detail = FString());

REECHO_API void LogAttackCommit(const AActor* Source,
                                const AActor* Target,
                                const FVector& Origin,
                                const FVector& Direction,
                                const FReEchoBuildSnapshot& Build,
                                const FReEchoWeaponAttackCommit& Commit);
} // namespace ReEchoBuildTrace
