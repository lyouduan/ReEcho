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
	static constexpr int32 CurrentSaveVersion = 2;

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

	UPROPERTY(SaveGame)
	TArray<FReEchoRecording> RecordingHistory;

	UPROPERTY(SaveGame)
	FGuid AnchorId;

	/** Present only for an explicit in-encounter save-and-quit. */
	UPROPERTY(SaveGame)
	FReEchoEncounterRuntimeState EncounterRuntimeState;
};
