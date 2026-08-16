#include "ReEchoAudioService.h"

#include "Containers/Ticker.h"
#include "Misc/Paths.h"
#include "ReEchoAudioBackend.h"
#include "ReEchoAudioCatalog.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioPolicyEngine.h"
#include "ReEchoAudioUserSettings.h"

void UReEchoAudioService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PolicyEngine = MakeShared<FReEchoAudioPolicyEngine>();
	Catalog = MakeShared<FReEchoAudioCatalog>();
	PolicyEngine->SetCatalogProvider(Catalog);
	PolicyEngine->SetBackend(ReEchoAudio::CreateUnrealBackend());

	const FString CatalogCsv = FPaths::ProjectContentDir() / TEXT("Data/audio_events.csv");
	if (Catalog->LoadCatalog(CatalogCsv))
	{
		Catalog->PreloadSoftAssets(AudioStreamableManager);
	}
	LoadAndApplyUserSettings();
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UReEchoAudioService::TickAudio), 0.0f);
}

void UReEchoAudioService::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	if (Catalog.IsValid()) Catalog->CancelPreload();
	if (PolicyEngine.IsValid()) PolicyEngine->Shutdown();
	Catalog.Reset();
	PolicyEngine.Reset();
	Super::Deinitialize();
}

bool UReEchoAudioService::TickAudio(const float DeltaTime)
{
	if (PolicyEngine.IsValid()) PolicyEngine->Update(DeltaTime);
	return true;
}

void UReEchoAudioService::PostEvent(UObject* WorldContextObject, const FReEchoAudioEventRequest& Request)
{
	if (PolicyEngine.IsValid())
	{
		PolicyEngine->PostEvent(Request, WorldContextObject ? WorldContextObject->GetWorld() : nullptr);
	}
}

void UReEchoAudioService::PostEventById(UObject* WorldContextObject, const FName EventId, const FVector& WorldLocation)
{
	FReEchoAudioEventRequest Request;
	Request.EventId = EventId;
	Request.WorldLocation = WorldLocation;
	PostEvent(WorldContextObject, Request);
}

void UReEchoAudioService::SetMusicState(const FName StateId)
{
	if (PolicyEngine.IsValid()) PolicyEngine->SetState(EReEchoAudioChannel::Music, StateId, GetWorld());
}

void UReEchoAudioService::SetAmbienceState(const FName StateId)
{
	if (PolicyEngine.IsValid()) PolicyEngine->SetState(EReEchoAudioChannel::Ambience, StateId, GetWorld());
}

void UReEchoAudioService::StopMusicState()
{
	if (PolicyEngine.IsValid()) PolicyEngine->StopState(EReEchoAudioChannel::Music);
}

void UReEchoAudioService::StopAmbienceState()
{
	if (PolicyEngine.IsValid()) PolicyEngine->StopState(EReEchoAudioChannel::Ambience);
}

void UReEchoAudioService::SetMasterVolume(const float Volume)
{
	if (PolicyEngine.IsValid()) PolicyEngine->SetMasterVolume(FMath::Clamp(Volume, 0.0f, 1.0f));
}

void UReEchoAudioService::SetBusVolume(const EReEchoAudioBus Bus, const float Volume)
{
	if (Bus == EReEchoAudioBus::Master) { SetMasterVolume(Volume); return; }
	if (PolicyEngine.IsValid()) PolicyEngine->SetBusVolume(Bus, FMath::Clamp(Volume, 0.0f, 1.0f));
}

void UReEchoAudioService::SetBusMuted(const EReEchoAudioBus Bus, const bool bMuted)
{
	if (PolicyEngine.IsValid()) PolicyEngine->SetBusMuted(Bus, bMuted);
}

float UReEchoAudioService::GetMasterVolume() const
{
	return PolicyEngine.IsValid() ? PolicyEngine->GetMasterVolume() : 1.0f;
}

float UReEchoAudioService::GetBusVolume(const EReEchoAudioBus Bus) const
{
	return Bus == EReEchoAudioBus::Master ? GetMasterVolume()
		: (PolicyEngine.IsValid() ? PolicyEngine->GetBusVolume(Bus) : 1.0f);
}

bool UReEchoAudioService::IsBusMuted(const EReEchoAudioBus Bus) const
{
	return PolicyEngine.IsValid() && PolicyEngine->IsBusMuted(Bus);
}

bool UReEchoAudioService::CommitUserSettings()
{
	if (!UserSettings || !PolicyEngine.IsValid()) return false;
	UserSettings->MasterVolume = GetMasterVolume();
	for (const EReEchoAudioBus Bus : {EReEchoAudioBus::Music, EReEchoAudioBus::Ambience, EReEchoAudioBus::CombatSfx, EReEchoAudioBus::UiSfx})
	{
		UserSettings->SetBusVolume(Bus, GetBusVolume(Bus));
		UserSettings->SetBusMuted(Bus, IsBusMuted(Bus));
	}
	UserSettings->SetBusMuted(EReEchoAudioBus::Master, IsBusMuted(EReEchoAudioBus::Master));
	UserSettings->Persist(this);
	return true;
}

void UReEchoAudioService::RevertUserSettings()
{
	ApplyPersistedUserSettings();
}

void UReEchoAudioService::PreviewDefaultSettings()
{
	SetMasterVolume(1.0f);
	SetBusMuted(EReEchoAudioBus::Master, false);
	for (const EReEchoAudioBus Bus : {EReEchoAudioBus::Music, EReEchoAudioBus::Ambience, EReEchoAudioBus::CombatSfx, EReEchoAudioBus::UiSfx})
	{
		SetBusVolume(Bus, 0.8f);
		SetBusMuted(Bus, false);
	}
}

void UReEchoAudioService::PlayDiagnosticTone(UObject* WorldContextObject)
{
	PostEventById(WorldContextObject ? WorldContextObject : this, FReEchoAudioEvents::UiError);
}

void UReEchoAudioService::LoadAndApplyUserSettings()
{
	UserSettings = UReEchoAudioUserSettings::LoadOrCreate(this);
	ApplyPersistedUserSettings();
}

void UReEchoAudioService::ApplyPersistedUserSettings()
{
	if (!UserSettings || !PolicyEngine.IsValid()) return;
	SetMasterVolume(UserSettings->MasterVolume);
	SetBusMuted(EReEchoAudioBus::Master, UserSettings->IsBusMuted(EReEchoAudioBus::Master));
	for (const EReEchoAudioBus Bus : {EReEchoAudioBus::Music, EReEchoAudioBus::Ambience, EReEchoAudioBus::CombatSfx, EReEchoAudioBus::UiSfx})
	{
		SetBusVolume(Bus, UserSettings->GetBusVolume(Bus));
		SetBusMuted(Bus, UserSettings->IsBusMuted(Bus));
	}
}
