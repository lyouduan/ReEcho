#include "ReEchoAudioPolicyEngine.h"

FReEchoAudioPolicyEngine::FReEchoAudioPolicyEngine()
{
	for (int32 i = 0; i < BusCount; ++i)
	{
		BusVolumes[i] = 1.0f;
		BusMuted[i] = false;
	}
	AudioRandom = FRandomStream(0x2E3C0A55u);
}

void FReEchoAudioPolicyEngine::SetCatalogProvider(TSharedPtr<IReEchoAudioCatalogProvider> InProvider)
{
	CatalogProvider = InProvider;
}

void FReEchoAudioPolicyEngine::SetBackend(TSharedPtr<IReEchoAudioBackend> InBackend)
{
	Backend = InBackend;
}

void FReEchoAudioPolicyEngine::Update(float DeltaSeconds)
{
	AudioClock += DeltaSeconds;
	ExpireVoices();
}

float FReEchoAudioPolicyEngine::ComputeOneShotVolume(EReEchoAudioBus Bus, float EventBaseVolume) const
{
	float Volume = BusMuted[static_cast<int32>(EReEchoAudioBus::Master)]
	                   ? 0.0f
	                   : BusVolumes[static_cast<int32>(EReEchoAudioBus::Master)];
	Volume *= BusMuted[static_cast<int32>(Bus)] ? 0.0f : BusVolumes[static_cast<int32>(Bus)];
	Volume *= EventBaseVolume;
	return FMath::Clamp(Volume, 0.0f, 1.0f);
}

void FReEchoAudioPolicyEngine::PostEvent(const FReEchoAudioEventRequest& Request, UWorld* World)
{
	if (!Backend.IsValid() || !CatalogProvider.IsValid())
	{
		return;
	}

	const FReEchoAudioEventDefinition* Def = CatalogProvider->FindDefinition(Request.EventId, Request.VariantId);
	if (Def == nullptr)
	{
		// Unknown event: warn at most once per id, then degrade to silent no-op.
		if (!WarnedUnknownEvents.Contains(Request.EventId))
		{
			WarnedUnknownEvents.Add(Request.EventId);
			++UnknownEventWarningCount;
#if !UE_BUILD_SHIPPING
			UE_LOG(LogReEchoAudio,
			       Warning,
			       TEXT("ReEchoAudio: unknown event id '%s' has no catalog definition; ignoring."),
			       *Request.EventId.ToString());
#endif
		}
		return;
	}

	Update(0.0f); // expire before evaluating budgets

	// Cooldown
	if (Def->CooldownSeconds > 0.0f)
	{
		if (const float* Last = LastPlayTime.Find(Request.EventId))
		{
			if (AudioClock - (*Last) < Def->CooldownSeconds)
			{
				return; // dropped silently (cooldown budget)
			}
		}
	}

	// Concurrency + priority preemption.
	// Per-event concurrency limit is enforced first. When the incoming event is
	// at its own limit, it may preempt the lowest-priority active voice on the
	// SAME BUS (a bus is the real shared resource). Lower-priority events can
	// thus be evicted to make room for higher-priority ones.
	if (Def->MaxConcurrency > 0)
	{
		int32 SameEventCount = 0;
		for (const FReEchoAudioActiveVoice& V : ActiveVoices)
		{
			if (V.EventId == Request.EventId)
			{
				++SameEventCount;
			}
		}

		if (SameEventCount >= Def->MaxConcurrency)
		{
			int32 LowestIdx = INDEX_NONE;
			for (int32 i = 0; i < ActiveVoices.Num(); ++i)
			{
				if (ActiveVoices[i].Bus != Def->Bus)
				{
					continue;
				}
				if (LowestIdx == INDEX_NONE || ActiveVoices[i].Priority < ActiveVoices[LowestIdx].Priority ||
				    (ActiveVoices[i].Priority == ActiveVoices[LowestIdx].Priority &&
				     ActiveVoices[i].EndTime < ActiveVoices[LowestIdx].EndTime))
				{
					LowestIdx = i;
				}
			}

			// Incoming only wins if strictly higher than the lowest active priority.
			if (LowestIdx == INDEX_NONE || Def->Priority <= ActiveVoices[LowestIdx].Priority)
			{
				return; // dropped: not worth preempting
			}
			Backend->StopOneShot(ActiveVoices[LowestIdx].VoiceHandle);
			ActiveVoices.RemoveAt(LowestIdx);
		}
	}

	// Build the resolved play command.
	FReEchoAudioPlayCommand Command;
	Command.EventId = Request.EventId;
	Command.Sound = Def->Sound;
	Command.Bus = Def->Bus;
	Command.bSpatial3D = Def->bSpatial3D;
	Command.Location = Request.WorldLocation;
	Command.StartTimeSeconds = Def->StartTimeSeconds;
	Command.Volume = ComputeOneShotVolume(Def->Bus, Def->BaseVolume);
	Command.Pitch = AudioRandom.FRandRange(Def->PitchMin, Def->PitchMax);
	Command.PausePolicy = Def->PausePolicy;
	Command.AttenuationMin = Def->AttenuationMin;
	Command.AttenuationMax = Def->AttenuationMax;
	Command.VariantId = Request.VariantId;
	Command.SourceCategory = Request.SourceCategory;
	Command.World = World;

	const uint32 Handle = Backend->PlayOneShot(Command);
	LastPlayTime.Add(Request.EventId, AudioClock);

	if (Handle != 0 && Def->MaxConcurrency > 0)
	{
		FReEchoAudioActiveVoice Voice;
		Voice.EventId = Request.EventId;
		Voice.Bus = Def->Bus;
		Voice.VoiceHandle = Handle;
		Voice.Priority = Def->Priority;
		Voice.EndTime = AudioClock + Backend->GetVoiceDuration(Command);
		ActiveVoices.Add(Voice);
	}
}

void FReEchoAudioPolicyEngine::SetState(EReEchoAudioChannel Channel, FName StateId, UWorld* World)

{
	SetState(Channel, StateId, NAME_None, World);
}

void FReEchoAudioPolicyEngine::SetState(EReEchoAudioChannel Channel, FName StateId, FName VariantId, UWorld* World)
{
	if (!Backend.IsValid() || !CatalogProvider.IsValid())
	{
		return;
	}

	FReEchoAudioChannelRuntime& RT = Channels[static_cast<int32>(Channel)];

	// Idempotent only while the requested state has a live loop. A failed start
	// keeps the previous state and remains retryable.
	if (RT.CurrentStateId == StateId && RT.CurrentVariantId == VariantId && RT.CurrentLoopHandle != 0)
	{
		return;
	}

	const FReEchoAudioEventDefinition* Def = CatalogProvider->FindDefinition(StateId, VariantId);
	if (Def == nullptr)
	{
		if (!WarnedUnknownEvents.Contains(StateId))
		{
			WarnedUnknownEvents.Add(StateId);
			++UnknownEventWarningCount;
#if !UE_BUILD_SHIPPING
			UE_LOG(LogReEchoAudio,
			       Warning,
			       TEXT("ReEchoAudio: unknown state id '%s' has no catalog definition; preserving current state."),
			       *StateId.ToString());
#endif
		}
		return;
	}

	FReEchoAudioPlayCommand Command;
	Command.EventId = StateId;
	Command.VariantId = VariantId;
	Command.Sound = Def->Sound;
	Command.Bus = Def->Bus;
	Command.bSpatial3D = Def->bSpatial3D;
	Command.Location = FVector::ZeroVector;
	Command.StartTimeSeconds = Def->StartTimeSeconds;
	Command.Volume = ComputeOneShotVolume(Def->Bus, Def->BaseVolume);
	Command.Pitch = 1.0f;
	Command.PausePolicy = Def->PausePolicy;
	Command.FadeInSeconds = StateFadeOutSeconds;
	Command.AttenuationMin = Def->AttenuationMin;
	Command.AttenuationMax = Def->AttenuationMax;
	Command.World = World;

	const uint32 Handle = Backend->StartLoop(Command);
	if (Handle == 0)
	{
		return;
	}

	// Start the new loop before fading out the old one so a missing asset,
	// unavailable world or unavailable device cannot silence the live state.
	if (RT.CurrentLoopHandle != 0)
	{
		Backend->StopLoop(RT.CurrentLoopHandle, StateFadeOutSeconds);
		RT.PendingStopHandles.Add(RT.CurrentLoopHandle);
	}
	RT.CurrentLoopHandle = Handle;
	RT.CurrentStateId = StateId;
	RT.CurrentVariantId = VariantId;
}

void FReEchoAudioPolicyEngine::StopState(EReEchoAudioChannel Channel)
{
	if (!Backend.IsValid())
	{
		return;
	}

	FReEchoAudioChannelRuntime& RT = Channels[static_cast<int32>(Channel)];
	if (RT.CurrentLoopHandle != 0)
	{
		Backend->StopLoop(RT.CurrentLoopHandle, StateFadeOutSeconds);
		RT.PendingStopHandles.Add(RT.CurrentLoopHandle);
		RT.CurrentLoopHandle = 0;
	}
	RT.CurrentStateId = NAME_None;
	RT.CurrentVariantId = NAME_None;
}

FName FReEchoAudioPolicyEngine::GetCurrentState(EReEchoAudioChannel Channel) const
{
	return Channels[static_cast<int32>(Channel)].CurrentStateId;
}

FName FReEchoAudioPolicyEngine::GetCurrentStateVariant(EReEchoAudioChannel Channel) const
{
	return Channels[static_cast<int32>(Channel)].CurrentVariantId;
}

void FReEchoAudioPolicyEngine::SetMasterVolume(float Volume)
{
	SetBusVolume(EReEchoAudioBus::Master, Volume);
}

void FReEchoAudioPolicyEngine::SetBusVolume(EReEchoAudioBus Bus, float Volume)
{
	BusVolumes[static_cast<int32>(Bus)] = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyBusVolumeToLoops();
}

void FReEchoAudioPolicyEngine::SetBusMuted(EReEchoAudioBus Bus, bool bMuted)
{
	BusMuted[static_cast<int32>(Bus)] = bMuted;
	ApplyBusVolumeToLoops();
}

float FReEchoAudioPolicyEngine::GetMasterVolume() const
{
	return BusVolumes[static_cast<int32>(EReEchoAudioBus::Master)];
}

float FReEchoAudioPolicyEngine::GetBusVolume(EReEchoAudioBus Bus) const
{
	return BusVolumes[static_cast<int32>(Bus)];
}

bool FReEchoAudioPolicyEngine::IsBusMuted(EReEchoAudioBus Bus) const
{
	return BusMuted[static_cast<int32>(Bus)];
}

int32 FReEchoAudioPolicyEngine::GetActiveVoiceCount(FName EventId) const
{
	int32 Count = 0;
	for (const FReEchoAudioActiveVoice& V : ActiveVoices)
	{
		if (V.EventId == EventId)
		{
			++Count;
		}
	}
	return Count;
}

int32 FReEchoAudioPolicyEngine::GetTotalActiveVoices() const
{
	return ActiveVoices.Num();
}

int32 FReEchoAudioPolicyEngine::GetActiveLoopCount() const
{
	int32 Count = 0;
	for (int32 c = 0; c < ChannelCount; ++c)
	{
		if (Channels[c].CurrentLoopHandle != 0)
		{
			++Count;
		}
	}
	return Count;
}

void FReEchoAudioPolicyEngine::Shutdown()
{
	if (Backend.IsValid())
	{
		for (int32 c = 0; c < ChannelCount; ++c)
		{
			FReEchoAudioChannelRuntime& RT = Channels[c];
			if (RT.CurrentLoopHandle != 0)
			{
				Backend->StopLoop(RT.CurrentLoopHandle, 0.0f);
				RT.CurrentLoopHandle = 0;
			}
			// PendingStopHandles already had StopLoop() called on them when they
			// were added (in SetState/StopState), so re-stopping them here would
			// double-stop and break the one-stop-per-loop contract. Just clear.
			RT.PendingStopHandles.Empty();
		}
	}
	ActiveVoices.Empty();
	LastPlayTime.Empty();
}

void FReEchoAudioPolicyEngine::ExpireVoices()
{
	for (int32 i = ActiveVoices.Num() - 1; i >= 0; --i)
	{
		if (ActiveVoices[i].EndTime <= AudioClock)
		{
			ActiveVoices.RemoveAt(i);
		}
	}
}

void FReEchoAudioPolicyEngine::ApplyBusVolumeToLoops()
{
	if (!Backend.IsValid() || !CatalogProvider.IsValid())
	{
		return;
	}
	for (int32 c = 0; c < ChannelCount; ++c)
	{
		FReEchoAudioChannelRuntime& RT = Channels[c];
		if (RT.CurrentLoopHandle != 0)
		{
			const FReEchoAudioEventDefinition* Def =
			    CatalogProvider->FindDefinition(RT.CurrentStateId, RT.CurrentVariantId);
			if (Def != nullptr)
			{
				const float Volume = ComputeOneShotVolume(Def->Bus, Def->BaseVolume);
				Backend->SetLoopVolume(RT.CurrentLoopHandle, Volume);
			}
		}
	}
}
