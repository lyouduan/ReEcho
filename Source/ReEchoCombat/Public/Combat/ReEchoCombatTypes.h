#pragma once

#include "CoreMinimal.h"
#include "ReEchoCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoCombatFaction : uint8
{
	Unaligned,
	PlayerSide,
	EnemySide
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoAttackIdentity
{
	GENERATED_BODY()

	/**
	 * The actor that owns the attack. The weak handle prevents delayed carriers from dereferencing a source that has
	 * already left the world; identity comparisons still include its object index/serial together with Sequence.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 Sequence = 0;

	/** Snapshotted when the attack commits so delayed carriers keep their relation after the source leaves the world.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoCombatFaction SourceFaction = EReEchoCombatFaction::Unaligned;

	/** Stable gameplay weapon identity snapshotted at commit; presentation may resolve its VisualKey without guessing.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName WeaponId = NAME_None;

	bool IsValid() const
	{
		return !Source.IsExplicitlyNull() && Sequence != 0;
	}

	bool HasLiveSource() const
	{
		return Source.IsValid();
	}

	bool operator==(const FReEchoAttackIdentity& Other) const
	{
		return Source.HasSameIndexAndSerialNumber(Other.Source) && Sequence == Other.Sequence;
	}

	bool operator!=(const FReEchoAttackIdentity& Other) const
	{
		return !(*this == Other);
	}
};

UENUM(BlueprintType)
enum class EReEchoElement : uint8
{
	None,
	Flame,
	Lightning,
	Grass,
	Water
};

UENUM(BlueprintType)
enum class EReEchoDamageSource : uint8
{
	Player,
	Echo,
	Path,
	Reaction,
	Enemy
};

/** A typed request for reconciling current health after an authoritative maximum-health change. */
UENUM(BlueprintType)
enum class EReEchoHealthAdjustment : uint8
{
	None,
	FillToMax,
	/** Keep the authoritative maximum and set current health to the committed build's HpPoint. */
	SetToStatPoint
};

/** A weapon-generated candidate hit. It contains no presentation resource and no final result. */
USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoHitIntent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RawDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement Element = EReEchoElement::None;
	/** Resource-free reaction provenance used by presentation; NAME_None for ordinary hits. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ReactionBehaviorId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ReactionEfficiency = 1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
	/** Multiplier applied to base damage for a critical hit (1.0 + CriticalEffect). Drives crit damage-number scaling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CriticalMultiplier = 1.0f;
	/** Internal adjudication guard: source-side rule providers already transformed this intent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bSourceRulesApplied = false;
	/** Explicit exception for authored self-damage such as enemy self-destruction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAllowSameFactionDamage = false;
	/** Generic statuses requested by source-side rules and applied by Combat immediately before final damage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FName> PreDamageStatusIds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector SourceLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector HitLocation = FVector::ZeroVector;
};

/** The immutable result produced by the Combat resolver. */
USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoHitResolved
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RawDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AppliedDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement Element = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ReactionBehaviorId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
	/** Multiplier applied to base damage for a critical hit (1.0 + CriticalEffect). Drives crit damage-number scaling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CriticalMultiplier = 1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bBlocked = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bKilled = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector HitLocation = FVector::ZeroVector;
};

UENUM(BlueprintType)
enum class EReEchoAttackMode : uint8
{
	Automatic,
	Manual
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HpPoint = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HpMax = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PhysicalAttack = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ElementalAttack = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Block = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MovementSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CriticalRate = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CriticalEffect = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EchoEfficiency = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReactionEfficiency = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RoleId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ProjectileCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeaponSize = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EchoCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ShopDiscount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CharacterSize = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Concentration = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PathAffinity = 1.f;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EReEchoElement Attached = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ImmunityUntil = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnhancedNextReaction = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float EnhancementMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EReEchoElement BlockedAttachment = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, float> ActiveStatusUntilSeconds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBurnActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BurnTickDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BurnNextTickTimeSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BurnReactionBehaviorId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector BurnSourceLocation = FVector::ZeroVector;
	UPROPERTY()
	FReEchoAttackIdentity BurnAttack;
};

/** Deterministic Combat command; the caller supplies its authoritative simulation time. */
USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementCleanseCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentTimeSeconds = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ImmunityDurationSeconds = 0.0f;
};

/** A valid command succeeds even when the requested state is already present. */
USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementCleanseResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bSucceeded = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStateChanged = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bClearedAttachment = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bClearedBurn = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ImmunityUntil = 0.0f;
};

/** Resource-free command for rune/card status application. Combat owns all timers and damage. */
USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoTimedStatusCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName StatusId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentTimeSeconds = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DamagePerTickMaxHealthFraction = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FReEchoAttackIdentity Attack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
};
