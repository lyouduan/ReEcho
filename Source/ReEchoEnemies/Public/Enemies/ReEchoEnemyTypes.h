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
	Slime,
	Ranged,
	Elite,
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
	Dead,
	BossWindup,
	BossActive,
	BossRecovery
};

UENUM(BlueprintType)
enum class EReEchoBossAbilityKind : uint8
{
	None,
	MeleeSweep,
	Projectile,
	BlinkSlam,
	PrayerBeam,
	ElementCleanse
};

UENUM(BlueprintType)
enum class EReEchoBossActionPhase : uint8
{
	None,
	Windup,
	Active,
	Recovery
};

UENUM(BlueprintType)
enum class EReEchoEnemySpecialActionPhase : uint8
{
	None,
	Windup,
	Recovery
};

UENUM(BlueprintType)
enum class EReEchoBossLockTiming : uint8
{
	WindupStarted,
	WindupEnded,
	Interval
};

UENUM(BlueprintType)
enum class EReEchoBossTargetingMode : uint8
{
	LockedLocation,
	LockedDirection,
	Self
};

UENUM(BlueprintType)
enum class EReEchoBossAttackShape : uint8
{
	None,
	Rectangle,
	Projectile,
	Circle,
	Beam
};

UENUM(BlueprintType)
enum class EReEchoBossIntentType : uint8
{
	TelegraphStarted,
	AttackWindowStarted,
	AbilityEnded,
	ElementCleanse,
	EncounterPhase
};

UENUM(BlueprintType)
enum class EReEchoBossEchoPolicy : uint8
{
	None,
	RetireEncounterEchoes
};

UENUM(BlueprintType)
enum class EReEchoBossRefillHealthPolicy : uint8
{
	None,
	RefillToMaximum
};

/** Immutable active/passive Boss ability definition compiled from the authoritative data source. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoEnemyAbilityDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Id = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName BehaviorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SequenceOrder = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Damage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WindupSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ActiveSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RecoverySeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CooldownSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MinRangeCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MaxRangeCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RadiusCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WidthCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LengthCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ProjectileSpeedCmPerSecond = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TeleportOffsetCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CleanseIntervalSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ImmunitySeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossTargetingMode TargetingMode = EReEchoBossTargetingMode::LockedLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossLockTiming LockTiming = EReEchoBossLockTiming::WindupStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEnabled = false;
};

/** Immutable encounter phase definition. The encounter host owns applying the returned policy and multipliers. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoBossPhaseDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Id = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PhaseIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TriggerSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossEchoPolicy EchoPolicy = EReEchoBossEchoPolicy::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PhysicalAttackMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ElementalAttackMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AttackSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MovementSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossRefillHealthPolicy RefillHealthPolicy = EReEchoBossRefillHealthPolicy::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEnabled = false;
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
	float CollisionRadiusCm = 51.84f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CollisionHalfHeightCm = 183.6f;

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

	/** Boss-only immutable definitions. Non-Boss archetypes leave both arrays empty. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	/** Data-authored abilities for any enemy archetype; Bosses simply own more entries. */
	TArray<FReEchoEnemyAbilityDefinition> Abilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoBossPhaseDefinition> BossPhases;
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

	/** Encounter-owned global token gate. Individual enemies never copy or mutate the shared budget. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSpecialActionPermitted = true;

	/** Collision-safe blink destination explicitly solved by the world host for this sample. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector TeleportDestination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasTeleportDestination = false;
};

/** One ordered Boss command. Multiple commands may be emitted by one large deterministic advance. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoBossIntent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossIntentType Type = EReEchoBossIntentType::TelegraphStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossAbilityKind AbilityKind = EReEchoBossAbilityKind::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossAttackShape AttackShape = EReEchoBossAttackShape::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName BehaviorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector LockedTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector LockedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector TeleportDestination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RadiusCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WidthCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LengthCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ProjectileSpeedCmPerSecond = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WindupSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ActiveSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RecoverySeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossTargetingMode TargetingMode = EReEchoBossTargetingMode::LockedLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossLockTiming LockTiming = EReEchoBossLockTiming::WindupStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ElementImmunitySeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoBossPhaseDefinition PhaseDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanDamageTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bRequestTeleport = false;
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

	/** Ordered semantic commands for Boss-only world execution. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoBossIntent> BossIntents;
};

USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoBossAbilityCooldownSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RemainingSeconds = 0.0f;
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
	EReEchoEnemySpecialActionPhase SpecialActionPhase = EReEchoEnemySpecialActionPhase::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName SpecialAbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SpecialActionRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector SpecialLockedTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector SpecialLockedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoBossActionPhase BossActionPhase = EReEchoBossActionPhase::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName BossCurrentAbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BossNextSequenceIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BossNextPhaseIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 BossCurrentAttackSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BossActionPhaseRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BossCleanseRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BossEncounterElapsedSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BossSimulationAccumulatorSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector BossLockedTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector BossLockedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector BossLockedTeleportDestination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoBossAbilityCooldownSnapshot> BossAbilityCooldowns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bBossHasLockedTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bBossHasLockedTeleportDestination = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bBossCurrentAbilityCommitted = false;

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
