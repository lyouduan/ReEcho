#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "ReEchoAudioTypes.h"
#include "ReEchoAudioService.generated.h"

class FReEchoAudioPolicyEngine;
class IReEchoAudioBackend;

/**
 * Public, gameplay-facing audio service.
 *
 * Owned by the GameInstance lifetime (a UGameInstanceSubsystem), so cleanup is
 * tied to the instance and never to a GameMode. Gameplay calls only the small
 * semantic API below; all strategy lives in FReEchoAudioPolicyEngine and all
 * real playback lives behind IReEchoAudioBackend.
 */
UCLASS()
class REECHOAUDIO_API UReEchoAudioService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- One-shot events ----

	/** Post a semantic audio event. Safe no-op for unknown/missing assets. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void PostEvent(UObject* WorldContextObject, const FReEchoAudioEventRequest& Request);

	/** Convenience overload posting by event id at an optional world location. */
	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void PostEventById(UObject* WorldContextObject, FName EventId, const FVector& WorldLocation = FVector::ZeroVector);

	// ---- Long-loop state channels (idempotent) ----

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void SetMusicState(FName StateId);

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void SetAmbienceState(FName StateId);

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void StopMusicState();

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void StopAmbienceState();

	// ---- Volume buses ----

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void SetBusVolume(EReEchoAudioBus Bus, float Volume);

	UFUNCTION(BlueprintCallable, Category = "ReEchoAudio")
	void SetBusMuted(EReEchoAudioBus Bus, bool bMuted);

	UFUNCTION(BlueprintPure, Category = "ReEchoAudio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "ReEchoAudio")
	float GetBusVolume(EReEchoAudioBus Bus) const;

	UFUNCTION(BlueprintPure, Category = "ReEchoAudio")
	bool IsBusMuted(EReEchoAudioBus Bus) const;

	// ---- Test/debug seam ----
	FReEchoAudioPolicyEngine& GetPolicyEngine() { return *PolicyEngine; }

private:
	bool TickAudio(float DeltaTime);

	TSharedPtr<FReEchoAudioPolicyEngine> PolicyEngine;
	FTSTicker::FDelegateHandle TickerHandle;
};
