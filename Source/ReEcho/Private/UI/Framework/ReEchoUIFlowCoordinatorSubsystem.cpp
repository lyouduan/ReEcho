#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "UI/ReEchoUIManagerSubsystem.h"

UUserWidget* UReEchoUIFlowCoordinatorSubsystem::OpenScreen(APlayerController* PlayerController,
                                                            const EReEchoUIScreen Screen,
                                                            const bool bUIOnly,
                                                            const bool bPauseWorld)
{
	UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>();
	UUserWidget* Widget = UIManager ? UIManager->CreateScreen(PlayerController, Screen) : nullptr;
	BindAudioFeedback(Widget);
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

void UReEchoUIFlowCoordinatorSubsystem::BindAudioFeedback(UUserWidget* Widget)
{
	if (!Widget || !Widget->WidgetTree)
	{
		return;
	}

	TArray<UWidget*> Widgets;
	Widget->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Child : Widgets)
	{
		if (UButton* Button = Cast<UButton>(Child))
		{
			Button->OnHovered.AddUniqueDynamic(this, &UReEchoUIFlowCoordinatorSubsystem::HandleButtonHovered);
			Button->OnClicked.AddUniqueDynamic(this, &UReEchoUIFlowCoordinatorSubsystem::HandleButtonClicked);
		}
	}
}

void UReEchoUIFlowCoordinatorSubsystem::PostUiEvent(const FName EventId) const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UReEchoAudioService* AudioService = GameInstance->GetSubsystem<UReEchoAudioService>())
		{
			AudioService->PostEventById(GameInstance, EventId);
		}
	}
}

void UReEchoUIFlowCoordinatorSubsystem::HandleButtonHovered()
{
	PostUiEvent(FReEchoAudioEvents::UiHover);
}

void UReEchoUIFlowCoordinatorSubsystem::HandleButtonClicked()
{
	PostUiEvent(FReEchoAudioEvents::UiConfirm);
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
