#pragma once

#include "CoreMinimal.h"
#include "Enemies/ReEchoEnemyTypes.h"

struct FReEchoEnemyCrowdNeighbor
{
	FVector Location = FVector::ZeroVector;
	float RadiusCm = 0.0f;
	int32 SpawnIndex = 0;
	bool bBoss = false;
};

struct FReEchoEnemyCrowdSteeringInput
{
	FVector SelfLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector DesiredMovementDelta = FVector::ZeroVector;
	TArray<FReEchoEnemyCrowdNeighbor> Neighbors;
	float SelfRadiusCm = 0.0f;
	float PreferredTargetDistanceCm = 0.0f;
	float BlockedSeconds = 0.0f;
	int32 SpawnIndex = 0;
};

/** Pure-value deterministic steering applied by EnemyHost after EnemyLogic produces movement intent. */
struct REECHO_API FReEchoEnemyCrowdSteering
{
	static FVector ResolveMovement(const FReEchoEnemyCrowdSteeringInput& Input);
	static bool ShouldIgnoreMovementCollision(EReEchoEnemyArchetype Self, EReEchoEnemyArchetype Other);
};
