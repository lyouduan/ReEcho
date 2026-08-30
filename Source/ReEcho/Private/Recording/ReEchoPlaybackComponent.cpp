#include "Recording/ReEchoPlaybackComponent.h"

UReEchoPlaybackComponent::UReEchoPlaybackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoPlaybackComponent::LoadRecording(const FReEchoRecording& InRecording)
{
	Recording = InRecording;
	NextSkillIndex = 0;
	PlaybackCycle = INDEX_NONE;
}

void UReEchoPlaybackComponent::SetLoopDuration(const float InLoopDurationSeconds)
{
	LoopDurationSeconds = FMath::IsFinite(InLoopDurationSeconds) ? FMath::Max(0.0f, InLoopDurationSeconds) : 0.0f;
	NextSkillIndex = 0;
	PlaybackCycle = INDEX_NONE;
}

float UReEchoPlaybackComponent::ResolvePlaybackTime(const float EncounterTime) const
{
	const float SafeEncounterTime = FMath::IsFinite(EncounterTime) ? FMath::Max(0.0f, EncounterTime) : 0.0f;
	return LoopDurationSeconds > UE_SMALL_NUMBER ? FMath::Fmod(SafeEncounterTime, LoopDurationSeconds)
	                                                   : SafeEncounterTime;
}

void UReEchoPlaybackComponent::BroadcastSkillsThrough(const float PlaybackTime)
{
	while (Recording.Skills.IsValidIndex(NextSkillIndex) &&
	       Recording.Skills[NextSkillIndex].Time <= PlaybackTime + KINDA_SMALL_NUMBER)
	{
		const FReEchoSkillEvent& Event = Recording.Skills[NextSkillIndex++];
		OnReplaySkill.Broadcast(Event.SkillId, Event.Position, Event.Time);
	}
}

void UReEchoPlaybackComponent::AdvancePlayback(const float EncounterTime)
{
	const float SafeEncounterTime = FMath::IsFinite(EncounterTime) ? FMath::Max(0.0f, EncounterTime) : 0.0f;
	const float PlaybackTime = ResolvePlaybackTime(SafeEncounterTime);
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(Recording.EvaluatePosition(PlaybackTime));
	}

	if (LoopDurationSeconds <= UE_SMALL_NUMBER)
	{
		BroadcastSkillsThrough(PlaybackTime);
		return;
	}

	const int64 RequestedCycle = FMath::FloorToInt64(SafeEncounterTime / LoopDurationSeconds);
	if (PlaybackCycle == INDEX_NONE || RequestedCycle < PlaybackCycle)
	{
		// A freshly restored Echo starts at the saved cycle without replaying every completed cycle.
		PlaybackCycle = RequestedCycle;
		NextSkillIndex = 0;
	}
	else
	{
		while (PlaybackCycle < RequestedCycle)
		{
			// Finish the previous cycle before resetting, so a low frame-rate boundary cannot drop tail events.
			BroadcastSkillsThrough(LoopDurationSeconds);
			++PlaybackCycle;
			NextSkillIndex = 0;
		}
	}
	BroadcastSkillsThrough(PlaybackTime);
}

const FReEchoBuildSnapshot& UReEchoPlaybackComponent::GetHistoricalBuild() const
{
	return Recording.BuildSnapshot;
}

int32 UReEchoPlaybackComponent::GetSourceEncounter() const
{
	return Recording.EncounterIndex;
}

FVector UReEchoPlaybackComponent::EvaluateRecordedPosition(const float EncounterTime) const
{
	return Recording.EvaluatePosition(ResolvePlaybackTime(EncounterTime));
}
