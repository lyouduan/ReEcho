#pragma once

#include "CoreMinimal.h"
#include "ReEchoTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoRunPhase : uint8
{
	CharacterSelect,
	Planning,
	Encounter,
	CardChoice,
	ForgeChoice,
	Shop,
	Summary,
	Failed
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

USTRUCT(BlueprintType)

struct REECHO_API FReEchoStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	float HpPoint = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HpMax = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PhysicalAttack = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ElementalAttack = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Block = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalRate = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalEffect = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EchoEfficiency = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReactionEfficiency = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RoleId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRandomElementProjectiles = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EverySecondAttackBonus = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ProjectileCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponSize = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EchoCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShopDiscount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CharacterSize = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Concentration = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PathAffinity = 1.f;
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoBuildSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	FName CharacterId = "J01";
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponId = "W_J_01";
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> Cards;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FReEchoStatBlock Stats;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FString> RuleFlags;
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoTraitCardOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName CardId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FText Description;
};
USTRUCT(BlueprintType)

struct REECHO_API FReEchoPositionSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	float Time = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoSkillEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	float Time = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SkillId = "ActiveAOE";
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoWeaponEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponId;
};
USTRUCT(BlueprintType)

struct REECHO_API FReEchoRecording
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)

	FGuid Id;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterIndex = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Duration = 30.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName MapId = "Arena01";
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoPositionSample> Positions;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoSkillEvent> Skills;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoWeaponEvent> WeaponChanges;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoBuildSnapshot BuildSnapshot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RandomSeed = 0;

	FVector EvaluatePosition(float Time) const;
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoElementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	EReEchoElement Attached = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ImmunityUntil = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnhancedNextReaction = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EnhancementMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoElement BlockedAttachment = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, float> ActiveStatusUntilSeconds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BurnTickDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BurnNextTickTimeSeconds = 0.0f;
};

/** Serializable runtime state for one living enemy in a suspended encounter. */
USTRUCT()

struct REECHO_API FReEchoEnemyRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Kind = 0;

	UPROPERTY()
	int32 SpawnIndex = 0;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	float CurrentHealth = 0.0f;

	/** Save boundary stores time-bearing fields as remaining durations; restore rebases them to the new world clock. */
	UPROPERTY()
	FReEchoElementState ElementState;

	UPROPERTY()
	float AttackCooldown = 0.0f;

	UPROPERTY()
	float FuseRemaining = 0.0f;

	UPROPERTY()
	bool bBomberFuseActive = false;

	UPROPERTY()
	float HitReactionRemaining = 0.0f;

	UPROPERTY()
	FVector KnockbackVelocity = FVector::ZeroVector;

	UPROPERTY()
	FVector ShakeDirection = FVector::ZeroVector;
};

/** Exact resumable state captured only when the player confirms an in-encounter quit. */
USTRUCT()

struct REECHO_API FReEchoEncounterRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bValid = false;

	UPROPERTY()
	float EncounterTime = 0.0f;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	float PlayerHealth = 0.0f;

	UPROPERTY()
	FReEchoStatBlock PlayerStats;

	UPROPERTY()
	FVector PlayerVelocity = FVector::ZeroVector;

	UPROPERTY()
	FReEchoRecording ActiveRecording;

	UPROPERTY()
	TArray<FReEchoEnemyRuntimeState> Enemies;
};
