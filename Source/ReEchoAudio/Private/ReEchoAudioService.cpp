#include "ReEchoAudioService.h"
#include "ReEchoAudioPolicyEngine.h"
#include "ReEchoAudioCatalog.h"
#include "ReEchoAudioBackend.h"
#include "Containers/Ticker.h"

void UReEchoAudioService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PolicyEngine = MakeShared<FReEchoAudioPolicyEngine>();
	// Default empty catalog; Plan34 swaps in a table-backed provider.
	PolicyEngine->SetCatalogProvider(MakeShared<FReEchoAudioCatalog>());
	// Real UE backend; degrades to no-op when assets/devices are absent.
	PolicyEngine->SetBackend(ReEchoAudio::CreateUnrealBackend());

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UReEchoAudioService::TickAudio), 0.0f);
}

void UReEchoAudioService::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->Shutdown();
	}
	PolicyEngine.Reset();

	Super::Deinitialize();
}

bool UReEchoAudioService::TickAudio(float DeltaTime)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->Update(DeltaTime);
	}
	return true;
}

void UReEchoAudioService::PostEvent(UObject* WorldContextObject, const FReEchoAudioEventRequest& Request)
{
	if (!PolicyEngine.IsValid())
	{
		return;
	}
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	PolicyEngine->PostEvent(Request, World);
}

void UReEchoAudioService::PostEventById(UObject* WorldContextObject, FName EventId, const FVector& WorldLocation)
{
	FReEchoAudioEventRequest Request;
	Request.EventId = EventId;
	Request.WorldLocation = WorldLocation;
	PostEvent(WorldContextObject, Request);
}

void UReEchoAudioService::SetMusicState(FName StateId)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->SetState(EReEchoAudioChannel::Music, StateId, GetWorld());
	}
}

void UReEchoAudioService::SetAmbienceState(FName StateId)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->SetState(EReEchoAudioChannel::Ambience, StateId, GetWorld());
	}
}

void UReEchoAudioService::StopMusicState()
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->StopState(EReEchoAudioChannel::Music);
	}
}

void UReEchoAudioService::StopAmbienceState()
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->StopState(EReEchoAudioChannel::Ambience);
	}
}

void UReEchoAudioService::SetMasterVolume(float Volume)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->SetMasterVolume(Volume);
	}
}

void UReEchoAudioService::SetBusVolume(EReEchoAudioBus Bus, float Volume)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->SetBusVolume(Bus, Volume);
	}
}

void UReEchoAudioService::SetBusMuted(EReEchoAudioBus Bus, bool bMuted)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->SetBusMuted(Bus, bMuted);
	}
}

float UReEchoAudioService::GetMasterVolume() const
{
	return PolicyEngine.IsValid() ? PolicyEngine->GetMasterVolume() : 1.0f;
}

float UReEchoAudioService::GetBusVolume(EReEchoAudioBus Bus) const
{
	return PolicyEngine.IsValid() ? PolicyEngine->GetBusVolume(Bus) : 1.0f;
}

bool UReEchoAudioService::IsBusMuted(EReEchoAudioBus Bus) const
{
	return PolicyEngine.IsValid() ? PolicyEngine->IsBusMuted(Bus) : false;
}
