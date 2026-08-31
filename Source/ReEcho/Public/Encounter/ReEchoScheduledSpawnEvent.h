#pragma once

#include "CoreMinimal.h"
#include "ReEchoScheduledSpawnEvent.generated.h"

UENUM()
enum class EReEchoScheduledSpawnEventType : uint8
{
	Warning,
	Commit
};

/** One deterministic role batch; also the save contract for phase-three plans and deferred quota. */
USTRUCT()

struct REECHO_API FReEchoScheduledSpawnEvent
{
	GENERATED_BODY()
	UPROPERTY()
	EReEchoScheduledSpawnEventType Type = EReEchoScheduledSpawnEventType::Warning;
	UPROPERTY()
	FName WaveId = NAME_None;
	UPROPERTY()
	FName EnemyRole = NAME_None;
	UPROPERTY()
	FName EnemyId = NAME_None;
	UPROPERTY()
	int32 Count = 0;
	UPROPERTY()
	float EventSeconds = 0.0f;
	UPROPERTY()
	float SpawnSeconds = 0.0f;
};
