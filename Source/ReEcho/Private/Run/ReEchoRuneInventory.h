#pragma once

#include "CoreMinimal.h"

struct FReEchoCsvDataSnapshot;

/** Candidate-only inventory arithmetic. Counts include equipped copies; no gameplay side effects. */
namespace ReEchoRuneInventory
{
struct FState
{
	TMap<FName, int32> Counts;
	TArray<FName> EquippedIds;
};

/** Upgrade recipes are the authority; terminal ownership also gates cached shop offers. */
bool OwnsTerminalTier(const FReEchoCsvDataSnapshot& Snapshot, FName PartId, const TArray<FName>& OwnedIds);

/** Direct acquired/equipped fusion, backpack closure, then backpack/equipped closure. */
bool TrySettle(
    const FReEchoCsvDataSnapshot& Snapshot, const FState& Input, FName AcquiredId, FState& OutState, FString& OutError);
}
