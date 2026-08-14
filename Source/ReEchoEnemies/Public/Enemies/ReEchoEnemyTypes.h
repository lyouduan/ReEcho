#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "CoreMinimal.h"
#include "ReEchoEnemyTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoEnemyArchetype : uint8
{
	Grunt,
	Shield,
	Bomber,
	Boss
};

UENUM(BlueprintType)
enum class EReEchoEnemyBehaviorPhase : uint8
{
	Idle,
	Pursuing,
	Attacking,
	Fuse,
	HitReaction,
	Dead
};

/** Immutable, presentation-free behavior definition compiled by the host. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyArchetype Archetype = EReEchoEnemyArchetype::Grunt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MaxHealth = 28.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MoveSpeedCmPerSecond = 95.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ContactDamage = 9.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AttackIntervalSeconds = 1.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ContactRangeCm = 85.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MovementStopDistanceCm = 75.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BomberTriggerRadiusCm = 260.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BomberDamageRadiusCm = 180.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BomberFuseDurationSeconds = 1.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HitReactionDurationSeconds = 0.22f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float KnockbackSpeedCmPerSecond = 360.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float KnockbackDrag = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bUsesDirectionalShield = false;
};

/** Explicit world sample. Enemy logic must not discover GameMode, PlayerController, or presentation state. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemySenseSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector SelfLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WorldTimeSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetExists = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetAlive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetInvulnerable = false;
};

/** Deterministic behavior output. The host applies movement and forwards attack candidates to Combat. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyActionIntent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector FacingDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector MovementDelta = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageRadiusCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector SourceLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasFacing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasMovement = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAttackCommitted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanDamageTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelfDestructAfterAttack = false;
};

/** Read-only authoritative enemy behavior state. Health and elements remain Combat-owned. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyLogicSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyArchetype Archetype = EReEchoEnemyArchetype::Grunt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyBehaviorPhase Phase = EReEchoEnemyBehaviorPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpawnIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AttackCooldownRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float FuseRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HitReactionRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector KnockbackVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector FacingDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 AttackSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bFuseActive = false;

	/** Prevents a fuse-expired bomber from publishing the same self-destruct action more than once. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelfDestructCommitted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAlive = true;
};

namespace ReEchoEnemyDefinitions
{
REECHOENEMIES_API FReEchoEnemyDefinition MakeLegacyEquivalent(EReEchoEnemyArchetype Archetype,
                                                                float BomberTriggerRadiusCm = 260.0f,
                                                                float BomberDamageRadiusCm = 180.0f,
                                                                float BomberFuseDurationSeconds = 1.2f,
                                                                float BomberDamage = 22.0f);
}
