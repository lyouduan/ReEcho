#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/SaveGame.h"
#include "ReEchoPlayerProgressSaveGame.generated.h"

/** Account-level presentation progress that survives deletion of individual run slots. */
UCLASS()
class REECHO_API UReEchoPlayerProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 2;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(SaveGame)
	bool bHasViewedStage01To02Cg = false;

	/** Added in v2. Used only as the default for the next newly-created run. */
	UPROPERTY(SaveGame)
	EReEchoRunDifficulty PreferredDifficulty = EReEchoRunDifficulty::Standard;
};
