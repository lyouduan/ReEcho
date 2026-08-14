#pragma once

#include "CoreMinimal.h"
#include "ReEchoEnemyProjectileLogic.generated.h"

/** Immutable, resource-free projectile motion definition supplied by the world host. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyProjectileDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector InitialLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SpeedCmPerSecond = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MaxRangeCm = 0.0f;
};

/** Serializable authoritative projectile motion state. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyProjectileSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DistanceTravelledCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bActive = false;
};

/** One deterministic path segment. Collision and hit resolution remain world-host responsibilities. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyProjectileAdvanceResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector PreviousLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector NewLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SegmentDistanceCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMoved = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bExpiredByRange = false;
};

/** Pure deterministic projectile policy. It performs no world queries and owns no actors or resources. */
class REECHOENEMIES_API FReEchoEnemyProjectileLogic
{
public:
	static bool Initialize(const FReEchoEnemyProjectileDefinition& Definition,
	                       FReEchoEnemyProjectileSnapshot& OutSnapshot);

	static bool RestoreSnapshot(const FReEchoEnemyProjectileDefinition& Definition,
	                            const FReEchoEnemyProjectileSnapshot& SavedSnapshot,
	                            FReEchoEnemyProjectileSnapshot& OutSnapshot);

	static FReEchoEnemyProjectileAdvanceResult Advance(
	    const FReEchoEnemyProjectileDefinition& Definition,
	    float DeltaSeconds,
	    FReEchoEnemyProjectileSnapshot& InOutSnapshot);
};
