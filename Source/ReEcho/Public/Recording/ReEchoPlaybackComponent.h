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

	/** Zero keeps legacy one-shot playback. A positive duration loops position and skill events on that period. */
	void SetLoopDuration(float InLoopDurationSeconds);

	/** 广播指定时间之前尚未触发的事件，避免低帧率时漏播。 */
	UFUNCTION(BlueprintCallable)
	void AdvancePlayback(float EncounterTime);

	UFUNCTION(BlueprintPure)
	const FReEchoBuildSnapshot& GetHistoricalBuild() const;

	UFUNCTION(BlueprintPure)
	int32 GetSourceEncounter() const;

	/** Read-only path sample used by deterministic encounter spawn anchoring. */
	FVector EvaluateRecordedPosition(float EncounterTime) const;

#if WITH_DEV_AUTOMATION_TESTS
	int32 GetNextSkillIndexForTests() const
	{
		return NextSkillIndex;
	}

	int64 GetPlaybackCycleForTests() const
	{
		return PlaybackCycle;
	}

	float ResolvePlaybackTimeForTests(float EncounterTime) const
	{
		return ResolvePlaybackTime(EncounterTime);
	}
#endif

private:
	float ResolvePlaybackTime(float EncounterTime) const;
	void BroadcastSkillsThrough(float PlaybackTime);

	UPROPERTY()
	FReEchoRecording Recording;

	int32 NextSkillIndex = 0;
	int64 PlaybackCycle = INDEX_NONE;
	float LoopDurationSeconds = 0.0f;
};
