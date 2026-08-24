#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Framework/ReEchoUIScreenTypes.h"
#include "UI/ReEchoUIManagerSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoUIManagerSubsystemResetOnTravelTest,
                                 "ReEcho.UIManagerSubsystem.ResetOnTravel",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoUIManagerSubsystemResetOnTravelTest::RunTest(const FString& Parameters)
{
	// Minimal world + player controller so CreateWidget can resolve an owner viewport context.
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ReEchoUIManagerResetTravelWorld"), GetTransientPackage());
	WorldContext.SetCurrentWorld(World);
	if (!TestNotNull(TEXT("Test world is created"), World))
	{
		GEngine->DestroyWorldContext(World);
		return false;
	}

	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Test player controller is spawned"), PlayerController))
	{
		GEngine->DestroyWorldContext(World);
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoUIManagerSubsystem* UIManager = NewObject<UReEchoUIManagerSubsystem>(GameInstance);
	if (!TestNotNull(TEXT("UI manager subsystem is instantiated"), UIManager))
	{
		GEngine->DestroyWorldContext(World);
		return false;
	}

	// CreateWidget requires a local player controller; bind a LocalPlayer so the spawned PC qualifies.
	// ULocalPlayer's ClassWithin is UEngine, so the outer must be GEngine rather than GameInstance.
	PlayerController->SetPlayer(NewObject<ULocalPlayer>(GEngine));

	// Open two distinct gameplay screens; CreateScreen adds them to ActiveScreens / ManagedWidgets.
	UUserWidget* PlayerHud = UIManager->CreateScreen(PlayerController, EReEchoUIScreen::PlayerHud);
	UUserWidget* EncounterHud = UIManager->CreateScreen(PlayerController, EReEchoUIScreen::EncounterHud);
	if (!TestNotNull(TEXT("Player HUD screen is created"), PlayerHud) ||
	    !TestNotNull(TEXT("Encounter HUD screen is created"), EncounterHud))
	{
		GEngine->DestroyWorldContext(World);
		return false;
	}

	// Before reset, re-opening a screen returns the cached instance (short-circuit path).
	TestTrue(TEXT("Re-opening a screen returns the cached instance before travel"),
	         UIManager->CreateScreen(PlayerController, EReEchoUIScreen::PlayerHud) == PlayerHud);

	// The friend access reaches the private ResetScreens used by the PreLoadMap travel handler.
	UIManager->ResetScreens();

	// After travel reset, the previously opened screens are gone from the manager and the old
	// widgets left the viewport.
	TestNull(TEXT("Player HUD cleared after travel reset"), UIManager->GetScreen(EReEchoUIScreen::PlayerHud));
	TestNull(TEXT("Encounter HUD cleared after travel reset"), UIManager->GetScreen(EReEchoUIScreen::EncounterHud));

	// Opening after reset yields a brand-new instance (rebuild + re-add-to-viewport path).
	UUserWidget* PlayerHudRebuilt = UIManager->CreateScreen(PlayerController, EReEchoUIScreen::PlayerHud);
	TestNotNull(TEXT("Player HUD rebuilt after travel reset"), PlayerHudRebuilt);
	TestTrue(TEXT("Rebuilt HUD is a distinct instance from before travel"), PlayerHudRebuilt != PlayerHud);

	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPauseOverlayLayerPolicyTest,
                                 "ReEcho.UIManagerSubsystem.PauseOverInventoryShop",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPauseOverlayLayerPolicyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Inventory/shop uses the regular screen layer"),
	          UReEchoUIManagerSubsystem::GetScreenLayer(EReEchoUIScreen::InventoryShop),
	          EReEchoUILayer::Screen);
	TestEqual(TEXT("Pause uses the dedicated pause layer"),
	          UReEchoUIManagerSubsystem::GetScreenLayer(EReEchoUIScreen::Restart),
	          EReEchoUILayer::Pause);
	TestTrue(TEXT("Pause renders above inventory/shop"),
	         UReEchoUIManagerSubsystem::GetLayerZOrder(EReEchoUILayer::Pause) >
	             UReEchoUIManagerSubsystem::GetLayerZOrder(EReEchoUILayer::Screen));
	return true;
}

#endif
