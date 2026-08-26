#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "UI/Framework/ReEchoButtonVisualFeedback.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "UI/ReEchoUIManagerSubsystem.h"

UUserWidget* UReEchoUIFlowCoordinatorSubsystem::OpenScreen(APlayerController* PlayerController,
                                                           const EReEchoUIScreen Screen,
                                                           const bool bUIOnly,
                                                           const bool bPauseWorld)
{
	UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>();
	UUserWidget* Widget = UIManager ? UIManager->CreateScreen(PlayerController, Screen) : nullptr;
	BindAudioFeedback(Widget, Screen);
	if (Widget && Screen != EReEchoUIScreen::Weather && Screen != EReEchoUIScreen::EncounterHud &&
	    Screen != EReEchoUIScreen::PlayerHud && Screen != EReEchoUIScreen::EncounterTransition)
	{
		UIManager->ConfigureMenuInput(PlayerController, Widget, bUIOnly);
		if (bPauseWorld)
		{
			UGameplayStatics::SetGamePaused(this, true);
		}
	}
	ReEchoUIInteractionAudit::Write(TEXT("SCREEN_OPEN_FLOW"),
	                                FString::Printf(TEXT("screen=%s result=%s widget=%s input=%s pause=%d"),
	                                                *ReEchoUIInteractionAudit::ScreenName(Screen),
	                                                Widget ? TEXT("Success") : TEXT("Failed"),
	                                                Widget ? *Widget->GetName() : TEXT("None"),
	                                                bUIOnly ? TEXT("UIOnly") : TEXT("GameAndUI"),
	                                                bPauseWorld ? 1 : 0));
	return Widget;
}

void UReEchoUIFlowCoordinatorSubsystem::BindAudioFeedback(UUserWidget* Widget, const EReEchoUIScreen Screen)
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
			BindButtonVisualFeedback(Button, Screen, Widget->GetName());
		}
	}
}

UWidget* UReEchoUIFlowCoordinatorSubsystem::ResolveButtonVisualRoot(UButton* Button)
{
	if (!Button)
	{
		return Button;
	}

	UOverlay* ParentOverlay = Cast<UOverlay>(Button->GetParent());
	if (ParentOverlay)
	{
		int32 DirectButtonCount = 0;
		for (int32 ChildIndex = 0; ChildIndex < ParentOverlay->GetChildrenCount(); ++ChildIndex)
		{
			DirectButtonCount += ParentOverlay->GetChildAt(ChildIndex)->IsA<UButton>() ? 1 : 0;
		}
		if (DirectButtonCount == 1 && ParentOverlay->GetChildrenCount() > 1)
		{
			return ParentOverlay;
		}
	}

	return Button;
}

void UReEchoUIFlowCoordinatorSubsystem::BindButtonVisualFeedback(UButton* Button,
                                                                 const EReEchoUIScreen Screen,
                                                                 const FString& WidgetName)
{
	if (!Button)
	{
		return;
	}

	ButtonVisualFeedbackBindings.RemoveAll(
	    [](const UReEchoButtonVisualFeedback* Binding)
	    {
		    return !Binding || !Binding->HasValidButton();
	    });

	for (const UReEchoButtonVisualFeedback* Binding : ButtonVisualFeedbackBindings)
	{
		if (Binding->IsBoundTo(Button))
		{
			return;
		}
	}

	UReEchoButtonVisualFeedback* Binding = NewObject<UReEchoButtonVisualFeedback>(this);
	Binding->Bind(Button, ResolveButtonVisualRoot(Button), ReEchoUIInteractionAudit::ScreenName(Screen), WidgetName);
	ButtonVisualFeedbackBindings.Add(Binding);
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
	ReEchoUIInteractionAudit::Write(TEXT("SCREEN_CLOSE_FLOW"),
	                                FString::Printf(TEXT("screen=%s"), *ReEchoUIInteractionAudit::ScreenName(Screen)));
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
		UUserWidget* Widget = UIManager->GetScreen(Screen);
		UIManager->ConfigureMenuInput(PlayerController, Widget, bUIOnly);
		ReEchoUIInteractionAudit::Write(TEXT("SCREEN_FOCUS"),
		                                FString::Printf(TEXT("screen=%s result=%s widget=%s input=%s"),
		                                                *ReEchoUIInteractionAudit::ScreenName(Screen),
		                                                Widget ? TEXT("Success") : TEXT("Missing"),
		                                                Widget ? *Widget->GetName() : TEXT("None"),
		                                                bUIOnly ? TEXT("UIOnly") : TEXT("GameAndUI")));
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
