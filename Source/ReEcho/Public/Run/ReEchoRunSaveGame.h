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
	/** v26 persists consumed rune offers separately from current ownership and synthesis counts. */
	static constexpr int32 CurrentSaveVersion = 26;

	/** Oldest layout this build can still migrate forward. */
	static constexpr int32 MinimumSupportedSaveVersion = 4;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	/** Added in v23. Zero-based stable logical slot used by the three-slot archive UI. */
	UPROPERTY(SaveGame)
	int32 LogicalSlotIndex = 0;

	/** Added in v23. UTC ticks at the most recent successful save. */
	UPROPERTY(SaveGame)
	int64 SavedAtUtcTicks = 0;

	/** Added in v23. Project-Saved-relative PNG path; missing files fall back to authored placeholder art. */
	UPROPERTY(SaveGame)
	FString PreviewScreenshotFileName;

	UPROPERTY(SaveGame)
	EReEchoRunPhase SavedPhase = EReEchoRunPhase::Planning;

	UPROPERTY(SaveGame)
	int32 EncounterIndex = 0;

	UPROPERTY(SaveGame)
	int32 TimeShards = 0;

	/** Added in v24. Generated once at run start and persisted so all content rolls remain reproducible. */
	UPROPERTY(SaveGame)
	int32 RunSeed = 0;

	/** Added in v12. Run-scoped card-offer stream; v24+ derives it from RunSeed. */
	UPROPERTY(SaveGame)
	int32 TraitOfferSeed = 0;

	/** Added in v18. Stable post-encounter/free three-choice page and per-slot refresh state. */
	UPROPERTY(SaveGame)
	int32 PendingTraitCardOfferEncounterIndex = INDEX_NONE;

	UPROPERTY(SaveGame)
	TArray<FName> PendingTraitCardIds;

	/** Added in v22. All cards displayed by the current free-choice group, including replaced cards. */
	UPROPERTY(SaveGame)
	TArray<FName> PendingTraitCardOfferHistoryIds;

	UPROPERTY(SaveGame)
	TArray<int32> PendingTraitCardRefreshUses;

	/** Added in v13. Per-enemy reward stream; v24+ derives it from RunSeed. */
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

	/**
	 * Copies of each rune currently held (backpack + equipped). OwnedPartIds is a de-duplicated ownership
	 * set and cannot express "two copies", so tier synthesis (2x I -> 1x II) counts copies here instead.
	 */
	UPROPERTY(SaveGame)
	TMap<FName, int32> RuneAcquisitionCounts;

	/** Added in v14. Stable weapon/rune offers for one encounter + refresh-sequence page. */
	UPROPERTY(SaveGame)
	int32 WeaponPartShopOfferEncounterIndex = INDEX_NONE;

	UPROPERTY(SaveGame)
	int32 WeaponPartShopOfferRefreshSequence = INDEX_NONE;

	/** Fixed three content ids; NAME_None represents an empty slot. */
	UPROPERTY(SaveGame)
	TArray<FName> WeaponPartShopOfferIds;

	/** Added in v26. Consumed offers on the saved encounter/refresh page, even if synthesis consumed the item. */
	UPROPERTY(SaveGame)
	TArray<FName> PurchasedWeaponPartOfferIds;

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
	 * Legacy v4 migration input only. Modern saves use CurrentBuild.CardState.Runtime.AnchorRecordingId.
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

	/** Compatibility container for the single time anchor. Modern saves contain at most one recording. */
	UPROPERTY(SaveGame)
	TArray<FReEchoRecording> StoredEchoes;

	/** Deprecated v5 compatibility field. Deserialized from old saves, ignored, and written empty. */
	UPROPERTY(SaveGame)
	TArray<FGuid> SelectedReplayIds;

	/** Deprecated multi-slot capacity retained only so old archives deserialize. New saves write zero. */
	UPROPERTY(SaveGame)
	int32 StorageCapacity = 0;

	/** Deprecated v5 compatibility field. Deserialized from old saves, ignored, and written as zero. */
	UPROPERTY(SaveGame)
	int32 SpecificReplayLimit = 0;

	/** Present only for an explicit in-encounter save-and-quit. */
	UPROPERTY(SaveGame)
	FReEchoEncounterRuntimeState EncounterRuntimeState;
};
