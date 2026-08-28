#include "ReEchoAudioBackend.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"

namespace
{
enum class EReEchoAudioPlaybackEndReason : uint8
{
	NaturalFinish,
	ExplicitStop,
	FadeOutComplete,
	ComponentUnavailable,
	BackendShutdown,
};

const TCHAR* ToLogString(const EReEchoAudioPlaybackEndReason Reason)
{
	switch (Reason)
	{
		case EReEchoAudioPlaybackEndReason::NaturalFinish:
			return TEXT("NaturalFinish");
		case EReEchoAudioPlaybackEndReason::ExplicitStop:
			return TEXT("ExplicitStop");
		case EReEchoAudioPlaybackEndReason::FadeOutComplete:
			return TEXT("FadeOutComplete");
		case EReEchoAudioPlaybackEndReason::ComponentUnavailable:
			return TEXT("ComponentUnavailable");
		case EReEchoAudioPlaybackEndReason::BackendShutdown:
			return TEXT("BackendShutdown");
	}
	return TEXT("Unknown");
}

const TCHAR* ToLogString(const bool bLoop)
{
	return bLoop ? TEXT("Loop") : TEXT("OneShot");
}

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
class FReEchoAudioUnrealBackend : public IReEchoAudioBackend, public TSharedFromThis<FReEchoAudioUnrealBackend>
{
public:
	virtual ~FReEchoAudioUnrealBackend() override
	{
		TArray<uint32> Handles;
		ActivePlaybackContexts.GetKeys(Handles);
		for (const uint32 Handle : Handles)
		{
			CompletePlayback(Handle, EReEchoAudioPlaybackEndReason::BackendShutdown);
		}
	}

	virtual bool IsAvailable() const override
	{
		return true;
	}

	virtual uint32 PlayOneShot(const FReEchoAudioPlayCommand& Command) override
	{
		if (Command.World == nullptr)
		{
			LogPlaybackStartFailed(Command, TEXT("MissingWorld"), false);
			return 0;
		}
		if (Command.Sound.IsNull())
		{
			LogPlaybackStartFailed(Command, TEXT("MissingSoundPath"), false);
			return 0;
		}
		USoundBase* Sound = Command.Sound.Get();
		if (Sound == nullptr)
		{
			LogPlaybackStartFailed(Command, TEXT("SoundNotResident"), false);
			return 0;
		}

		UAudioComponent* Comp = CreateConfiguredComponent(Command, Sound, true);
		if (Comp == nullptr)
		{
			LogPlaybackStartFailed(Command, TEXT("ComponentCreationFailed"), false);
			return 0;
		}

		const uint32 Handle = NextHandle++;
		ActiveComponents.Add(Handle, Comp);
		RegisterPlayback(Handle, Command, Comp, false);
		LogPlaybackStarted(Handle, Command, false);
		Comp->Play(Command.StartTimeSeconds);
		return Handle;
	}

	virtual void StopOneShot(uint32 VoiceHandle) override
	{
		if (TWeakObjectPtr<UAudioComponent>* Comp = ActiveComponents.Find(VoiceHandle))
		{
			const TWeakObjectPtr<UAudioComponent> Component = *Comp;
			CompletePlayback(VoiceHandle,
			                 Component.IsValid() ? EReEchoAudioPlaybackEndReason::ExplicitStop
			                                     : EReEchoAudioPlaybackEndReason::ComponentUnavailable);
			if (Component.IsValid())
			{
				Component->Stop();
			}
		}
	}

	virtual uint32 StartLoop(const FReEchoAudioPlayCommand& Command) override
	{
		if (Command.World == nullptr)
		{
			LogPlaybackStartFailed(Command, TEXT("MissingWorld"), true);
			return 0;
		}
		if (Command.Sound.IsNull())
		{
			LogPlaybackStartFailed(Command, TEXT("MissingSoundPath"), true);
			return 0;
		}
		USoundBase* Sound = Command.Sound.Get();
		if (Sound == nullptr)
		{
			LogPlaybackStartFailed(Command, TEXT("SoundNotResident"), true);
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
			LogPlaybackStartFailed(Command, TEXT("ComponentCreationFailed"), true);
			return 0;
		}
		const uint32 Handle = NextHandle++;
		LoopComponents.Add(Handle, Comp);
		RegisterPlayback(Handle, Command, Comp, true);
		LogPlaybackStarted(Handle, Command, true);
		if (Command.FadeInSeconds > 0.0f)
		{
			Comp->FadeIn(Command.FadeInSeconds, 1.0f, Command.StartTimeSeconds);
		}
		else
		{
			Comp->Play(Command.StartTimeSeconds);
		}
		return Handle;
	}

	virtual void StopLoop(uint32 LoopHandle, float FadeOutSeconds) override
	{
		if (TWeakObjectPtr<UAudioComponent>* Comp = LoopComponents.Find(LoopHandle))
		{
			const TWeakObjectPtr<UAudioComponent> Component = *Comp;
			LoopComponents.Remove(LoopHandle);
			if (FActivePlaybackContext* Context = ActivePlaybackContexts.Find(LoopHandle))
			{
				Context->PendingEndReason = EReEchoAudioPlaybackEndReason::FadeOutComplete;
			}
			if (Component.IsValid())
			{
				Component->FadeOut(FadeOutSeconds, 0.0f);
			}
			else
			{
				if (FActivePlaybackContext* Context = ActivePlaybackContexts.Find(LoopHandle))
				{
					Context->PendingEndReason = EReEchoAudioPlaybackEndReason::ComponentUnavailable;
				}
				CompletePlayback(LoopHandle, EReEchoAudioPlaybackEndReason::ComponentUnavailable);
			}
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
	struct FActivePlaybackContext
	{
		FName EventId;
		FName VariantId;
		FString AssetPath;
		bool bLoop = false;
		double StartedAtSeconds = 0.0;
		EReEchoAudioPlaybackEndReason PendingEndReason = EReEchoAudioPlaybackEndReason::NaturalFinish;
	};

	static void LogPlaybackStartFailed(const FReEchoAudioPlayCommand& Command, const TCHAR* Reason, const bool bLoop)
	{
		UE_LOG(LogReEchoAudio,
		       Warning,
		       TEXT("[PlaybackStartFailed] Kind=%s EventId=%s VariantId=%s Asset=%s Reason=%s"),
		       ToLogString(bLoop),
		       *Command.EventId.ToString(),
		       *Command.VariantId.ToString(),
		       *Command.Sound.ToSoftObjectPath().ToString(),
		       Reason);
	}

	static void LogPlaybackStarted(const uint32 Handle, const FReEchoAudioPlayCommand& Command, const bool bLoop)
	{
		UE_LOG(LogReEchoAudio,
		       Log,
		       TEXT("[PlaybackStart] Handle=%u Kind=%s EventId=%s VariantId=%s Asset=%s StartTime=%.3f "
		            "Volume=%.3f Pitch=%.3f"),
		       Handle,
		       ToLogString(bLoop),
		       *Command.EventId.ToString(),
		       *Command.VariantId.ToString(),
		       *Command.Sound.ToSoftObjectPath().ToString(),
		       Command.StartTimeSeconds,
		       Command.Volume,
		       Command.Pitch);
	}

	void RegisterPlayback(const uint32 Handle,
	                      const FReEchoAudioPlayCommand& Command,
	                      UAudioComponent* Component,
	                      const bool bLoop)
	{
		FActivePlaybackContext& Context = ActivePlaybackContexts.Add(Handle);
		Context.EventId = Command.EventId;
		Context.VariantId = Command.VariantId;
		Context.AssetPath = Command.Sound.ToSoftObjectPath().ToString();
		Context.bLoop = bLoop;
		Context.StartedAtSeconds = FPlatformTime::Seconds();

		const TWeakPtr<FReEchoAudioUnrealBackend> WeakThis = AsShared();
		Component->OnAudioFinishedNative.AddLambda(
		    [WeakThis, Handle](UAudioComponent*)
		    {
			    if (const TSharedPtr<FReEchoAudioUnrealBackend> Backend = WeakThis.Pin())
			    {
				    Backend->CompletePlayback(Handle, EReEchoAudioPlaybackEndReason::NaturalFinish);
			    }
		    });
	}

	void CompletePlayback(const uint32 Handle, const EReEchoAudioPlaybackEndReason FallbackReason)
	{
		FActivePlaybackContext* Context = ActivePlaybackContexts.Find(Handle);
		if (Context == nullptr)
		{
			return;
		}
		const EReEchoAudioPlaybackEndReason Reason =
		    Context->PendingEndReason == EReEchoAudioPlaybackEndReason::NaturalFinish ? FallbackReason
		                                                                              : Context->PendingEndReason;
		const double ElapsedSeconds = FMath::Max(0.0, FPlatformTime::Seconds() - Context->StartedAtSeconds);
		UE_LOG(LogReEchoAudio,
		       Log,
		       TEXT("[PlaybackEnd] Handle=%u Kind=%s EventId=%s VariantId=%s Asset=%s Reason=%s "
		            "Elapsed=%.3f"),
		       Handle,
		       ToLogString(Context->bLoop),
		       *Context->EventId.ToString(),
		       *Context->VariantId.ToString(),
		       *Context->AssetPath,
		       ToLogString(Reason),
		       ElapsedSeconds);
		ActivePlaybackContexts.Remove(Handle);
		ActiveComponents.Remove(Handle);
		LoopComponents.Remove(Handle);
	}

	uint32 NextHandle = 1;
	TMap<uint32, TWeakObjectPtr<UAudioComponent>> ActiveComponents;
	TMap<uint32, TWeakObjectPtr<UAudioComponent>> LoopComponents;
	TMap<uint32, FActivePlaybackContext> ActivePlaybackContexts;
};

namespace ReEchoAudio
{
TSharedRef<IReEchoAudioBackend> CreateUnrealBackend()
{
	return MakeShared<FReEchoAudioUnrealBackend>();
}
}
