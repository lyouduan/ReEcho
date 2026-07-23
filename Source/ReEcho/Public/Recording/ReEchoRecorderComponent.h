#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoRecorderComponent.generated.h"

UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHO_API UReEchoRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoRecorderComponent();

	UFUNCTION(BlueprintCallable)
	void BeginRecording(int32 EncounterIndex, FName MapId, int32 RandomSeed, const FReEchoBuildSnapshot& Snapshot);

	UFUNCTION(BlueprintCallable)
	void UpdateBuildSnapshot(const FReEchoBuildSnapshot& Snapshot);
	UFUNCTION(BlueprintCallable)
	void AdvanceRecording(float EncounterTime, const FVector& Position);

	UFUNCTION(BlueprintCallable)
	void RecordWeaponChange(float EncounterTime, FName WeaponId);
	UFUNCTION(BlueprintCallable)
	void RecordSkill(float EncounterTime, FVector Position, FName SkillId);

	UFUNCTION(BlueprintCallable)
	FReEchoRecording FinishRecording(float Duration);

	UFUNCTION(BlueprintPure)
	bool IsRecording() const;

	UFUNCTION(BlueprintPure)
	const FReEchoRecording& GetRecording() const;

private:
	UPROPERTY(VisibleAnywhere)
	FReEchoRecording Recording;

	float NextSampleTime = 0.f;
	float SampleInterval = 0.05f;
	bool bRecording = false;
};
