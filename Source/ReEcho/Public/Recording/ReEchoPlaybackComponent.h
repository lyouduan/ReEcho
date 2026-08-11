#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoPlaybackComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FReEchoReplaySkill, FName, SkillId, FVector, Position, float, RecordedTime);

/** 回响回放器：使用录制中的锁定构筑，并按遭遇时间依次补播技能事件。 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoPlaybackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoPlaybackComponent();

	UPROPERTY(BlueprintAssignable)
	FReEchoReplaySkill OnReplaySkill;

	/** 装载录制并重置所有事件游标，供新回响从头播放。 */
	UFUNCTION(BlueprintCallable)
	void LoadRecording(const FReEchoRecording& InRecording);

	/** 广播指定时间之前尚未触发的事件，避免低帧率时漏播。 */
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
};
