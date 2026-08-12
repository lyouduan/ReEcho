#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ReEchoUIManagerSubsystem.h"

UUserWidget* UReEchoUIFlowCoordinatorSubsystem::OpenScreen(APlayerController* PlayerController,
                                                            const EReEchoUIScreen Screen,
                                                            const bool bUIOnly,
                                                            const bool bPauseWorld)
{
	UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>();
	UUserWidget* Widget = UIManager ? UIManager->CreateScreen(PlayerController, Screen) : nullptr;
	if (Widget && Screen != EReEchoUIScreen::Weather && Screen != EReEchoUIScreen::EncounterHud &&
	    Screen != EReEchoUIScreen::PlayerHud)
	{
		UIManager->ConfigureMenuInput(PlayerController, Widget, bUIOnly);
		if (bPauseWorld)
		{
			UGameplayStatics::SetGamePaused(this, true);
		}
	}
	return Widget;
}

void UReEchoUIFlowCoordinatorSubsystem::CloseScreen(const EReEchoUIScreen Screen)
{
	if (UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>())
	{
		UIManager->CloseScreen(Screen);
	}
}

UUserWidget* UReEchoUIFlowCoordinatorSubsystem::GetScreen(const EReEchoUIScreen Screen) const
{
	const UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>();
	return UIManager ? UIManager->GetScreen(Screen) : nullptr;
}

bool UReEchoUIFlowCoordinatorSubsystem::IsScreenOpen(const EReEchoUIScreen Screen) const
{
	return GetScreen(Screen) != nullptr;
}

void UReEchoUIFlowCoordinatorSubsystem::FocusScreen(APlayerController* PlayerController,
                                                     const EReEchoUIScreen Screen,
                                                     const bool bUIOnly) const
{
	UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>();
	if (UIManager)
	{
		UIManager->ConfigureMenuInput(PlayerController, UIManager->GetScreen(Screen), bUIOnly);
	}
}

void UReEchoUIFlowCoordinatorSubsystem::PreparePausedScreenTransition(const UObject* WorldContextObject) const
{
	UGameplayStatics::SetGamePaused(WorldContextObject, false);
}

void UReEchoUIFlowCoordinatorSubsystem::RestoreGameplay(const UObject* WorldContextObject,
                                                        APlayerController* PlayerController) const
{
	UGameplayStatics::SetGamePaused(WorldContextObject, false);
	if (UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>())
	{
		UIManager->ConfigureGameplayInput(PlayerController);
	}
}
