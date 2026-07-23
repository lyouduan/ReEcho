#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoPlaybackComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FReEchoReplaySkill,
	FName,
	SkillId,
	FVector,
	Position,
	float,
	RecordedTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FReEchoReplayWeapon,
	FName,
	WeaponId,
	float,
	RecordedTime);

UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHO_API UReEchoPlaybackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoPlaybackComponent();

	UPROPERTY(BlueprintAssignable)
	FReEchoReplaySkill OnReplaySkill;

	UPROPERTY(BlueprintAssignable)
	FReEchoReplayWeapon OnReplayWeapon;

	UFUNCTION(BlueprintCallable)
	void LoadRecording(const FReEchoRecording& InRecording);

	UFUNCTION(BlueprintCallable)
	void AdvancePlayback(float EncounterTime);

	UFUNCTION(BlueprintPure)
	const FReEchoBuildSnapshot& GetHistoricalBuild() const;

	UFUNCTION(BlueprintPure)
	int32 GetSourceEncounter() const;

private:
	UPROPERTY()
	FReEchoRecording Recording;

	int32 NextSkillIndex = 0;
	int32 NextWeaponIndex = 0;
};