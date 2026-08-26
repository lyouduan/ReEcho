#include "ReEchoAudioBackend.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"

namespace
{
UAudioComponent*
CreateConfiguredComponent(const FReEchoAudioPlayCommand& Command, USoundBase* Sound, const bool bAutoDestroy)
{
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(
	    Command.World, Sound, Command.Volume, Command.Pitch, 0.0f, nullptr, false, bAutoDestroy);
	if (Component == nullptr)
	{
		return nullptr;
	}

	Component->SetUISound(Command.PausePolicy == EReEchoAudioPausePolicy::ContinueOnPause);
	Component->bAllowSpatialization = Command.bSpatial3D;
	if (Command.bSpatial3D)
	{
		Component->SetWorldLocation(Command.Location);

		FSoundAttenuationSettings Attenuation;
		Attenuation.bAttenuate = true;
		Attenuation.bSpatialize = true;
		Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
		Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Attenuation.AttenuationShapeExtents = FVector(Command.AttenuationMin, 0.0f, 0.0f);
		Attenuation.FalloffDistance = FMath::Max(0.0f, Command.AttenuationMax - Command.AttenuationMin);
		Component->SetOverrideAttenuation(true);
		Component->SetAttenuationOverrides(Attenuation);
	}
	return Component;
}
}

/**
 * Real UE playback backend.
 *
 * Non-blocking by design: it only calls Command.Sound.Get() after the catalog's
 * asynchronous preload. When the sound
 * is not resident or there is no world,
 * every method degrades to a safe no-op returning 0; callers can retry
 * without
 * blocking the gameplay thread or crashing with no audio device/assets.
 * Components are tracked via
 * TWeakObjectPtr; spawned sounds auto-destroy on finish/stop, so no manual lifetime bookkeeping is required.
 */
class FReEchoAudioUnrealBackend : public IReEchoAudioBackend
{
public:
	virtual bool IsAvailable() const override
	{
		return true;
	}

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

		UAudioComponent* Comp = CreateConfiguredComponent(Command, Sound, true);
		if (Comp == nullptr)
		{
			return 0;
		}
		Comp->Play(Command.StartTimeSeconds);

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

		// FadeIn drives UAudioComponent's internal volume fader; it does not
		// replace the component's VolumeMultiplier. Creating the component at
		// zero volume therefore leaves the final gain at 0 * FadeTarget forever.
		// Create without auto-playing, keep the resolved bus gain on the component,
		// then let FadeIn move the independent fader from 0 to unity.
		UAudioComponent* Comp = CreateConfiguredComponent(Command, Sound, true);
		if (Comp == nullptr)
		{
			return 0;
		}
		if (Command.FadeInSeconds > 0.0f)
		{
			Comp->FadeIn(Command.FadeInSeconds, 1.0f, Command.StartTimeSeconds);
		}
		else
		{
			Comp->Play(Command.StartTimeSeconds);
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
			return FMath::Max(0.0f, Sound->GetDuration() - Command.StartTimeSeconds);
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
