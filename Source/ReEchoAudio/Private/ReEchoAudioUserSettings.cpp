#include "ReEchoAudioUserSettings.h"

#include "Kismet/GameplayStatics.h"

const TCHAR* UReEchoAudioUserSettings::GetSlotName()
{
	return TEXT("ReEchoAudioUserSettings");
}

int32 UReEchoAudioUserSettings::GetSlotIndex()
{
	return 0;
}

UReEchoAudioUserSettings::UReEchoAudioUserSettings()
{
	SetFlags(RF_Transactional);
}

float UReEchoAudioUserSettings::GetBusVolume(EReEchoAudioBus Bus) const
{
	switch (Bus)
	{
		case EReEchoAudioBus::Master: return MasterVolume;
		case EReEchoAudioBus::Music: return MusicVolume;
		case EReEchoAudioBus::Ambience: return AmbienceVolume;
		case EReEchoAudioBus::CombatSfx: return CombatSfxVolume;
		case EReEchoAudioBus::UiSfx: return UiSfxVolume;
		default: return 1.0f;
	}
}

void UReEchoAudioUserSettings::SetBusVolume(EReEchoAudioBus Bus, float Volume)
{
	Volume = FMath::Clamp(Volume, 0.0f, 4.0f);
	switch (Bus)
	{
		case EReEchoAudioBus::Master: MasterVolume = Volume; break;
		case EReEchoAudioBus::Music: MusicVolume = Volume; break;
		case EReEchoAudioBus::Ambience: AmbienceVolume = Volume; break;
		case EReEchoAudioBus::CombatSfx: CombatSfxVolume = Volume; break;
		case EReEchoAudioBus::UiSfx: UiSfxVolume = Volume; break;
		default: break;
	}
}

bool UReEchoAudioUserSettings::IsBusMuted(EReEchoAudioBus Bus) const
{
	switch (Bus)
	{
		case EReEchoAudioBus::Master: return bMasterMuted;
		case EReEchoAudioBus::Music: return bMusicMuted;
		case EReEchoAudioBus::Ambience: return bAmbienceMuted;
		case EReEchoAudioBus::CombatSfx: return bCombatSfxMuted;
		case EReEchoAudioBus::UiSfx: return bUiSfxMuted;
		default: return false;
	}
}

void UReEchoAudioUserSettings::SetBusMuted(EReEchoAudioBus Bus, bool bMuted)
{
	switch (Bus)
	{
		case EReEchoAudioBus::Master: bMasterMuted = bMuted; break;
		case EReEchoAudioBus::Music: bMusicMuted = bMuted; break;
		case EReEchoAudioBus::Ambience: bAmbienceMuted = bMuted; break;
		case EReEchoAudioBus::CombatSfx: bCombatSfxMuted = bMuted; break;
		case EReEchoAudioBus::UiSfx: bUiSfxMuted = bMuted; break;
		default: break;
	}
}

UReEchoAudioUserSettings* UReEchoAudioUserSettings::LoadOrCreate(UObject* Outer)
{
	if (UGameplayStatics::DoesSaveGameExist(GetSlotName(), GetSlotIndex()))
	{
		if (UReEchoAudioUserSettings* Existing =
				Cast<UReEchoAudioUserSettings>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), GetSlotIndex())))
		{
			return Existing;
		}
	}
	return NewObject<UReEchoAudioUserSettings>(Outer);
}

bool UReEchoAudioUserSettings::Persist(UObject* Outer)
{
	return UGameplayStatics::SaveGameToSlot(this, GetSlotName(), GetSlotIndex());
}
