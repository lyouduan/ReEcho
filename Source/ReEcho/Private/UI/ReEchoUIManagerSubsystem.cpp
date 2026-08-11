#include "UI/ReEchoUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

namespace
{
constexpr int32 WeatherZOrder = 5;
constexpr int32 GameplayHudZOrder = 10;
constexpr int32 PlayerHudZOrder = 12;
constexpr int32 BuildChoiceZOrder = 90;
constexpr int32 ScreenZOrder = 95;
constexpr int32 PauseZOrder = 100;
constexpr int32 StartZOrder = 200;
constexpr int32 LoadoutZOrder = 210;
constexpr int32 SettingsZOrder = 220;
} // namespace

void UReEchoUIManagerSubsystem::AddToLayer(UUserWidget* Widget, const EReEchoUILayer Layer)
{
	if (!Widget)
	{
		return;
	}

	Widget->AddToViewport(GetLayerZOrder(Layer));
	ManagedWidgets.AddUnique(Widget);
}

void UReEchoUIManagerSubsystem::ConfigureMenuInput(APlayerController* PlayerController,
                                                   UUserWidget* Widget,
                                                   const bool bUIOnly) const
{
	if (!PlayerController || !Widget)
	{
		return;
	}

	if (bUIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
	PlayerController->SetShowMouseCursor(true);
}

void UReEchoUIManagerSubsystem::ConfigureGameplayInput(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}

void UReEchoUIManagerSubsystem::Deinitialize()
{
	for (UUserWidget* Widget : ManagedWidgets)
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
	}
	ManagedWidgets.Reset();
	Super::Deinitialize();
}

int32 UReEchoUIManagerSubsystem::GetLayerZOrder(const EReEchoUILayer Layer)
{
	switch (Layer)
	{
		case EReEchoUILayer::Weather:
			return WeatherZOrder;
		case EReEchoUILayer::GameplayHud:
			return GameplayHudZOrder;
		case EReEchoUILayer::PlayerHud:
			return PlayerHudZOrder;
		case EReEchoUILayer::BuildChoice:
			return BuildChoiceZOrder;
		case EReEchoUILayer::Screen:
			return ScreenZOrder;
		case EReEchoUILayer::Pause:
			return PauseZOrder;
		case EReEchoUILayer::Start:
			return StartZOrder;
		case EReEchoUILayer::Loadout:
			return LoadoutZOrder;
		case EReEchoUILayer::Settings:
			return SettingsZOrder;
		default:
			return GameplayHudZOrder;
	}
}
