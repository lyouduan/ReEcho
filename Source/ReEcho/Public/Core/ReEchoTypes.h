#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTypes.h"
#include "Cards/ReEchoCardTypes.h"
#include "Enemies/ReEchoEnemyProjectileLogic.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoTypes.generated.h"

class AActor;
class UTexture2D;

UENUM(BlueprintType)
enum class EReEchoRunPhase : uint8
{
	CharacterSelect,
	Planning,
	Encounter,
	CardChoice,
	/** Serialized compatibility only. Runtime restores this legacy value as CardChoice. */
	LegacyForgeChoice,
	Shop,
	Summary,
	Failed
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

	FName CharacterId = "J_SPADE";
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
	FReEchoCardBuildState CardState;
	/** v8 migration input only. v9 writes CardState.OwnedCardIds and leaves this empty. */
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

	/** Authored build tags from ReEchoData.xlsx / 构筑体系G / Tags. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> Tags;

	/** Gameplay/card-group tier. Easter candidates keep the containing group's tier. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Tier = 0;

	/** Visual-only card-frame tier. Easter candidates explicitly reuse tier three. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PresentationTier = 0;

	/** Visible slot identity and refresh projection for post-encounter/free card choices. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RemainingRefreshes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RefreshCost = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanRefresh = false;

	/** Optional card presentation textures resolved at runtime by the choice widget (Plan 69).
	    Empty when source art is absent; never a hard dependency, so missing art degrades gracefully. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> CardArt = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> CardIcon = nullptr;
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

/** Serializable Host state for one in-flight or committed-but-delayed enemy projectile. */
USTRUCT()

struct REECHO_API FReEchoEnemyProjectileRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	FReEchoEnemyProjectileDefinition Definition;

	UPROPERTY()
	FReEchoEnemyProjectileSnapshot Snapshot;

	UPROPERTY()
	FReEchoAttackIdentity Attack;

	UPROPERTY()
	float Damage = 0.0f;

	UPROPERTY()
	float CollisionRadiusCm = 20.0f;

	/** Stable zero-based ball index inside one enemy volley; INDEX_NONE keeps legacy single-projectile semantics. */
	UPROPERTY()
	int32 VolleyBallIndex = INDEX_NONE;

	/** Remaining delay before this committed ball enters the world and publishes Spawned. */
	UPROPERTY()
	float SpawnDelayRemainingSeconds = 0.0f;

	/** Defaults true so saves created before delayed volleys restore existing in-flight projectiles unchanged. */
	UPROPERTY()
	bool bSpawnEventPublished = true;

	/** Guards the authoritative collision sample until a hit removes the projectile. */
	UPROPERTY()
	bool bCollisionConsumed = false;
};

/** Serializable runtime state for one living enemy in a suspended encounter. */
USTRUCT()

struct REECHO_API FReEchoEnemyRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Kind = 0;

	/** v9+: stable data identity; Kind remains legacy migration input. */
	UPROPERTY()
	FName EnemyId = NAME_None;

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

	/** Added with the EnemyLogic migration; old saves default to zero and continue monotonically. */
	UPROPERTY()
	int64 AttackSequence = 0;

	/** Prevents a restored fuse-expired bomber from submitting the same self-destruct twice. */
	UPROPERTY()
	bool bSelfDestructCommitted = false;

	/** v8+ canonical logic state, including deterministic special/Boss ability timing. */
	UPROPERTY()
	FReEchoEnemyLogicSnapshot LogicSnapshot;

	/** Older saves reconstruct LogicSnapshot from the compatibility fields above. */
	UPROPERTY()
	bool bHasLogicSnapshot = false;

	UPROPERTY()
	TArray<FReEchoEnemyProjectileRuntimeState> BossProjectiles;
};

/** A warned spawn batch whose exact locations are reserved before its commit time. */
USTRUCT()

struct REECHO_API FReEchoPendingSpawnBatchState
{
	GENERATED_BODY()

	UPROPERTY()
	FName WaveId = NAME_None;

	UPROPERTY()
	FName EnemyRole = NAME_None;

	UPROPERTY()
	FName EnemyId = NAME_None;

	UPROPERTY()
	TArray<FVector> Locations;
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

	/** v9+: next warning/commit gate; prevents duplicate wave generation after continue. */
	UPROPERTY()
	int32 NextScheduledSpawnEventIndex = 0;

	/** v9+: remaining durations of Encounter-owned ranged burst leases. */
	UPROPERTY()
	TArray<float> RangedBurstWindowRemainingSeconds;

	/** v9+: warning-resolved positions that must commit unchanged after continue. */
	UPROPERTY()
	TArray<FReEchoPendingSpawnBatchState> PendingSpawnBatches;

	UPROPERTY()
	int32 SpawnResolveSequence = 0;

	UPROPERTY()
	TArray<FVector> ReservedSpawnLocations;

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

	/** Boss room 30-second transition already removed echoes and applied the one-shot player boost. */
	UPROPERTY()
	bool bBossPostEchoPhaseTriggered = false;

	/** Hidden Boss Phase3 already doubled player stats and future non-Boss wave capacity. */
	UPROPERTY()
	bool bBossPhase3EscalationTriggered = false;

	UPROPERTY()
	TArray<FReEchoEnemyRuntimeState> Enemies;
};

/** Stable identifiers and bounds for G_3_02's single time anchor. */
namespace ReEchoTimeAnchor
{
constexpr const TCHAR* CardId = TEXT("G_3_02");
/** Upper bound accepted by the replay resolver; actual count is further limited by card rules. */
constexpr int32 MaximumResolvedEchoes = 3;
} // namespace ReEchoTimeAnchor

/** Explicit outcome of every echo storage command; commands never partially mutate on failure. */
UENUM(BlueprintType)
enum class EReEchoEchoStorageResult : uint8
{
	Success,
	/** No successfully completed encounter is currently awaiting a store-or-skip decision. */
	NoPendingRecording,
	/** A referenced recording id is zero, or is not present in stored echoes. */
	InvalidRecordingId,
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
};

/** Read-only snapshot of the whole run-local echo storage state. */
USTRUCT(BlueprintType)

struct REECHO_API FReEchoEchoStorageSummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasPendingRecording = false;

	/** Only valid when bHasPendingRecording is true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoStoredEchoSummary PendingRecording;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasLatestCompletedRecording = false;

	/** Only valid when bHasLatestCompletedRecording is true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoStoredEchoSummary LatestCompletedRecording;

	/** True only when G_3_02 currently owns one persistent time-anchor recording. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasTimeAnchor = false;

	/** The single G_3_02 time anchor. Only valid when bHasTimeAnchor is true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoStoredEchoSummary TimeAnchorRecording;
};
