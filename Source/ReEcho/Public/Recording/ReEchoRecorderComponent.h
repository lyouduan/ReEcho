#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoRecorderComponent.generated.h"

/** 战斗录制器：按固定采样间隔保存位置，并记录主动技能事件。 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHO_API UReEchoRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoRecorderComponent();

	/** 清空旧数据并以当前遭遇信息和构筑快照开始一份新录制。 */
	UFUNCTION(BlueprintCallable)
	void BeginRecording(int32 EncounterIndex, FName MapId, int32 RandomSeed, const FReEchoBuildSnapshot& Snapshot);

	/** 推进采样时钟，并补齐当前时间之前尚未写入的位置样本。 */
	UFUNCTION(BlueprintCallable)
	void AdvanceRecording(float EncounterTime, const FVector& Position);

	UFUNCTION(BlueprintCallable)
	void RecordSkill(float EncounterTime, FVector Position, FName SkillId);

	/** 封存录制时长并返回完整记录，调用后停止继续采样。 */
	UFUNCTION(BlueprintCallable)
	FReEchoRecording FinishRecording(float Duration);
	/** Continue appending to a recording captured by save-and-quit. */
	void ResumeRecording(const FReEchoRecording& SavedRecording);

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
