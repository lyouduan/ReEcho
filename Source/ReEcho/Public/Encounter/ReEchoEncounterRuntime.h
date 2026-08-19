#pragma once

#include "CoreMinimal.h"
#include "Data/ReEchoCsvDataRegistry.h"

enum class EReEchoScheduledSpawnEventType : uint8
{
	Warning,
	Commit
};

/** Pure lifecycle decision between the completed encounter and the next configured encounter. */
struct REECHO_API FReEchoStageTransitionDecision
{
	FName PreviousStageId = NAME_None;
	FName NextStageId = NAME_None;
	bool bHasPreviousEncounter = false;
	bool bSameStage = false;
	bool bPreserveEnemyRoster = false;
	bool bPreservePlayerLocation = false;
};

namespace ReEchoStageTransition
{
/** Resolves the single transition policy consumed by intermission and next-encounter startup. */
REECHO_API bool Resolve(const FReEchoCsvDataSnapshot& Snapshot,
                        int32 CompletedEncounterIndex,
                        FReEchoStageTransitionDecision& OutDecision,
                        FString& OutError);
}

/** One typed, deterministic role batch emitted by the encounter clock. */
struct REECHO_API FReEchoScheduledSpawnEvent
{
	EReEchoScheduledSpawnEventType Type = EReEchoScheduledSpawnEventType::Warning;
	FName WaveId = NAME_None;
	FName EnemyRole = NAME_None;
	FName EnemyId = NAME_None;
	int32 Count = 0;
	float EventSeconds = 0.0f;
	float SpawnSeconds = 0.0f;
};

/** Pure fixed-clock wave gate. It never reads World time or spawns actors. */
class REECHO_API FReEchoEncounterWaveScheduler
{
public:
	bool Configure(const FReEchoCsvDataSnapshot& Snapshot, FName EncounterId, FString& OutError);
	TArray<FReEchoScheduledSpawnEvent> AdvanceTo(float EncounterSeconds);
	void RestoreNextEventIndex(int32 InNextEventIndex);
	void Reset();

	int32 GetNextEventIndex() const
	{
		return NextEventIndex;
	}

	int32 GetEventCount() const
	{
		return Events.Num();
	}

private:
	TArray<FReEchoScheduledSpawnEvent> Events;
	int32 NextEventIndex = 0;
};

struct REECHO_API FReEchoSpawnResolveRequest
{
	FVector PlayerAnchor = FVector::ZeroVector;
	FVector EchoAnchor = FVector::ZeroVector;
	TArray<FVector> ExistingLocations;
	float EchoAnchorRatio = 0.0f;
	float ArenaHalfX = 0.0f;
	float ArenaHalfY = 0.0f;
	int32 Seed = 0;
	int32 Sequence = 0;
	bool bHasEchoAnchor = false;
};

struct REECHO_API FReEchoResolvedSpawn
{
	FVector Location = FVector::ZeroVector;
	bool bUsedEchoAnchor = false;
	bool bUsedDeterministicFallback = false;
};

/** Pure deterministic candidate solver shared by every encounter spawn batch. */
class REECHO_API FReEchoSpawnResolver
{
public:
	static bool Resolve(const FReEchoCsvSpawnProfileRow& Profile,
	                    const FReEchoCsvSpawnPolicyRow& Policy,
	                    const FReEchoSpawnResolveRequest& Request,
	                    FReEchoResolvedSpawn& OutSpawn,
	                    FString& OutError);
};
