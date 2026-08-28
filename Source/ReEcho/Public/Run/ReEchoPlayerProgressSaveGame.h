#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ReEchoPlayerProgressSaveGame.generated.h"

/** Account-level presentation progress that survives deletion of individual run slots. */
UCLASS()
class REECHO_API UReEchoPlayerProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(SaveGame)
	bool bHasViewedStage01To02Cg = false;
};
