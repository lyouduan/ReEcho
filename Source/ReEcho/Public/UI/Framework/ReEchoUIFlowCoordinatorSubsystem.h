#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Framework/ReEchoUIScreenTypes.h"
#include "ReEchoUIFlowCoordinatorSubsystem.generated.h"

class APlayerController;
class UUserWidget;

/** Coordinates screen lifecycle, focus and pause-safe replacement without owning gameplay decisions. */
UCLASS()
class REECHO_API UReEchoUIFlowCoordinatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UUserWidget* OpenScreen(APlayerController* PlayerController,
	                        EReEchoUIScreen Screen,
	                        bool bUIOnly,
	                        bool bPauseWorld);
	void CloseScreen(EReEchoUIScreen Screen);
	UUserWidget* GetScreen(EReEchoUIScreen Screen) const;
	bool IsScreenOpen(EReEchoUIScreen Screen) const;
	void FocusScreen(APlayerController* PlayerController, EReEchoUIScreen Screen, bool bUIOnly) const;
	void PreparePausedScreenTransition(const UObject* WorldContextObject) const;
	void RestoreGameplay(const UObject* WorldContextObject, APlayerController* PlayerController) const;
};
