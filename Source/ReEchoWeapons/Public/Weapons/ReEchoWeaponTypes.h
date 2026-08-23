#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "CoreMinimal.h"
#include "ReEchoWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoWeaponAttackCarrier : uint8
{
	Melee,
	Projectile,
	Wave
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoWeaponStepDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName StepId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 StepIndex = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DurationSeconds = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PhysicalCoefficient = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ElementalCoefficient = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RangeCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ArcDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ProjectileCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpreadDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ExplosionRadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MovementCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bInvulnerable = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoWeaponAttackCarrier Carrier = EReEchoWeaponAttackCarrier::Melee;
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoWeaponDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName WeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackPatternId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName DamageChannelId = TEXT("Physical");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AttackIntervalSeconds = 0.55f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PhysicalCoefficient = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ElementalCoefficient = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RangeCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ArcDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ProjectileCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpreadDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ExplosionRadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float OnKillHealPercent = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bUsesCyclingElement = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bUsesDeterministicRandomElement = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FReEchoWeaponStepDefinition> AttackSteps;
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoWeaponAttackCommit
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName WeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackPatternId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackStepId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 StepIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoWeaponAttackCarrier Carrier = EReEchoWeaponAttackCarrier::Melee;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement Element = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RawDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RangeCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ArcDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ProjectileCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpreadDegrees = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ExplosionRadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MovementCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BehaviorDurationSeconds = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bInvulnerable = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoWeaponSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName WeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackPatternId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName NextAttackStepId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ReadinessRemainingSeconds = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BehaviorRemainingSeconds = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bInvulnerable = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SuccessfulAttackCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement NextElement = EReEchoElement::None;
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoProjectileId
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid Value;

	bool IsValid() const
	{
		return Value.IsValid();
	}
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoLogicalProjectileSpec
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoHitIntent HitIntent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Direction = FVector::ForwardVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpeedCmPerSecond = 950.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CarrierRadiusCm = 13.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ExplosionRadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumRangeCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bPierceOnCritical = false;
};

USTRUCT(BlueprintType)

struct REECHOWEAPONS_API FReEchoProjectileSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoProjectileId ProjectileId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Position = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Velocity = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TravelledCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumRangeCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bActive = false;
};
