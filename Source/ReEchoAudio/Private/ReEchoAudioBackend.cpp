#include "ReEchoAudioBackend.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

/**
 * Real UE playback backend.
 *
 * Non-blocking by design: it only calls C.Sound.Get() (the soft pointer is
 * expected to already be resident; a future Plan34 provider preloads it). When
 * the sound is null or there is no world, every method degrades to a safe
 * no-op returning 0. This guarantees Plan33 never blocks on disk and never
 * crashes with no audio device/assets.
 *
 * Components are tracked via TWeakObjectPtr; spawned sounds auto-destroy on
 * finish/stop, so no manual lifetime bookkeeping is required.
 */
class FReEchoAudioUnrealBackend : public IReEchoAudioBackend
{
public:
	virtual bool IsAvailable() const override { return true; }

	virtual uint32 PlayOneShot(const FReEchoAudioPlayCommand& Command) override
	{
		if (Command.World == nullptr || Command.Sound.IsNull())
		{
			return 0;
		}
		USoundBase* Sound = Command.Sound.Get();
		if (Sound == nullptr)
		{
			return 0;
		}

		UAudioComponent* Comp = Command.bSpatial3D
			? UGameplayStatics::SpawnSoundAtLocation(
				Command.World, Sound, Command.Location, FRotator::ZeroRotator,
				Command.Volume, Command.Pitch, 0.0f, nullptr)
			: UGameplayStatics::SpawnSound2D(
				Command.World, Sound, Command.Volume, Command.Pitch, 0.0f);

		if (Comp == nullptr)
		{
			return 0;
		}
		Comp->SetUISound(Command.PausePolicy == EReEchoAudioPausePolicy::ContinueOnPause);

		const uint32 Handle = NextHandle++;
		ActiveComponents.Add(Handle, Comp);
		return Handle;
	}

	virtual void StopOneShot(uint32 VoiceHandle) override
	{
		if (TWeakObjectPtr<UAudioComponent>* Comp = ActiveComponents.Find(VoiceHandle))
		{
			if (Comp->IsValid())
			{
				(*Comp)->Stop();
			}
			ActiveComponents.Remove(VoiceHandle);
		}
	}

	virtual uint32 StartLoop(const FReEchoAudioPlayCommand& Command) override
	{
		if (Command.World == nullptr || Command.Sound.IsNull())
		{
			return 0;
		}
		USoundBase* Sound = Command.Sound.Get();
		if (Sound == nullptr)
		{
			return 0;
		}

		const float InitialVolume = Command.FadeInSeconds > 0.0f ? 0.0f : Command.Volume;
		UAudioComponent* Comp = Command.bSpatial3D
			? UGameplayStatics::SpawnSoundAtLocation(
				Command.World, Sound, Command.Location, FRotator::ZeroRotator,
				InitialVolume, 1.0f, 0.0f, nullptr)
			: UGameplayStatics::SpawnSound2D(
				Command.World, Sound, InitialVolume, 1.0f, 0.0f);

		if (Comp == nullptr)
		{
			return 0;
		}

		Comp->SetUISound(Command.PausePolicy == EReEchoAudioPausePolicy::ContinueOnPause);
		if (Command.FadeInSeconds > 0.0f)
		{
			Comp->FadeIn(Command.FadeInSeconds, Command.Volume);
		}
		const uint32 Handle = NextHandle++;
		LoopComponents.Add(Handle, Comp);
		return Handle;
	}

	virtual void StopLoop(uint32 LoopHandle, float FadeOutSeconds) override
	{
		if (TWeakObjectPtr<UAudioComponent>* Comp = LoopComponents.Find(LoopHandle))
		{
			if (Comp->IsValid())
			{
				(*Comp)->FadeOut(FadeOutSeconds, 0.0f);
			}
			LoopComponents.Remove(LoopHandle);
		}
	}

	virtual void SetLoopVolume(uint32 LoopHandle, float Volume) override
	{
		if (TWeakObjectPtr<UAudioComponent>* Comp = LoopComponents.Find(LoopHandle))
		{
			if (Comp->IsValid())
			{
				(*Comp)->SetVolumeMultiplier(Volume);
			}
		}
	}

	virtual float GetVoiceDuration(const FReEchoAudioPlayCommand& Command) const override
	{
		if (USoundBase* Sound = Command.Sound.Get())
		{
			return Sound->GetDuration();
		}
		return 1.0f;
	}

private:
	uint32 NextHandle = 1;
	TMap<uint32, TWeakObjectPtr<UAudioComponent>> ActiveComponents;
	TMap<uint32, TWeakObjectPtr<UAudioComponent>> LoopComponents;
};

namespace ReEchoAudio
{
	TSharedRef<IReEchoAudioBackend> CreateUnrealBackend()
	{
		return MakeShared<FReEchoAudioUnrealBackend>();
	}
}
