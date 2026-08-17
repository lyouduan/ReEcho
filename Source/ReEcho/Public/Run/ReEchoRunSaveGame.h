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
	/** v9 replaces legacy card ids with the versioned ReEchoCards build/runtime state. */
	static constexpr int32 CurrentSaveVersion = 9;

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

	UPROPERTY(SaveGame)
	FReEchoBuildSnapshot CurrentBuild;

	UPROPERTY(SaveGame)
	TArray<FName> InventoryItems;

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
