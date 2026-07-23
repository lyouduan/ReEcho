#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoEncounterDirector.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoFixedStep, float, FixedDeltaSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoEncounterEnded);

UCLASS(Blueprintable)
class REECHO_API AReEchoEncounterDirector : public AActor
{
	GENERATED_BODY()

public:
	AReEchoEncounterDirector();

	UPROPERTY(BlueprintAssignable)
	FReEchoFixedStep OnFixedStep;

	UPROPERTY(BlueprintAssignable)
	FReEchoEncounterEnded OnEncounterEnded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EncounterTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSetupPhase = true;

	UFUNCTION(BlueprintCallable)
	void StartEncounter();

	UFUNCTION(BlueprintCallable)
	void EndEncounter();

	UFUNCTION(BlueprintCallable)
	void SetPaused(bool bInPaused);

	UFUNCTION(BlueprintPure)
	float GetRemainingTime() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	float Accumulator = 0.f;
	float FixedDelta = 1.f / 60.f;
	bool bRunning = false;
	bool bSimulationPaused = false;
};
