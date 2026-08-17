#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoEncounterDirector.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoFixedStep, float, FixedDeltaSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoEncounterEnded);

/** 遭遇流程时钟：以固定步长驱动录制与回放，并统一广播遭遇结束。 */
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
	/** Resume a suspended encounter at its saved deterministic clock time. */
	void ResumeEncounter(float SavedEncounterTime);

	/** 主动结束当前遭遇；重复调用不会再次广播结束事件。 */
	UFUNCTION(BlueprintCallable)
	void EndEncounter();

	/** Boss 遭遇可在标准时长后继续，直到 GameMode 根据胜负条件主动结束。 */
	UFUNCTION(BlueprintCallable)
	void SetEndsOnDuration(bool bInEndsOnDuration);
	/** Configure the current encounter's authoritative table-driven duration and end policy. */
	void ConfigureEncounter(float InDurationSeconds, bool bInEndsOnDuration);

	UFUNCTION(BlueprintCallable)
	void SetPaused(bool bInPaused);

	UFUNCTION(BlueprintPure)
	float GetRemainingTime() const;
#if WITH_DEV_AUTOMATION_TESTS
	void AdvanceForTesting(float DeltaSeconds)
	{
		Tick(DeltaSeconds);
	}
#endif

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	float Accumulator = 0.f;
	float FixedDelta = 1.f / 60.f;
	bool bRunning = false;
	bool bSimulationPaused = false;
	bool bEndsOnDuration = true;
	float EncounterDurationSeconds = 30.0f;
};
