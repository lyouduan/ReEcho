#include "Recording/ReEchoRecorderComponent.h"

#include "Core/ReEchoBalanceSettings.h"

UReEchoRecorderComponent::UReEchoRecorderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoRecorderComponent::BeginRecording(const int32 EncounterIndex,
                                              const FName MapId,
                                              const int32 RandomSeed,
                                              const FReEchoBuildSnapshot& Snapshot)
{
	Recording = {};
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = EncounterIndex;
	Recording.MapId = MapId;
	Recording.RandomSeed = RandomSeed;
	Recording.BuildSnapshot = Snapshot;

	SampleInterval = 1.0f / FMath::Max(1.0f, GetDefault<UReEchoBalanceSettings>()->RecordingHz);
	NextSampleTime = 0.0f;
	bRecording = true;
}

void UReEchoRecorderComponent::AdvanceRecording(const float EncounterTime, const FVector& Position)
{
	if (!bRecording)
	{
		return;
	}

	while (EncounterTime + KINDA_SMALL_NUMBER >= NextSampleTime)
	{
		Recording.Positions.Add({NextSampleTime, Position});
		NextSampleTime += SampleInterval;
	}
}

void UReEchoRecorderComponent::RecordSkill(const float EncounterTime, const FVector Position, const FName SkillId)
{
	if (bRecording)
	{
		Recording.Skills.Add({EncounterTime, Position, SkillId});
	}
}

FReEchoRecording UReEchoRecorderComponent::FinishRecording(const float Duration)
{
	bRecording = false;
	Recording.Duration = Duration;
	return Recording;
}

void UReEchoRecorderComponent::ResumeRecording(const FReEchoRecording& SavedRecording)
{
	Recording = SavedRecording;
	SampleInterval = 1.0f / FMath::Max(1.0f, GetDefault<UReEchoBalanceSettings>()->RecordingHz);
	NextSampleTime = Recording.Positions.IsEmpty() ? 0.0f : Recording.Positions.Last().Time + SampleInterval;
	bRecording = true;
}

bool UReEchoRecorderComponent::IsRecording() const
{
	return bRecording;
}

const FReEchoRecording& UReEchoRecorderComponent::GetRecording() const
{
	return Recording;
}
