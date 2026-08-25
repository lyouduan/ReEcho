#include "UI/ReEchoUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/ReEchoEncounterHudWidget.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoLoadoutSelectionWidget.h"
#include "UI/ReEchoPlayerHudWidget.h"
#include "UI/ReEchoRestartWidget.h"
#include "UI/ReEchoSettingsWidget.h"
#include "UI/ReEchoAboutWidget.h"
#include "UI/ReEchoStartMenuWidget.h"
#include "UI/ReEchoStatsWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UI/ReEchoWeatherWidget.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace
{
constexpr int32 WeatherZOrder = 5;
constexpr int32 GameplayHudZOrder = 10;
constexpr int32 PlayerHudZOrder = 12;
constexpr int32 BuildChoiceZOrder = 98;
constexpr int32 ScreenZOrder = 95;
constexpr int32 PauseZOrder = 100;
constexpr int32 StartZOrder = 200;
constexpr int32 LoadoutZOrder = 210;
constexpr int32 SettingsZOrder = 220;
} // namespace

UReEchoUIManagerSubsystem::UReEchoUIManagerSubsystem()
{
	static ConstructorHelpers::FClassFinder<UReEchoPlayerHudWidget> PlayerHudClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoPlayerHud"));
	static ConstructorHelpers::FClassFinder<UReEchoEncounterHudWidget> EncounterHudClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoEncounterHud"));
	static ConstructorHelpers::FClassFinder<UReEchoStartMenuWidget> StartMenuClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoStartMenu"));
	static ConstructorHelpers::FClassFinder<UReEchoLoadoutSelectionWidget> LoadoutClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutSelection"));
	static ConstructorHelpers::FClassFinder<UReEchoSettingsWidget> SettingsClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoSettings"));
	static ConstructorHelpers::FClassFinder<UUserWidget> AboutClass(TEXT("/Game/ReEcho/UI/WBP_ReEchoAbout"));
	static ConstructorHelpers::FClassFinder<UReEchoRestartWidget> RestartClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart"));
	static ConstructorHelpers::FClassFinder<UReEchoTraitCardChoiceWidget> TraitClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoTraitCardChoice"));
	static ConstructorHelpers::FClassFinder<UReEchoInventoryShopWidget> InventoryShopClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoInventoryShopScreen"));
	static ConstructorHelpers::FClassFinder<UReEchoStatsWidget> StatsClass(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoStatsScreen"));

	ScreenClasses.Add(EReEchoUIScreen::Weather, UReEchoWeatherWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::EncounterHud,
	                  EncounterHudClass.Class ? EncounterHudClass.Class.Get()
	                                          : UReEchoEncounterHudWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::PlayerHud,
	                  PlayerHudClass.Class ? PlayerHudClass.Class.Get() : UReEchoPlayerHudWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::StartMenu,
	                  StartMenuClass.Class ? StartMenuClass.Class.Get() : UReEchoStartMenuWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::Loadout,
	                  LoadoutClass.Class ? LoadoutClass.Class.Get() : UReEchoLoadoutSelectionWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::Settings,
	                  SettingsClass.Class ? SettingsClass.Class.Get() : UReEchoSettingsWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::About,
	                  AboutClass.Class ? AboutClass.Class.Get() : UReEchoAboutWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::Restart,
	                  RestartClass.Class ? RestartClass.Class.Get() : UReEchoRestartWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::TraitChoice,
	                  TraitClass.Class ? TraitClass.Class.Get() : UReEchoTraitCardChoiceWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::InventoryShop,
	                  InventoryShopClass.Class ? InventoryShopClass.Class.Get()
	                                           : UReEchoInventoryShopWidget::StaticClass());
	ScreenClasses.Add(EReEchoUIScreen::Stats,
	                  StatsClass.Class ? StatsClass.Class.Get() : UReEchoStatsWidget::StaticClass());
}

UUserWidget* UReEchoUIManagerSubsystem::CreateScreen(APlayerController* PlayerController, const EReEchoUIScreen Screen)
{
	if (!PlayerController)
	{
		ReEchoUIInteractionAudit::Write(TEXT("SCREEN_OPEN_FAILED"),
		                                FString::Printf(TEXT("screen=%s reason=MissingPlayerController"),
		                                                *ReEchoUIInteractionAudit::ScreenName(Screen)));
		return nullptr;
	}
	if (UUserWidget* ExistingScreen = GetScreen(Screen))
	{
		const EReEchoUILayer Layer = GetScreenLayer(Screen);
		ReEchoUIInteractionAudit::Write(TEXT("SCREEN_OPEN_REUSED"),
		                                FString::Printf(TEXT("screen=%s widget=%s layer=%s zorder=%d"),
		                                                *ReEchoUIInteractionAudit::ScreenName(Screen),
		                                                *ExistingScreen->GetName(),
		                                                *ReEchoUIInteractionAudit::LayerName(Layer),
		                                                GetLayerZOrder(Layer)));
		return ExistingScreen;
	}

	const TSubclassOf<UUserWidget> ScreenClass = GetScreenClass(Screen);
	UUserWidget* Widget = ScreenClass ? CreateWidget<UUserWidget>(PlayerController, ScreenClass) : nullptr;
	if (!Widget)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to create registered UI screen %d."), static_cast<int32>(Screen));
		ReEchoUIInteractionAudit::Write(TEXT("SCREEN_OPEN_FAILED"),
		                                FString::Printf(TEXT("screen=%s reason=ClassOrCreateWidgetFailed class=%s"),
		                                                *ReEchoUIInteractionAudit::ScreenName(Screen),
		                                                ScreenClass ? *ScreenClass->GetName() : TEXT("None")));
		return nullptr;
	}

	const EReEchoUILayer Layer = GetScreenLayer(Screen);
	ActiveScreens.Add(Screen, Widget);
	AddToLayer(Widget, Layer);
	ReEchoUIInteractionAudit::Write(TEXT("SCREEN_OPEN_CREATED"),
	                                FString::Printf(TEXT("screen=%s widget=%s class=%s layer=%s zorder=%d"),
	                                                *ReEchoUIInteractionAudit::ScreenName(Screen),
	                                                *Widget->GetName(),
	                                                *Widget->GetClass()->GetName(),
	                                                *ReEchoUIInteractionAudit::LayerName(Layer),
	                                                GetLayerZOrder(Layer)));
	return Widget;
}

UUserWidget* UReEchoUIManagerSubsystem::GetScreen(const EReEchoUIScreen Screen) const
{
	const TObjectPtr<UUserWidget>* Widget = ActiveScreens.Find(Screen);
	return Widget && IsValid(Widget->Get()) ? Widget->Get() : nullptr;
}

bool UReEchoUIManagerSubsystem::IsScreenOpen(const EReEchoUIScreen Screen) const
{
	return GetScreen(Screen) != nullptr;
}

void UReEchoUIManagerSubsystem::CloseScreen(const EReEchoUIScreen Screen)
{
	if (TObjectPtr<UUserWidget>* Widget = ActiveScreens.Find(Screen))
	{
		const FString WidgetName = IsValid(Widget->Get()) ? Widget->Get()->GetName() : TEXT("Invalid");
		if (IsValid(Widget->Get()))
		{
			Widget->Get()->RemoveFromParent();
			ManagedWidgets.Remove(Widget->Get());
		}
		ActiveScreens.Remove(Screen);
		ReEchoUIInteractionAudit::Write(
		    TEXT("SCREEN_CLOSED"),
		    FString::Printf(TEXT("screen=%s widget=%s"), *ReEchoUIInteractionAudit::ScreenName(Screen), *WidgetName));
		return;
	}
	ReEchoUIInteractionAudit::Write(TEXT("SCREEN_CLOSE_MISSING"),
	                                FString::Printf(TEXT("screen=%s"), *ReEchoUIInteractionAudit::ScreenName(Screen)));
}

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

void UReEchoUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UReEchoUIManagerSubsystem::HandlePreLoadMap);
}

void UReEchoUIManagerSubsystem::Deinitialize()
{
	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
		PreLoadMapHandle.Reset();
	}
	ResetScreens();
	Super::Deinitialize();
}

void UReEchoUIManagerSubsystem::ResetScreens()
{
	ReEchoUIInteractionAudit::Write(TEXT("SCREEN_RESET_ALL"), FString::Printf(TEXT("count=%d"), ActiveScreens.Num()));
	for (UUserWidget* Widget : ManagedWidgets)
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
	}
	ManagedWidgets.Reset();
	ActiveScreens.Reset();
}

void UReEchoUIManagerSubsystem::HandlePreLoadMap(const FString& MapName)
{
	(void)MapName;
	ResetScreens();
}

EReEchoUILayer UReEchoUIManagerSubsystem::GetScreenLayer(const EReEchoUIScreen Screen)
{
	switch (Screen)
	{
		case EReEchoUIScreen::Weather:
			return EReEchoUILayer::Weather;
		case EReEchoUIScreen::EncounterHud:
			return EReEchoUILayer::GameplayHud;
		case EReEchoUIScreen::PlayerHud:
			return EReEchoUILayer::PlayerHud;
		case EReEchoUIScreen::TraitChoice:
			return EReEchoUILayer::BuildChoice;
		case EReEchoUIScreen::InventoryShop:
		case EReEchoUIScreen::Stats:
			return EReEchoUILayer::Screen;
		case EReEchoUIScreen::Restart:
			return EReEchoUILayer::Pause;
		case EReEchoUIScreen::StartMenu:
			return EReEchoUILayer::Start;
		case EReEchoUIScreen::Loadout:
			return EReEchoUILayer::Loadout;
		case EReEchoUIScreen::Settings:
		case EReEchoUIScreen::About:
			return EReEchoUILayer::Settings;
		default:
			return EReEchoUILayer::GameplayHud;
	}
}

TSubclassOf<UUserWidget> UReEchoUIManagerSubsystem::GetScreenClass(const EReEchoUIScreen Screen) const
{
	const TSubclassOf<UUserWidget>* ScreenClass = ScreenClasses.Find(Screen);
	return ScreenClass ? *ScreenClass : nullptr;
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
