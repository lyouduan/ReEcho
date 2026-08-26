#include "ReEchoAudioService.h"

#include "Containers/Ticker.h"
#include "Misc/Paths.h"
#include "ReEchoAudioBackend.h"
#include "ReEchoAudioCatalog.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioPolicyEngine.h"
#include "ReEchoAudioUserSettings.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	// UReEchoAudioService is a UGameInstanceSubsystem and therefore has no world
	// of its own. Music/ambience voices are spawned into the active game/PIE world,
	// so resolve it from the engine's world contexts instead of GetWorld() (which
	// returns nullptr here and silently disables every loop).
	UWorld* ResolveActiveWorld()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			UWorld* World = Ctx.World();
			if (World && (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game))
			{
				return World;
			}
		}
		return nullptr;
	}
}

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
	DesiredMusicStateId = NAME_None;
	DesiredMusicVariantId = NAME_None;
	DesiredAmbienceStateId = NAME_None;
	DesiredAmbienceVariantId = NAME_None;
	StateWorld.Reset();
	QueuedWorldEventId = NAME_None;
	QueuedWorldEventOrigin.Reset();
	if (Catalog.IsValid()) Catalog->CancelPreload();
	if (PolicyEngine.IsValid()) PolicyEngine->Shutdown();
	Catalog.Reset();
	PolicyEngine.Reset();
	Super::Deinitialize();
}

bool UReEchoAudioService::TickAudio(const float DeltaTime)
{
	if (PolicyEngine.IsValid()) PolicyEngine->Update(DeltaTime);
	UWorld* ActiveWorld = ResolveActiveWorld();
	PrepareStateWorld(ActiveWorld);
	RetryDesiredStates(ActiveWorld);
	TryPostQueuedWorldEvent();
	return true;
}

void UReEchoAudioService::PrepareStateWorld(UWorld* ActiveWorld)
{
	if (!PolicyEngine.IsValid() || !ActiveWorld || ActiveWorld == StateWorld.Get())
	{
		return;
	}
	PolicyEngine->StopState(EReEchoAudioChannel::Music);
	PolicyEngine->StopState(EReEchoAudioChannel::Ambience);
	StateWorld = ActiveWorld;
}

void UReEchoAudioService::RetryDesiredStates(UWorld* ActiveWorld)
{
	if (!PolicyEngine.IsValid() || !ActiveWorld)
	{
		return;
	}
	if (!DesiredMusicStateId.IsNone() &&
	    (PolicyEngine->GetCurrentState(EReEchoAudioChannel::Music) != DesiredMusicStateId ||
	     PolicyEngine->GetCurrentStateVariant(EReEchoAudioChannel::Music) != DesiredMusicVariantId))
	{
		PolicyEngine->SetState(
		    EReEchoAudioChannel::Music, DesiredMusicStateId, DesiredMusicVariantId, ActiveWorld);
	}
	if (!DesiredAmbienceStateId.IsNone() &&
	    (PolicyEngine->GetCurrentState(EReEchoAudioChannel::Ambience) != DesiredAmbienceStateId ||
	     PolicyEngine->GetCurrentStateVariant(EReEchoAudioChannel::Ambience) != DesiredAmbienceVariantId))
	{
		PolicyEngine->SetState(
		    EReEchoAudioChannel::Ambience, DesiredAmbienceStateId, DesiredAmbienceVariantId, ActiveWorld);
	}
}

void UReEchoAudioService::TryPostQueuedWorldEvent()
{
	if (QueuedWorldEventId.IsNone() || !PolicyEngine.IsValid())
	{
		return;
	}

	UWorld* ActiveWorld = ResolveActiveWorld();
	if (!ActiveWorld || ActiveWorld == QueuedWorldEventOrigin.Get())
	{
		return;
	}

	FReEchoAudioEventRequest Request;
	Request.EventId = QueuedWorldEventId;
	PolicyEngine->PostEvent(Request, ActiveWorld);
	QueuedWorldEventId = NAME_None;
	QueuedWorldEventOrigin.Reset();
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
	SetMusicStateVariant(StateId, NAME_None);
}

void UReEchoAudioService::SetMusicStateVariant(const FName StateId, const FName VariantId)
{
	if (StateId.IsNone())
	{
		StopMusicState();
		return;
	}
	DesiredMusicStateId = StateId;
	DesiredMusicVariantId = VariantId;
	UWorld* ActiveWorld = ResolveActiveWorld();
	PrepareStateWorld(ActiveWorld);
	if (PolicyEngine.IsValid()) PolicyEngine->SetState(EReEchoAudioChannel::Music, StateId, VariantId, ActiveWorld);
}

void UReEchoAudioService::SetAmbienceState(const FName StateId)

{
	SetAmbienceStateVariant(StateId, NAME_None);
}

void UReEchoAudioService::SetAmbienceStateVariant(const FName StateId, const FName VariantId)
{
	if (StateId.IsNone())
	{
		StopAmbienceState();
		return;
	}
	DesiredAmbienceStateId = StateId;
	DesiredAmbienceVariantId = VariantId;
	UWorld* ActiveWorld = ResolveActiveWorld();
	PrepareStateWorld(ActiveWorld);
	if (PolicyEngine.IsValid()) PolicyEngine->SetState(EReEchoAudioChannel::Ambience, StateId, VariantId, ActiveWorld);
}

void UReEchoAudioService::StopMusicState()
{
	DesiredMusicStateId = NAME_None;
	DesiredMusicVariantId = NAME_None;
	if (PolicyEngine.IsValid()) PolicyEngine->StopState(EReEchoAudioChannel::Music);
}

void UReEchoAudioService::StopAmbienceState()
{
	DesiredAmbienceStateId = NAME_None;
	DesiredAmbienceVariantId = NAME_None;
	if (PolicyEngine.IsValid()) PolicyEngine->StopState(EReEchoAudioChannel::Ambience);
}

void UReEchoAudioService::QueueEventForNextWorld(const FName EventId)
{
	if (EventId.IsNone())
	{
		return;
	}
	QueuedWorldEventId = EventId;
	QueuedWorldEventOrigin = ResolveActiveWorld();
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
	return UserSettings->Persist(this);
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
