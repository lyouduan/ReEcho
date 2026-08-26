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
	Transforming,
	Dead,
	BossWindup,
	BossActive,
	BossRecovery
};

UENUM(BlueprintType)
enum class EReEchoEnemyPhaseTriggerReason : uint8
{
	None,
	AttackCountReached,
	RangeEntered,
	// WS4 (Plan 68): second phase entered because the blood bar was depleted (HP reached the configured threshold,
	// including a full depletion to zero).
	HealthDepleted
};

/** How a boss's optional second phase is triggered. Attack-count/range is the legacy model; health-threshold makes
 *  the transition fire when the current health ratio drops to (or below) HealthThresholdRatio. */
UENUM(BlueprintType)
enum class EReEchoEnemyPhase2TriggerMode : uint8
{
	// Legacy: trigger on received-attack count or aggro-target entering range (TimeGuard-style).
	AttackCountOrRange,
	// Blood-bar depleted transition: trigger when CurrentHealth/HpMax <= HealthThresholdRatio.
	HealthThreshold
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
	Recovery,
	/** Appended to preserve the serialized numeric values of the pre-Plan113 phases. */
	Active
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
	ImpactResolved,
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

	/** Number of projectiles in one volley. 1 = single shot; >1 fans them across SpreadAngleDegrees. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ProjectileCount = 1;

	/** Total fan angle in degrees across the whole volley, centered on the locked aim direction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SpreadAngleDegrees = 0.0f;

	/** When true, the enemy keeps moving toward its target during the active/recovery window of this ability. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMovementDuringCast = false;

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
	float PhaseMaxHealth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEnabled = false;
};

/** Optional one-shot transition into the enemy's second presentation/gameplay phase. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoEnemyPhaseTransitionDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Id = NAME_None;

	/** Non-positive disables the current-aggro-target range trigger. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TriggerRangeCm = 0.0f;

	/** Non-positive disables the actual health-reduction-count trigger. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RequiredAttackCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TransformSeconds = 0.0f;

	/** WS4 (Plan 68): how the second phase is triggered. HealthThreshold makes the boss transform when its health
	 *  ratio drops to/at HealthThresholdRatio (0 = full depletion). AttackCountOrRange keeps legacy behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyPhase2TriggerMode TriggerMode = EReEchoEnemyPhase2TriggerMode::AttackCountOrRange;

	/** WS4 (Plan 68): health ratio threshold for HealthThreshold mode. The transition fires when
	 *  CurrentHealth/HpMax <= this value. 0.0 means "depleted to zero". Ignored in AttackCountOrRange mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HealthThresholdRatio = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AnimationSetId = TEXT("Phase2");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEnabled = false;
};

/** Immutable, presentation-free behavior definition compiled by the host. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoEnemyDefinition
{
	GENERATED_BODY()

	/** Stable, resource-free key resolved by the host presentation catalog. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName PresentationId;

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

	/** Aggro / sensing range in cm. Enemy enters combat (wander + aggro) when the player is within this distance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HateRangeCm = 520.0f;

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

	/** Optional generic second phase. Production values are injected by the data host. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoEnemyPhaseTransitionDefinition Phase2;
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

	/** True only when Target is the currently selected target and is allowed to attract aggro. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bTargetCanAttractAggro = false;

	/** Encounter-owned global token gate. Individual enemies never copy or mutate the shared budget. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSpecialActionPermitted = true;

	/** Host-owned presentation gate. False delays only the start of a new Phase2 transition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPhase2TransitionPermitted = true;

	/** Host-owned movement gate. False suppresses movement intent without pausing behavior advancement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMovementPermitted = true;

	/** Host-owned attack gate. False delays new attack commits while target sampling and timers continue. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAttackPermitted = true;

	/** Collision-safe blink destination explicitly solved by the world host for this sample. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector TeleportDestination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasTeleportDestination = false;

	/** True when the target is inside this enemy's aggro range or the enemy has been struck; Host-owned. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInCombat = false;

	/** Host-authored aggro radius in cm. Values <= 0 fall back to pursuit-only (legacy) behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HateRangeCm = 0.0f;

	/** WS4 (Plan 68): current health ratio (CurrentHealth/HpMax) sampled by the host, clamped to [0,1]. Default 1.0
	 *  (full/unknown) keeps legacy enemies inert for the blood-depleted phase trigger. Enemy logic never reads the
	 *  owner's combat component directly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CurrentHealthRatio = 1.0f;
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

	/** True only for one authoritative Elite dash movement step. The Host performs the world sweep and collision. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSpecialDashMovement = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelfDestructAfterAttack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPhaseTransitionStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPhaseTransitionCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyPhaseTriggerReason PhaseTriggerReason = EReEchoEnemyPhaseTriggerReason::None;

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
	int32 CurrentPhaseIndex = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ReceivedDamageCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PhaseTransitionRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyPhaseTriggerReason PhaseTriggerReason = EReEchoEnemyPhaseTriggerReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPhase2Triggered = false;

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

	/** Remaining authored dash distance. Decremented by emitted movement, never by presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SpecialDashRemainingDistanceCm = 0.0f;

	/** One identity survives every Active step and save/restore boundary of the current dash. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity SpecialAttack;

	/** Consumed on the first authoritative path contact, even when Combat resolves zero applied damage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSpecialDamageConsumed = false;

	/** Index of the next enabled non-Boss special ability in deterministic SequenceOrder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpecialNextSequenceIndex = 0;

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

	/** Idle-wander state. Direction is re-derived deterministically every WanderPeriodSeconds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float IdleWanderElapsedSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector IdleWanderDirection = FVector::ForwardVector;

	/** True once the enemy has ever entered combat this encounter (prevents re-wander after aggro). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasEngaged = false;

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
