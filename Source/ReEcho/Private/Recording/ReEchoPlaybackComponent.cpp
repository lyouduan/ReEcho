#include "Recording/ReEchoPlaybackComponent.h"

UReEchoPlaybackComponent::UReEchoPlaybackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UReEchoPlaybackComponent::LoadRecording(const FReEchoRecording& InRecording)
{
	Recording = InRecording;
	NextSkillIndex = 0;
	NextWeaponIndex = 0;
}

void UReEchoPlaybackComponent::AdvancePlayback(const float EncounterTime)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(Recording.EvaluatePosition(EncounterTime));
	}
	while (Recording.Skills.IsValidIndex(NextSkillIndex) &&
	       Recording.Skills[NextSkillIndex].Time <= EncounterTime + KINDA_SMALL_NUMBER)
	{
		const FReEchoSkillEvent& Event = Recording.Skills[NextSkillIndex++];
		OnReplaySkill.Broadcast(Event.SkillId, Event.Position, Event.Time);
	}

	while (Recording.WeaponChanges.IsValidIndex(NextWeaponIndex)
		&& Recording.WeaponChanges[NextWeaponIndex].Time
			<= EncounterTime + KINDA_SMALL_NUMBER)
	{
		const FReEchoWeaponEvent& Event =
			Recording.WeaponChanges[NextWeaponIndex++];
		OnReplayWeapon.Broadcast(Event.WeaponId, Event.Time);
	}
}

const FReEchoBuildSnapshot& UReEchoPlaybackComponent::GetHistoricalBuild() const
{
	return Recording.BuildSnapshot;
}

int32 UReEchoPlaybackComponent::GetSourceEncounter() const
{
	return Recording.EncounterIndex;
}

