#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Framework/ReEchoUIScreenTypes.h"
#include "ReEchoUIFlowCoordinatorSubsystem.generated.h"

class APlayerController;
class UButton;
class UReEchoButtonAudioFeedback;
class UReEchoButtonVisualFeedback;
class UUserWidget;
class UWidget;

/** Coordinates screen lifecycle, focus and pause-safe replacement without owning gameplay decisions. */
UCLASS()

class REECHO_API UReEchoUIFlowCoordinatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UUserWidget*
	OpenScreen(APlayerController* PlayerController, EReEchoUIScreen Screen, bool bUIOnly, bool bPauseWorld);
	void CloseScreen(EReEchoUIScreen Screen);
	UUserWidget* GetScreen(EReEchoUIScreen Screen) const;
	bool IsScreenOpen(EReEchoUIScreen Screen) const;
	void FocusScreen(APlayerController* PlayerController, EReEchoUIScreen Screen, bool bUIOnly) const;
	void PreparePausedScreenTransition(const UObject* WorldContextObject) const;
	void RestoreGameplay(const UObject* WorldContextObject, APlayerController* PlayerController) const;
	/** Carries the terminal-restart destination across one world replacement. */
	void RequestLoadoutAfterWorldTravel();
	bool ConsumeLoadoutAfterWorldTravelRequest();
	/** Binds a runtime-created button to stable semantic events; Blueprint may pass NAME_None to disable one side. */
	UFUNCTION(BlueprintCallable, Category = "ReEcho|UI|Audio")
	void BindButtonAudioFeedback(UButton* Button, FName HoverEventId, FName ClickEventId);
	void PostUiEvent(FName EventId) const;
	static UWidget* ResolveButtonVisualRoot(UButton* Button);

private:
	void BindAudioFeedback(UUserWidget* Widget, EReEchoUIScreen Screen);
	void BindDefaultButtonAudioFeedback(UButton* Button);
	void BindButtonVisualFeedback(UButton* Button, EReEchoUIScreen Screen, const FString& WidgetName);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoButtonAudioFeedback>> ButtonAudioFeedbackBindings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoButtonVisualFeedback>> ButtonVisualFeedbackBindings;

	bool bLoadoutRequestedAfterWorldTravel = false;
};
