#pragma once

#include "CoreMinimal.h"
#include "ReEchoTypes.generated.h"

class AActor;

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

struct REECHO_API FReEchoEquippedPartSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SlotTypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PartId = NAME_None;
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
	int32 WeaponDataRevision = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WeaponDomainRevision;
	/** Non-equipment stats used to deterministically rebuild derived part effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FReEchoStatBlock EquipmentBaseStats;
	/** Non-equipment rules used to deterministically rebuild derived part effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FString> EquipmentBaseRuleFlags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasEquipmentBase = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FReEchoEquippedPartSnapshot> EquippedParts;
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
	bool bBurnActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BurnTickDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BurnNextTickTimeSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BurnSourceLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> BurnSourceActor;
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

/** Prototype-locked bounds for the run-local echo storage and specific-replay capabilities. */
namespace ReEchoEchoStorage
{
/** Slot count a fresh run starts with; the prototype does not grow or shrink it yet. */
constexpr int32 DefaultStorageCapacity = 3;
/** Hard prototype ceiling for stored echo slots. */
constexpr int32 MaxStorageCapacity = 3;
/** Sentinel meaning the player has not unlocked specific replay at all. */
constexpr int32 SpecificReplayUnavailable = 0;
/** Hard prototype ceiling for how many stored echoes one encounter may replay. */
constexpr int32 MaxSpecificReplayLimit = 3;
} // namespace ReEchoEchoStorage

/** Explicit outcome of every echo storage command; commands never partially mutate on failure. */
UENUM(BlueprintType)
enum class EReEchoEchoStorageResult : uint8
{
	Success,
	/** No successfully completed encounter is currently awaiting a store-or-skip decision. */
	NoPendingRecording,
	/** A referenced recording id is zero, or is not present in stored echoes. */
	InvalidRecordingId,
	/** The recording id already exists in stored echoes, or the request repeats an id. */
	DuplicateRecordingId,
	/** Every storage slot is occupied and no explicit replacement target was supplied. */
	StorageFull,
	/** The replacement target is not currently stored, so the request is stale. */
	InvalidReplacementTarget,
	/** Requested capacity is out of prototype range, or would drop already stored echoes. */
	InvalidStorageCapacity,
	/** Requested specific replay limit is out of prototype range. */
	InvalidReplayLimit,
	/** More replay selections were requested than the current specific replay limit allows. */
	ReplayLimitExceeded
};

/** Read-only description of one stored echo; UI reads this instead of the immutable recording payload. */
USTRUCT(BlueprintType)

struct REECHO_API FReEchoStoredEchoSummary
{
	GENERATED_BODY()

	/** Stable persistent identity of the stored echo; array order is never an identity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid RecordingId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Duration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName MapId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName CharacterId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName WeaponId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelectedForNextEncounter = false;
};

/** Read-only snapshot of the whole run-local echo storage state. */
USTRUCT(BlueprintType)

struct REECHO_API FReEchoEchoStorageSummary
{
	GENERATED_BODY()

	/** Explicitly stored echoes, in slot order; never contains the rolling latest recording implicitly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FReEchoStoredEchoSummary> StoredEchoes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StorageCapacity = ReEchoEchoStorage::DefaultStorageCapacity;

	/** Zero means specific replay is not unlocked; the next encounter defaults to the rolling latest echo. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpecificReplayLimit = ReEchoEchoStorage::SpecificReplayUnavailable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasPendingRecording = false;

	/** Only valid when bHasPendingRecording is true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoStoredEchoSummary PendingRecording;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasLatestCompletedRecording = false;

	/** Only valid when bHasLatestCompletedRecording is true; independent from StoredEchoes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoStoredEchoSummary LatestCompletedRecording;

	/** Stable ids chosen for the next encounter; always a subset of StoredEchoes with no duplicates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FGuid> SelectedReplayIds;
};
