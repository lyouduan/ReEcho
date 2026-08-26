#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "ReEchoAudioTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReEchoAudioService.generated.h"

class FReEchoAudioCatalog;
class FReEchoAudioPolicyEngine;
class UReEchoAudioUserSettings;

/** GameInstance-lifetime semantic audio facade. */
UCLASS()

class REECHOAUDIO_API UReEchoAudioService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void PostEvent(UObject* WorldContextObject, const FReEchoAudioEventRequest& Request);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void PostEventById(UObject* WorldContextObject, FName EventId, const FVector& WorldLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void SetMusicState(FName StateId);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void SetMusicStateVariant(FName StateId, FName VariantId);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void SetAmbienceState(FName StateId);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void SetAmbienceStateVariant(FName StateId, FName VariantId);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void StopMusicState();
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void StopAmbienceState();
	/** Queue a one-shot until the current game world has been replaced. Never delays the world transition. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio") void QueueEventForNextWorld(FName EventId);

	/** Preview values immediately in the policy engine; these calls do not persist. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings") void SetMasterVolume(float Volume);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings")
	void SetBusVolume(EReEchoAudioBus Bus, float Volume);
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings") void SetBusMuted(EReEchoAudioBus Bus, bool bMuted);
	UFUNCTION(BlueprintPure, Category = "ReEchoAudio|Settings") float GetMasterVolume() const;
	UFUNCTION(BlueprintPure, Category = "ReEchoAudio|Settings") float GetBusVolume(EReEchoAudioBus Bus) const;
	UFUNCTION(BlueprintPure, Category = "ReEchoAudio|Settings") bool IsBusMuted(EReEchoAudioBus Bus) const;

	/** Commit current preview values to the module-owned SaveGame slot. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings") bool CommitUserSettings();
	/** Discard preview values and restore the last persisted values. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings") void RevertUserSettings();
	/** Preview defaults without persisting until CommitUserSettings. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Settings") void PreviewDefaultSettings();
	/** One-shot diagnostic action; never persisted. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio|Diagnostics")
	void PlayDiagnosticTone(UObject* WorldContextObject);

	FReEchoAudioPolicyEngine& GetPolicyEngine()
	{
		return *PolicyEngine;
	}

	const FReEchoAudioCatalog* GetCatalog() const
	{
		return Catalog.Get();
	}

private:
	bool TickAudio(float DeltaTime);
	void PrepareStateWorld(UWorld* ActiveWorld);
	void RetryDesiredStates(UWorld* ActiveWorld);
	void TryPostQueuedWorldEvent();
	void LoadAndApplyUserSettings();
	void ApplyPersistedUserSettings();

	TSharedPtr<FReEchoAudioPolicyEngine> PolicyEngine;
	TSharedPtr<FReEchoAudioCatalog> Catalog;
	FStreamableManager AudioStreamableManager;
	FName DesiredMusicStateId;
	FName DesiredMusicVariantId;
	FName DesiredAmbienceStateId;
	FName DesiredAmbienceVariantId;
	TWeakObjectPtr<UWorld> StateWorld;
	FName QueuedWorldEventId;
	TWeakObjectPtr<UWorld> QueuedWorldEventOrigin;
	UPROPERTY()
	TObjectPtr<UReEchoAudioUserSettings> UserSettings;
	FTSTicker::FDelegateHandle TickerHandle;
};
