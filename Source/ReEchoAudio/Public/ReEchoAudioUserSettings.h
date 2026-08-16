#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ReEchoAudioTypes.h"
#include "ReEchoAudioUserSettings.generated.h"

/**
 * Module-owned user audio preferences for Plan34.
 *
 * Deliberately NOT a UDeveloperSettings and NOT written into ReEcho config
 * sections: it is a USaveGame slot owned by the ReEchoAudio module, so the
 * preferences survive sessions without leaking into the ReEcho project config.
 * The ReEcho settings widget writes through UReEchoAudioService, which then
 * persists and applies these values to the policy engine.
 *
 * Defaults: master bus 1.0, the four content buses 0.8, nothing muted.
 * Diagnostic actions are deliberately not user preferences and are never saved.
 */
UCLASS()
class REECHOAUDIO_API UReEchoAudioUserSettings : public USaveGame
{
	GENERATED_BODY()

public:
	static const TCHAR* GetSlotName();
	static int32 GetSlotIndex();

	UReEchoAudioUserSettings();

	UPROPERTY()
	float MasterVolume = 1.0f;

	UPROPERTY()
	float MusicVolume = 0.8f;

	UPROPERTY()
	float AmbienceVolume = 0.8f;

	UPROPERTY()
	float CombatSfxVolume = 0.8f;

	UPROPERTY()
	float UiSfxVolume = 0.8f;

	UPROPERTY()
	bool bMasterMuted = false;

	UPROPERTY()
	bool bMusicMuted = false;

	UPROPERTY()
	bool bAmbienceMuted = false;

	UPROPERTY()
	bool bCombatSfxMuted = false;

	UPROPERTY()
	bool bUiSfxMuted = false;

	float GetBusVolume(EReEchoAudioBus Bus) const;
	void SetBusVolume(EReEchoAudioBus Bus, float Volume);
	bool IsBusMuted(EReEchoAudioBus Bus) const;
	void SetBusMuted(EReEchoAudioBus Bus, bool bMuted);

	/** Load the saved settings, or create a default instance if none exists. */
	static UReEchoAudioUserSettings* LoadOrCreate(UObject* Outer);
	/** Persist the current settings to the module-owned save slot. */
	bool Persist(UObject* Outer);
};
