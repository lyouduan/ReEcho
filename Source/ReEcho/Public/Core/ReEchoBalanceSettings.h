#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ReEchoBalanceSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ReEcho Balance"))
class REECHO_API UReEchoBalanceSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	float EncounterDuration = 30.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	float SetupDuration = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Simulation")
	float FixedStepHz = 60.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Recording")
	float RecordingHz = 20.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HpBase = 25.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HpPerPoint = 5.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseMoveSpeed = 210.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Echo")
	int32 RecordingHistoryLimit = 3;
};

