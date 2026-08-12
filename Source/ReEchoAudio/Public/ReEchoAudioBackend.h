#pragma once

#include "CoreMinimal.h"
#include "ReEchoAudioTypes.h"

/**
 * Playback backend abstraction.
 *
 * The policy engine performs all pure strategy (event resolution, cooldown,
 * concurrency, priority, state, volume) and then hands resolved play commands
 * to a backend. Splitting the backend lets automation use a fake/no-device
 * backend that needs no speakers and no real audio assets.
 */
class REECHOAUDIO_API IReEchoAudioBackend
{
public:
	virtual ~IReEchoAudioBackend() = default;

	/** True if this backend can actually emit sound on the current device. */
	virtual bool IsAvailable() const = 0;

	/** Play a one-shot. Returns a voice handle (>0) or 0 if nothing was played. */
	virtual uint32 PlayOneShot(const FReEchoAudioPlayCommand& Command) = 0;

	/** Stop a previously started one-shot voice (best-effort for real devices). */
	virtual void StopOneShot(uint32 VoiceHandle) = 0;

	/** Start a looping state voice. Returns a handle (>0) or 0. */
	virtual uint32 StartLoop(const FReEchoAudioPlayCommand& Command) = 0;

	/** Stop a looping voice, fading out over FadeOutSeconds. */
	virtual void StopLoop(uint32 LoopHandle, float FadeOutSeconds) = 0;

	/** Update the volume of an active looping voice (called when buses change). */
	virtual void SetLoopVolume(uint32 LoopHandle, float Volume) = 0;

	/** Estimated one-shot duration, used for concurrency expiry. */
	virtual float GetVoiceDuration(const FReEchoAudioPlayCommand& Command) const = 0;
};

namespace ReEchoAudio
{
	/** Construct the real UE playback backend. Exposed for tests/providers. */
	REECHOAUDIO_API TSharedRef<IReEchoAudioBackend> CreateUnrealBackend();
}
