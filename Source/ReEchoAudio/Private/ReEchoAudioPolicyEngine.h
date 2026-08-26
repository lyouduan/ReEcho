#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "ReEchoAudioTypes.h"
#include "ReEchoAudioCatalog.h"
#include "ReEchoAudioBackend.h"

/** One active one-shot voice tracked for concurrency/expiry/preemption. */
struct FReEchoAudioActiveVoice
{
	FName EventId = NAME_None;
	EReEchoAudioBus Bus = EReEchoAudioBus::CombatSfx;
	uint32 VoiceHandle = 0;
	float EndTime = 0.0f;
	int32 Priority = 0;
};

/** Runtime state of a single long-loop channel. */
struct FReEchoAudioChannelRuntime
{
	FName CurrentStateId = NAME_None;
	FName CurrentVariantId = NAME_None;
	uint32 CurrentLoopHandle = 0;
	TArray<uint32> PendingStopHandles;
};

/**
 * Pure audio policy engine. Holds no UObject world state and talks to the
 * playback backend only through IReEchoAudioBackend, so it is fully unit
 * testable with a fake backend and no audio device/assets.
 */
class REECHOAUDIO_API FReEchoAudioPolicyEngine
{
public:
	FReEchoAudioPolicyEngine();

	void SetCatalogProvider(TSharedPtr<IReEchoAudioCatalogProvider> InProvider);
	void SetBackend(TSharedPtr<IReEchoAudioBackend> InBackend);

	/** Advance the internal audio clock and expire finished voices. */
	void Update(float DeltaSeconds);

	// ---- One-shot events ----
	void PostEvent(const FReEchoAudioEventRequest& Request, UWorld* World = nullptr);

	// ---- State channels ----
	void SetState(EReEchoAudioChannel Channel, FName StateId, UWorld* World = nullptr);
	void SetState(EReEchoAudioChannel Channel, FName StateId, FName VariantId, UWorld* World = nullptr);
	void StopState(EReEchoAudioChannel Channel);
	FName GetCurrentState(EReEchoAudioChannel Channel) const;
	FName GetCurrentStateVariant(EReEchoAudioChannel Channel) const;

	// ---- Volume buses ----
	void SetMasterVolume(float Volume);
	void SetBusVolume(EReEchoAudioBus Bus, float Volume);
	void SetBusMuted(EReEchoAudioBus Bus, bool bMuted);
	float GetMasterVolume() const;
	float GetBusVolume(EReEchoAudioBus Bus) const;
	bool IsBusMuted(EReEchoAudioBus Bus) const;

	/** Final one-shot volume = Master * Bus * EventBase, clamped to [0,1]. */
	float ComputeOneShotVolume(EReEchoAudioBus Bus, float EventBaseVolume) const;

	// ---- Diagnostics / test seams ----
	int32 GetUnknownEventWarningCount() const { return UnknownEventWarningCount; }
	int32 GetActiveVoiceCount(FName EventId) const;
	int32 GetTotalActiveVoices() const;
	int32 GetActiveLoopCount() const;

	/** Tear down all active voices/loops (called on subsystem Deinitialize). */
	void Shutdown();

private:
	static constexpr int32 BusCount = static_cast<int32>(EReEchoAudioBus::UiSfx) + 1;
	static constexpr int32 ChannelCount = static_cast<int32>(EReEchoAudioChannel::Ambience) + 1;
	static constexpr float StateFadeOutSeconds = 1.2f;

	void ExpireVoices();
	void ApplyBusVolumeToLoops();

	TSharedPtr<IReEchoAudioCatalogProvider> CatalogProvider;
	TSharedPtr<IReEchoAudioBackend> Backend;

	float BusVolumes[BusCount];
	bool BusMuted[BusCount];

	TMap<FName, float> LastPlayTime;

	/** All active one-shot voices, tracked for per-event concurrency,
	 *  same-bus priority preemption and expiry. */
	TArray<FReEchoAudioActiveVoice> ActiveVoices;
	FReEchoAudioChannelRuntime Channels[ChannelCount];

	TSet<FName> WarnedUnknownEvents;
	int32 UnknownEventWarningCount = 0;

	float AudioClock = 0.0f;

	/** Audio-only random stream, isolated from any gameplay RNG. */
	FRandomStream AudioRandom;
};
