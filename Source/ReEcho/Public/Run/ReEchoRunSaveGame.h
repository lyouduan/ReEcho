#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/SaveGame.h"
#include "ReEchoRunSaveGame.generated.h"

/** Persistent safe-checkpoint data for one resumable run. */
UCLASS()

class REECHO_API UReEchoRunSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** v18 persists refreshed post-encounter card choices and their independent slot budgets. */
	static constexpr int32 CurrentSaveVersion = 18;

	/** Oldest layout this build can still migrate forward. */
	static constexpr int32 MinimumSupportedSaveVersion = 4;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(SaveGame)
	EReEchoRunPhase SavedPhase = EReEchoRunPhase::Planning;

	UPROPERTY(SaveGame)
	int32 EncounterIndex = 0;

	UPROPERTY(SaveGame)
	int32 TimeShards = 0;

	/** Added in v12. Makes each run's card offers random while keeping save/load reproducible. */
	UPROPERTY(SaveGame)
	int32 TraitOfferSeed = 0;

	/** Added in v18. Stable post-encounter/free three-choice page and per-slot refresh state. */
	UPROPERTY(SaveGame)
	int32 PendingTraitCardOfferEncounterIndex = INDEX_NONE;

	UPROPERTY(SaveGame)
	TArray<FName> PendingTraitCardIds;

	UPROPERTY(SaveGame)
	TArray<int32> PendingTraitCardRefreshUses;

	/** Added in v13. Independent seed for per-enemy time-shard ranges. */
	UPROPERTY(SaveGame)
	int32 EnemyShardDropSeed = 0;

	/** Added in v13. Packed EncounterIndex/SpawnIndex deaths already processed for base rewards. */
	UPROPERTY(SaveGame)
	TArray<int64> RewardedEnemyShardDropKeys;

	UPROPERTY(SaveGame)
	FReEchoBuildSnapshot CurrentBuild;

	UPROPERTY(SaveGame)
	TArray<FName> InventoryItems;

	/** Added in v10. Older saves derive ownership from their valid equipped parts. */
	UPROPERTY(SaveGame)
	TArray<FName> OwnedPartIds;

	/** Added in v13. Older saves derive the minimum weapon backpack from CurrentBuild.WeaponId. */
	UPROPERTY(SaveGame)
	TArray<FName> OwnedWeaponIds;

	/** Added in v14. Stable weapon/rune offers for one encounter + refresh-sequence page. */
	UPROPERTY(SaveGame)
	int32 WeaponPartShopOfferEncounterIndex = INDEX_NONE;

	UPROPERTY(SaveGame)
	int32 WeaponPartShopOfferRefreshSequence = INDEX_NONE;

	/** Fixed three content ids; NAME_None represents an empty slot. */
	UPROPERTY(SaveGame)
	TArray<FName> WeaponPartShopOfferIds;

	/** Added in v17. Desired deterministic weapon/rune page sequence, independent from card packs. */
	UPROPERTY(SaveGame)
	int32 WeaponRuneRefreshSequence = 0;

	/** Added in v17. Per-encounter refresh budget state. */
	UPROPERTY(SaveGame)
	int32 WeaponRuneRefreshEncounterIndex = INDEX_NONE;

	UPROPERTY(SaveGame)
	int32 WeaponRuneRefreshesUsed = 0;

	/** Added in v6. Older saves deterministically migrate to automatic attack. */
	UPROPERTY(SaveGame)
	bool bAutomaticAttackMode = true;

	/**
	 * Legacy v4 migration input only. v5 never writes this array and the runtime never keeps a
	 * rolling history; it exists solely so a v4 archive can still be deserialized and migrated.
	 */
	UPROPERTY(SaveGame)
	TArray<FReEchoRecording> RecordingHistory;

	/**
	 * Legacy v4 migration input only. v5 never writes this and it is no longer a live authority;
	 * SelectedReplayIds carries the "which echo replays next" decision instead.
	 */
	UPROPERTY(SaveGame)
	FGuid AnchorId;

	/** True when PendingRecording holds a finished encounter still awaiting store-or-skip. */
	UPROPERTY(SaveGame)
	bool bHasPendingRecording = false;

	UPROPERTY(SaveGame)
	FReEchoRecording PendingRecording;

	/** True when LatestCompletedRecording holds the rolling "previous encounter" echo. */
	UPROPERTY(SaveGame)
	bool bHasLatestCompletedRecording = false;

	/** Rolling previous-encounter echo. Overwritten every successful encounter, occupies no slot. */
	UPROPERTY(SaveGame)
	FReEchoRecording LatestCompletedRecording;

	/** v9 rolling recording immediately preceding LatestCompletedRecording. */
	UPROPERTY(SaveGame)
	bool bHasPreviousCompletedRecording = false;

	UPROPERTY(SaveGame)
	FReEchoRecording PreviousCompletedRecording;

	/** Only echoes the player explicitly chose to store; identity is FReEchoRecording::Id, not index. */
	UPROPERTY(SaveGame)
	TArray<FReEchoRecording> StoredEchoes;

	/** Stable ids selected for the next encounter's specific replay; subset of StoredEchoes. */
	UPROPERTY(SaveGame)
	TArray<FGuid> SelectedReplayIds;

	UPROPERTY(SaveGame)
	int32 StorageCapacity = ReEchoEchoStorage::DefaultStorageCapacity;

	/** Zero means specific replay is not unlocked yet. */
	UPROPERTY(SaveGame)
	int32 SpecificReplayLimit = ReEchoEchoStorage::SpecificReplayUnavailable;

	/** Present only for an explicit in-encounter save-and-quit. */
	UPROPERTY(SaveGame)
	FReEchoEncounterRuntimeState EncounterRuntimeState;
};
