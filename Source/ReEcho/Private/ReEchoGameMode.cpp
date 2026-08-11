#include "ReEchoGameMode.h"

#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BillboardComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoEncounterHudWidget.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoLoadoutSelectionWidget.h"
#include "UI/ReEchoPlayerHudWidget.h"
#include "UI/ReEchoRestartWidget.h"
#include "UI/ReEchoSettingsWidget.h"
#include "UI/ReEchoStartMenuWidget.h"
#include "UI/ReEchoStatsWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UI/ReEchoUIManagerSubsystem.h"
#include "UI/ReEchoWeatherWidget.h"
#include "UObject/ConstructorHelpers.h"

AReEchoGameMode::AReEchoGameMode()
{
	DefaultPawnClass = AReEchoPlayerPawn::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> ArenaBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/Scenes/ArenaGround3D.ArenaGround3D"));
	ArenaBackgroundTexture = ArenaBackgroundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArenaMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/M_ArenaBackground.M_ArenaBackground"));
	ArenaBackgroundMaterial = ArenaMaterialFinder.Object;
	static ConstructorHelpers::FClassFinder<UReEchoPlayerHudWidget> PlayerHudClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoPlayerHud"));
	PlayerHudWidgetClass = PlayerHudClassFinder.Class;
}

void AReEchoGameMode::AddWidgetToUILayer(UUserWidget* Widget, const EReEchoUILayer Layer) const
{
	if (UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>())
	{
		UIManager->AddToLayer(Widget, Layer);
	}
}

void AReEchoGameMode::PrintGMResult(const FString& Message, const bool bSuccess) const
{
	UE_LOG(LogTemp, Display, TEXT("[GM] %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
		    -1, 6.0f, bSuccess ? FColor::Green : FColor::Red, FString::Printf(TEXT("[GM] %s"), *Message));
	}
}

bool AReEchoGameMode::EnsureGMCommandAvailable() const
{
#if UE_BUILD_SHIPPING
	PrintGMResult(TEXT("GM commands are disabled in Shipping builds."), false);
	return false;
#else
	return true;
#endif
}

void AReEchoGameMode::GMHelp()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	PrintGMResult(
	    TEXT("GMStatus | GMHeal [amount, 0=full] | GMAddShards [amount] | GMWeather <Clear|Rain|Fog> | GMKillAll"));
}

void AReEchoGameMode::GMStatus()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const float Health = Player && Player->Combatant ? Player->Combatant->CurrentHealth : 0.0f;
	const float MaximumHealth = Player && Player->Combatant ? Player->Combatant->Stats.HpMax : 0.0f;
	PrintGMResult(FString::Printf(TEXT("Encounter=%d, HP=%.0f/%.0f, TimeShards=%d, Echoes=%d"),
	                              RunSubsystem ? RunSubsystem->EncounterIndex : 0,
	                              Health,
	                              MaximumHealth,
	                              RunSubsystem ? RunSubsystem->TimeShards : 0,
	                              Echoes.Num()));
}

void AReEchoGameMode::GMHeal(const float Amount)
{
	if (!EnsureGMCommandAvailable() || !Player || !Player->Combatant)
	{
		return;
	}
	UReEchoCombatantComponent* Combatant = Player->Combatant;
	if (!Combatant->IsAlive())
	{
		PrintGMResult(TEXT("Cannot heal a dead player; restart the encounter first."), false);
		return;
	}
	const float PreviousHealth = Combatant->CurrentHealth;
	Combatant->ApplyHealing(Amount <= 0.0f ? Combatant->Stats.HpMax : Amount);
	PrintGMResult(FString::Printf(
	    TEXT("Player HP %.0f -> %.0f/%.0f"), PreviousHealth, Combatant->CurrentHealth, Combatant->Stats.HpMax));
}

void AReEchoGameMode::GMAddShards(const int32 Amount)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		PrintGMResult(TEXT("Run subsystem is unavailable."), false);
		return;
	}
	const int64 UpdatedShards = static_cast<int64>(RunSubsystem->TimeShards) + static_cast<int64>(Amount);
	RunSubsystem->TimeShards = static_cast<int32>(FMath::Clamp<int64>(UpdatedShards, 0, MAX_int32));
	PrintGMResult(FString::Printf(TEXT("TimeShards=%d"), RunSubsystem->TimeShards));
}

void AReEchoGameMode::GMWeather(const FString& Scene)
{
	if (!EnsureGMCommandAvailable() || !WeatherWidget)
	{
		return;
	}
	EReEchoWeatherScene WeatherScene;
	if (Scene.Equals(TEXT("Clear"), ESearchCase::IgnoreCase) || Scene.Equals(TEXT("Off"), ESearchCase::IgnoreCase))
	{
		WeatherScene = EReEchoWeatherScene::Clear;
	}
	else if (Scene.Equals(TEXT("Rain"), ESearchCase::IgnoreCase))
	{
		WeatherScene = EReEchoWeatherScene::Rain;
	}
	else if (Scene.Equals(TEXT("Fog"), ESearchCase::IgnoreCase))
	{
		WeatherScene = EReEchoWeatherScene::Fog;
	}
	else
	{
		PrintGMResult(TEXT("Usage: GMWeather <Clear|Rain|Fog>"), false);
		return;
	}
	WeatherWidget->SetWeatherScene(WeatherScene);
	PrintGMResult(FString::Printf(TEXT("Weather=%s"), *Scene));
}

void AReEchoGameMode::GMKillAll()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	int32 KilledCount = 0;
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		AReEchoEnemyActor* Enemy = *EnemyIterator;
		const FVector DamageSource =
		    Enemy ? Enemy->GetActorLocation() - Enemy->GetActorForwardVector() * 100.0f : FVector::ZeroVector;
		for (int32 Attempt = 0; Enemy && Enemy->IsAlive() && Attempt < 32; ++Attempt)
		{
			Enemy->ReceiveGrayboxDamage(TNumericLimits<float>::Max(), DamageSource);
		}
		if (Enemy && !Enemy->IsAlive())
		{
			++KilledCount;
		}
	}
	PrintGMResult(
	    FString::Printf(TEXT("Killed %d enemies; normal encounter completion will run next tick."), KilledCount));
}

void AReEchoGameMode::StartPlay()
{
	Super::StartPlay();
	Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UReEchoBalanceSettings* BalanceSettings = GetDefault<UReEchoBalanceSettings>();
	const float SceneWorldHeight = FMath::Max(100.0f, BalanceSettings->ArenaSceneWorldHeight);
	const float SceneAspectRatio = ArenaBackgroundTexture ? static_cast<float>(ArenaBackgroundTexture->GetSizeX()) /
	                                                            FMath::Max(1, ArenaBackgroundTexture->GetSizeY())
	                                                      : 16.0f / 9.0f;
	const float CameraOrthoWidth = SceneWorldHeight * SceneAspectRatio;
	ArenaSceneWorldHeight = SceneWorldHeight;
	ArenaSceneWorldWidth = CameraOrthoWidth;
	if (Player)
	{
		Player->ConfigureArenaBounds(ArenaSceneWorldHeight * 0.5f, ArenaSceneWorldWidth * 0.5f);
	}
	FixedCamera = GetWorld()->SpawnActor<ACameraActor>(FVector(-700.0f, 0.0f, 900.0f), FRotator(-55.0f, 0.0f, 0.0f));
	if (FixedCamera)
	{
		UCameraComponent* FixedCameraComponent = FixedCamera->GetCameraComponent();
		FixedCameraComponent->SetProjectionMode(ECameraProjectionMode::Orthographic);
		FixedCameraComponent->SetOrthoWidth(CameraOrthoWidth);
		FixedCameraComponent->SetAspectRatio(SceneAspectRatio);
		FixedCameraComponent->SetConstraintAspectRatio(true);
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			PlayerController->SetViewTarget(FixedCamera);
		}
		if (Player && Player->Camera)
		{
			Player->Camera->Deactivate();
		}
	}
	CreateArena();
	Director = GetWorld()->SpawnActor<AReEchoEncounterDirector>();
	Director->OnFixedStep.AddDynamic(this, &AReEchoGameMode::HandleFixedStep);
	Director->OnEncounterEnded.AddDynamic(this, &AReEchoGameMode::HandleEncounterEnded);
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		WeatherWidget = CreateWidget<UReEchoWeatherWidget>(PlayerController, UReEchoWeatherWidget::StaticClass());
		if (WeatherWidget)
		{
			WeatherWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			WeatherWidget->SetFogRevealSources(Player, nullptr);
			AddWidgetToUILayer(WeatherWidget, EReEchoUILayer::Weather);
		}
		EncounterHudWidget =
		    CreateWidget<UReEchoEncounterHudWidget>(PlayerController, UReEchoEncounterHudWidget::StaticClass());
		if (EncounterHudWidget)
		{
			EncounterHudWidget->SetVisibility(ESlateVisibility::Collapsed);
			AddWidgetToUILayer(EncounterHudWidget, EReEchoUILayer::GameplayHud);
		}
	}
	if (Player)
	{
		Player->OnActiveSkill.AddDynamic(this, &AReEchoGameMode::HandlePlayerSkill);
		Player->Combatant->OnDeath.AddDynamic(this, &AReEchoGameMode::HandlePlayerDeath);
	}
	if (Player)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			TSubclassOf<UReEchoPlayerHudWidget> HudClass = PlayerHudWidgetClass;
			if (!HudClass)
			{
				HudClass = UReEchoPlayerHudWidget::StaticClass();
			}
			PlayerHudWidget = CreateWidget<UReEchoPlayerHudWidget>(PlayerController, HudClass);
			if (PlayerHudWidget)
			{
				PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CharacterSprite->Sprite);
				PlayerHudWidget->SetVisibility(ESlateVisibility::Collapsed);
				AddWidgetToUILayer(PlayerHudWidget, EReEchoUILayer::PlayerHud);
			}
		}
	}
	SetGameplayPresentationVisible(false);
	GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowStartMenu);
}

void AReEchoGameMode::ShowStartMenu()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!PlayerController || !RunSubsystem || StartMenuWidget)
	{
		return;
	}

	StartMenuWidget = CreateWidget<UReEchoStartMenuWidget>(PlayerController, UReEchoStartMenuWidget::StaticClass());
	if (!StartMenuWidget)
	{
		return;
	}
	const bool bHasSavedRun = RunSubsystem->HasSavedRun();
	StartMenuWidget->InitializeMenu(bHasSavedRun);
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[ReEchoStartFlow] Showing start menu. HasSavedRun=%s"),
	       bHasSavedRun ? TEXT("true") : TEXT("false"));
	StartMenuWidget->OnNewGameRequested.AddDynamic(this, &AReEchoGameMode::HandleNewGameRequested);
	StartMenuWidget->OnContinueGameRequested.AddDynamic(this, &AReEchoGameMode::HandleContinueGameRequested);
	StartMenuWidget->OnGameSettingRequested.AddDynamic(this, &AReEchoGameMode::HandleStartSettingsRequested);
	AddWidgetToUILayer(StartMenuWidget, EReEchoUILayer::Start);
	StartMenuWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(StartMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleNewGameRequested()
{
	UE_LOG(LogTemp, Display, TEXT("[ReEchoStartFlow] New game requested."));
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	RunSubsystem->DeleteSavedRun();
	ShowLoadoutSelection();
}

void AReEchoGameMode::HandleContinueGameRequested()
{
	UE_LOG(LogTemp, Display, TEXT("[ReEchoStartFlow] Continue requested."));
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !RunSubsystem->LoadSavedRun())
	{
		if (StartMenuWidget)
		{
			StartMenuWidget->InitializeMenu(false);
		}
		return;
	}
	BeginSelectedRun();
}

void AReEchoGameMode::HandleStartSettingsRequested()
{
	if (!StartMenuWidget || SettingsWidget)
	{
		return;
	}

	StartMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	ShowSettingsScreen(true);
	if (!SettingsWidget)
	{
		StartMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AReEchoGameMode::HandlePauseSettingsRequested()
{
	if (!RestartWidget || SettingsWidget)
	{
		return;
	}

	RestartWidget->SetVisibility(ESlateVisibility::Collapsed);
	ShowSettingsScreen(false);
	if (!SettingsWidget)
	{
		RestartWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AReEchoGameMode::HandleSettingsClosed()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (SettingsWidget)
	{
		SettingsWidget->RemoveFromParent();
		SettingsWidget = nullptr;
	}

	if (bSettingsReturnToStartMenu && StartMenuWidget)
	{
		StartMenuWidget->SetVisibility(ESlateVisibility::Visible);
		StartMenuWidget->SetKeyboardFocus();
		if (PlayerController)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(StartMenuWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
	}
	else if (RestartWidget)
	{
		RestartWidget->SetVisibility(ESlateVisibility::Visible);
		RestartWidget->SetKeyboardFocus();
		if (PlayerController)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(RestartWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
		}
	}
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
	}
	bSettingsReturnToStartMenu = false;
}

void AReEchoGameMode::ShowSettingsScreen(const bool bReturnToStartMenu)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || SettingsWidget)
	{
		return;
	}

	SettingsWidget = CreateWidget<UReEchoSettingsWidget>(PlayerController, UReEchoSettingsWidget::StaticClass());
	if (!SettingsWidget)
	{
		return;
	}

	bSettingsReturnToStartMenu = bReturnToStartMenu;
	SettingsWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleSettingsClosed);
	AddWidgetToUILayer(SettingsWidget, EReEchoUILayer::Settings);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(SettingsWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}

void AReEchoGameMode::ShowLoadoutSelection()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || LoadoutSelectionWidget)
	{
		return;
	}
	UReEchoLoadoutSelectionWidget* NewLoadoutSelectionWidget =
	    CreateWidget<UReEchoLoadoutSelectionWidget>(PlayerController, UReEchoLoadoutSelectionWidget::StaticClass());
	if (!NewLoadoutSelectionWidget)
	{
		return;
	}
	if (StartMenuWidget)
	{
		StartMenuWidget->RemoveFromParent();
		StartMenuWidget = nullptr;
	}

	LoadoutSelectionWidget = NewLoadoutSelectionWidget;
	LoadoutSelectionWidget->OnLoadoutConfirmed.AddDynamic(this, &AReEchoGameMode::HandleLoadoutConfirmed);
	AddWidgetToUILayer(LoadoutSelectionWidget, EReEchoUILayer::Loadout);
	LoadoutSelectionWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(LoadoutSelectionWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
	UE_LOG(LogTemp, Display, TEXT("[ReEchoStartFlow] Showing first-encounter loadout selection."));
}

void AReEchoGameMode::HandleLoadoutConfirmed(const FName CharacterId, const FName WeaponId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}

	RunSubsystem->StartRun(CharacterId, WeaponId);
	if (!RunSubsystem->SaveRun())
	{
		UE_LOG(LogTemp, Error, TEXT("[ReEchoStartFlow] Initial loadout save failed."));
		return;
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[ReEchoStartFlow] Loadout locked. Character=%s Weapon=%s"),
	       *CharacterId.ToString(),
	       *WeaponId.ToString());
	BeginSelectedRun();
}

void AReEchoGameMode::BeginSelectedRun()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	if (StartMenuWidget)
	{
		StartMenuWidget->RemoveFromParent();
		StartMenuWidget = nullptr;
	}
	if (LoadoutSelectionWidget)
	{
		LoadoutSelectionWidget->RemoveFromParent();
		LoadoutSelectionWidget = nullptr;
	}
	bAwaitingStartChoice = false;
	SetGameplayPresentationVisible(true);
	if (Player)
	{
		Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
	}
	RestoreGameInput();
	if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice || RunSubsystem->Phase == EReEchoRunPhase::ForgeChoice)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
	}
	else if (RunSubsystem->Phase == EReEchoRunPhase::Encounter && RunSubsystem->HasPendingEncounterResume())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ResumeSavedEncounter);
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
	}
}

void AReEchoGameMode::SetGameplayPresentationVisible(const bool bVisible)
{
	const ESlateVisibility PassiveVisibility =
	    bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (WeatherWidget)
	{
		WeatherWidget->SetVisibility(PassiveVisibility);
	}
	if (EncounterHudWidget)
	{
		EncounterHudWidget->SetVisibility(PassiveVisibility);
	}
	if (PlayerHudWidget)
	{
		PlayerHudWidget->SetVisibility(PassiveVisibility);
	}
	if (Player)
	{
		Player->SetActorHiddenInGame(!bVisible);
		Player->SetActorEnableCollision(bVisible);
	}
}

void AReEchoGameMode::UpdateWeatherScene(const int32 EncounterIndex)
{
	if (!WeatherWidget)
	{
		return;
	}

	const UReEchoBalanceSettings* BalanceSettings = GetDefault<UReEchoBalanceSettings>();
	EReEchoWeatherScene WeatherScene = EReEchoWeatherScene::Clear;
	if (BalanceSettings->RainEncounterIndices.Contains(EncounterIndex))
	{
		WeatherScene = EReEchoWeatherScene::Rain;
	}
	else if (BalanceSettings->FogEncounterIndices.Contains(EncounterIndex))
	{
		WeatherScene = EReEchoWeatherScene::Fog;
	}
	WeatherWidget->SetWeatherScene(WeatherScene);
}

void AReEchoGameMode::CreateArena()
{
	const float ArenaHalfExtentX = ArenaSceneWorldHeight * 0.5f;
	const float ArenaHalfExtentY = ArenaSceneWorldWidth * 0.5f;
	const float BlockScaleX = ArenaHalfExtentX / 100.0f;
	const float BlockScaleY = ArenaHalfExtentY / 100.0f;
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto SpawnCollisionBlock = [&](const FVector& Location, const FVector& Scale, const TCHAR* Name)
	{
		AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
#if WITH_EDITOR
		StaticMeshActor->SetActorLabel(Name);
#endif
		StaticMeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		StaticMeshActor->SetActorScale3D(Scale);
		StaticMeshActor->SetActorHiddenInGame(true);
		StaticMeshActor->GetStaticMeshComponent()->SetVisibility(false, true);
	};
	SpawnCollisionBlock(FVector(0, 0, -55), FVector(BlockScaleX, BlockScaleY, 0.5f), TEXT("ArenaFloor"));
	SpawnCollisionBlock(FVector(0, ArenaHalfExtentY, 100), FVector(BlockScaleX, 0.25f, 2.f), TEXT("WallNorth"));
	SpawnCollisionBlock(FVector(0, -ArenaHalfExtentY, 100), FVector(BlockScaleX, 0.25f, 2.f), TEXT("WallSouth"));
	SpawnCollisionBlock(FVector(ArenaHalfExtentX, 0, 100), FVector(0.25f, BlockScaleY, 2.f), TEXT("WallEast"));
	SpawnCollisionBlock(FVector(-ArenaHalfExtentX, 0, 100), FVector(0.25f, BlockScaleY, 2.f), TEXT("WallWest"));
	if (ArenaBackgroundTexture && ArenaBackgroundMaterial)
	{
		const FVector CameraLocation = FixedCamera ? FixedCamera->GetActorLocation() : FVector(-700.0f, 0.0f, 900.0f);
		const FVector CameraForward =
		    FixedCamera ? FixedCamera->GetActorForwardVector() : FVector(0.5736f, 0.0f, -0.8192f);
		constexpr float BackdropDistance = 3000.0f;
		const float BackdropWorldHeight =
		    FMath::Max(100.0f, GetDefault<UReEchoBalanceSettings>()->ArenaSceneWorldHeight);
		const float TextureAspect =
		    static_cast<float>(ArenaBackgroundTexture->GetSizeX()) / FMath::Max(1, ArenaBackgroundTexture->GetSizeY());
		const float BackdropWorldWidth = BackdropWorldHeight * TextureAspect;
		const FVector BackdropLocation = CameraLocation + CameraForward * BackdropDistance;
		const FVector CameraRight = FixedCamera ? FixedCamera->GetActorRightVector() : FVector::RightVector;
		const FRotator BackdropRotation = FRotationMatrix::MakeFromZX(-CameraForward, CameraRight).Rotator();
		AStaticMeshActor* Backdrop = GetWorld()->SpawnActor<AStaticMeshActor>(BackdropLocation, BackdropRotation);
#if WITH_EDITOR
		Backdrop->SetActorLabel(TEXT("ArenaSkyBackdrop"));
#endif
		UStaticMeshComponent* BackdropMesh = Backdrop->GetStaticMeshComponent();
		BackdropMesh->SetMobility(EComponentMobility::Movable);
		BackdropMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		BackdropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BackdropMesh->SetCastShadow(false);
		BackdropMesh->SetTranslucentSortPriority(-100);
		Backdrop->SetActorScale3D(FVector(BackdropWorldWidth / 100.0f, BackdropWorldHeight / 100.0f, 1.0f));
		BackdropMesh->SetMaterial(0, ArenaBackgroundMaterial);
	}
}

void AReEchoGameMode::ClearCombatants()
{
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		EnemyIterator->Destroy();
	}
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->Destroy();
		}
	}
	Echoes.Reset();
	if (WeatherWidget)
	{
		WeatherWidget->SetFogRevealSources(Player, nullptr);
	}
}

void AReEchoGameMode::BeginNextEncounter()
{
	ClearCombatants();
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->EncounterIndex >= GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount())
	{
		return;
	}
	bEncounterTransitioning = false;
	bEncounterClearedByDefeat = false;
	RunSubsystem->BeginEncounter();
	UpdateWeatherScene(RunSubsystem->EncounterIndex);
	if (Player)
	{
		Player->SetActorLocation(FVector(0, 0, 112));
		Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
		Player->RestoreEquippedWeapon(RunSubsystem->CurrentBuild.WeaponId);
		const FReEchoStatBlock& Stats = RunSubsystem->CurrentBuild.Stats;
		Player->Combatant->InitializeFromStats(Stats, true);
		Player->Movement->MaxSpeed = 420.0f * Stats.MovementSpeed;
		if (PlayerHudWidget)
		{
			PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CharacterSprite->Sprite);
		}
		Player->Recorder->BeginRecording(RunSubsystem->EncounterIndex,
		                                 TEXT("GrayboxArena"),
		                                 1337 + RunSubsystem->EncounterIndex,
		                                 RunSubsystem->CurrentBuild);
	}
	const TArray<FReEchoRecording> Recordings = RunSubsystem->GetEchoRecordings(1);
	if (!Recordings.IsEmpty())
	{
		AReEchoEchoActor* Echo = GetWorld()->SpawnActor<AReEchoEchoActor>();
		if (Echo && Echo->InitializeEcho(Recordings[0],
		                                 RunSubsystem->CurrentBuild.Stats.EchoEfficiency,
		                                 RunSubsystem->GetRunDataSnapshot()))
		{
			Echoes.Add(Echo);
		}
		else if (Echo)
		{
			Echo->Destroy();
		}
	}
	if (WeatherWidget)
	{
		WeatherWidget->SetFogRevealSources(Player, Echoes.IsEmpty() ? nullptr : Echoes[0].Get());
	}
	SpawnEnemies(RunSubsystem->EncounterIndex);
	Director->StartEncounter();
}

FReEchoEncounterRuntimeState AReEchoGameMode::CaptureEncounterRuntimeState() const
{
	FReEchoEncounterRuntimeState Result;
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->Phase != EReEchoRunPhase::Encounter || !Director || !Player ||
	    !Player->Combatant || !Player->Combatant->IsAlive() || !Player->Recorder->IsRecording())
	{
		return Result;
	}

	Result.bValid = true;
	Result.EncounterTime = Director->EncounterTime;
	Result.PlayerTransform = Player->GetActorTransform();
	Result.PlayerHealth = Player->Combatant->CurrentHealth;
	Result.PlayerStats = Player->Combatant->Stats;
	Result.PlayerVelocity = Player->GetVelocity();
	Result.ActiveRecording = Player->Recorder->GetRecording();
	for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
	{
		if (EnemyIterator->IsAlive())
		{
			Result.Enemies.Add(EnemyIterator->CaptureRuntimeState());
		}
	}
	Result.Enemies.Sort(
	    [](const FReEchoEnemyRuntimeState& Left, const FReEchoEnemyRuntimeState& Right)
	    {
		    return Left.SpawnIndex < Right.SpawnIndex;
	    });
	return Result;
}

void AReEchoGameMode::ResumeSavedEncounter()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !RunSubsystem->HasPendingEncounterResume() || !Player || !Director)
	{
		return;
	}

	const FReEchoEncounterRuntimeState SavedState = RunSubsystem->ConsumePendingEncounterResume();
	ClearCombatants();
	bEncounterTransitioning = false;
	bEncounterClearedByDefeat = false;
	UpdateWeatherScene(RunSubsystem->EncounterIndex);

	Player->SetActorTransform(SavedState.PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
	Player->RestoreEquippedWeapon(RunSubsystem->CurrentBuild.WeaponId);
	Player->Combatant->InitializeFromStats(SavedState.PlayerStats, true);
	Player->Combatant->RestoreCurrentHealth(SavedState.PlayerHealth);
	Player->Movement->MaxSpeed = 420.0f * SavedState.PlayerStats.MovementSpeed;
	Player->Movement->Velocity = SavedState.PlayerVelocity;
	Player->Recorder->ResumeRecording(SavedState.ActiveRecording);
	if (PlayerHudWidget)
	{
		PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CharacterSprite->Sprite);
	}

	const TArray<FReEchoRecording> Recordings = RunSubsystem->GetEchoRecordings(1);
	if (!Recordings.IsEmpty())
	{
		AReEchoEchoActor* Echo = GetWorld()->SpawnActor<AReEchoEchoActor>();
		if (Echo)
		{
			if (Echo->InitializeEcho(
			        Recordings[0], RunSubsystem->CurrentBuild.Stats.EchoEfficiency, RunSubsystem->GetRunDataSnapshot()))
			{
				Echo->AdvanceEcho(SavedState.EncounterTime);
				Echoes.Add(Echo);
			}
			else
			{
				Echo->Destroy();
			}
		}
	}
	if (WeatherWidget)
	{
		WeatherWidget->SetFogRevealSources(Player, Echoes.IsEmpty() ? nullptr : Echoes[0].Get());
	}

	for (const FReEchoEnemyRuntimeState& EnemyState : SavedState.Enemies)
	{
		AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>();
		if (Enemy)
		{
			Enemy->RestoreRuntimeState(EnemyState);
		}
	}
	Director->ResumeEncounter(SavedState.EncounterTime);
}

void AReEchoGameMode::SpawnEnemies(const int32 EncounterIndex)
{
	const UReEchoBalanceSettings* BalanceSettings = GetDefault<UReEchoBalanceSettings>();
	const bool bBossEncounter = EncounterIndex == BalanceSettings->GetTotalEncounterCount();
	const int32 GruntCount =
	    bBossEncounter ? FMath::Min(8, BalanceSettings->MaxGruntCount)
	                   : FMath::Clamp(BalanceSettings->BaseGruntCount +
	                                      FMath::Max(0, EncounterIndex - 1) * BalanceSettings->GruntsPerEncounter,
	                                  1,
	                                  BalanceSettings->MaxGruntCount);
	const int32 BomberCount = bBossEncounter        ? FMath::Min(2, BalanceSettings->MaxBomberCount)
	                          : EncounterIndex >= 2 ? FMath::Min(EncounterIndex, BalanceSettings->MaxBomberCount)
	                                                : 0;
	const float SpawnHalfX = FMath::Max(100.0f, ArenaSceneWorldHeight * 0.5f - BalanceSettings->EnemySpawnEdgeInset);
	const float SpawnHalfY = FMath::Max(100.0f, ArenaSceneWorldWidth * 0.5f - BalanceSettings->EnemySpawnEdgeInset);
	FRandomStream SpawnRandom(1337 + EncounterIndex * 7919);
	int32 SpawnIndex = 0;

	auto GetPeripheralSpawnLocation = [&]()
	{
		const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
		const float MinimumDistance =
		    FMath::Min(BalanceSettings->EnemySpawnMinPlayerDistance, BalanceSettings->EnemySpawnMaxPlayerDistance);
		const float MaximumDistance =
		    FMath::Max(BalanceSettings->EnemySpawnMinPlayerDistance, BalanceSettings->EnemySpawnMaxPlayerDistance);
		const float Angle = SpawnRandom.FRandRange(0.0f, 2.0f * PI);
		const float Distance =
		    FMath::Sqrt(SpawnRandom.FRandRange(MinimumDistance * MinimumDistance, MaximumDistance * MaximumDistance));
		FVector SpawnLocation = PlayerLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
		SpawnLocation.X = FMath::Clamp(SpawnLocation.X, -SpawnHalfX, SpawnHalfX);
		SpawnLocation.Y = FMath::Clamp(SpawnLocation.Y, -SpawnHalfY, SpawnHalfY);
		SpawnLocation.Z = 50.0f;
		return SpawnLocation;
	};

	auto SpawnEnemy = [&](const EReEchoEnemyKind Kind)
	{
		AReEchoEnemyActor* Enemy =
		    GetWorld()->SpawnActor<AReEchoEnemyActor>(GetPeripheralSpawnLocation(), FRotator::ZeroRotator);
		if (Enemy)
		{
			Enemy->Configure(Kind, ++SpawnIndex);
		}
	};

	if (bBossEncounter)
	{
		SpawnEnemy(EReEchoEnemyKind::Boss);
	}
	for (int32 EnemyIndex = 0; EnemyIndex < GruntCount; ++EnemyIndex)
	{
		SpawnEnemy(EReEchoEnemyKind::Grunt);
	}
	for (int32 EnemyIndex = 0; EnemyIndex < BomberCount; ++EnemyIndex)
	{
		SpawnEnemy(EReEchoEnemyKind::Bomber);
	}
}

void AReEchoGameMode::HandleFixedStep(float)
{
	if (!Player || !Director)
	{
		return;
	}
	Player->Recorder->AdvanceRecording(Director->EncounterTime, Player->GetActorLocation());
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->AdvanceEcho(Director->EncounterTime);
		}
	}
}

void AReEchoGameMode::HandlePlayerSkill(FVector Position, FName SkillId)
{
	if (Player && Director)
	{
		Player->Recorder->RecordSkill(Director->EncounterTime, Position, SkillId);
	}
}

void AReEchoGameMode::HandlePlayerDeath()
{
	if (Director)
	{
		Director->EndEncounter();
	}

	ShowRestartScreen();
}

void AReEchoGameMode::ShowRestartScreen(const bool bDeathScreen, const bool bVictoryScreen)
{
	if (RestartWidget)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	RestartWidget = CreateWidget<UReEchoRestartWidget>(PlayerController, UReEchoRestartWidget::StaticClass());
	if (!RestartWidget)
	{
		return;
	}

	bRestartScreenIsTerminal = bDeathScreen || bVictoryScreen;
	if (bVictoryScreen)
	{
		const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
		RestartWidget->SetVictoryScreen(RunSubsystem ? RunSubsystem->TimeShards : 0,
		                                RunSubsystem ? RunSubsystem->CurrentBuild.Cards.Num() : 0);
	}
	else
	{
		RestartWidget->SetDeathScreen(bDeathScreen);
	}
	RestartWidget->OnRestartRequested.AddDynamic(this, &AReEchoGameMode::HandleRestartRequested);
	RestartWidget->OnResumeRequested.AddDynamic(this, &AReEchoGameMode::HandleResumeRequested);
	RestartWidget->OnQuitRequested.AddDynamic(this, &AReEchoGameMode::HandleQuitRequested);
	RestartWidget->OnSettingsRequested.AddDynamic(this, &AReEchoGameMode::HandlePauseSettingsRequested);
	AddWidgetToUILayer(RestartWidget, EReEchoUILayer::Pause);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(RestartWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::TogglePauseMenu()
{
	if (SettingsWidget)
	{
		HandleSettingsClosed();
		return;
	}
	if (bAwaitingStartChoice)
	{
		return;
	}
	if (StatsWidget)
	{
		HandleStatsClosed();
		return;
	}
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	if (TraitCardChoiceWidget || bRestartScreenIsTerminal)
	{
		return;
	}
	if (RestartWidget)
	{
		HandleResumeRequested();
		return;
	}
	ShowRestartScreen(false);
}

void AReEchoGameMode::ToggleStatsMenu()
{
	if (StatsWidget)
	{
		HandleStatsClosed();
		return;
	}
	ShowStatsMenu();
}

void AReEchoGameMode::ShowStatsMenu()
{
	if (InventoryShopWidget || TraitCardChoiceWidget || RestartWidget || bRestartScreenIsTerminal || !Player)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	StatsWidget = CreateWidget<UReEchoStatsWidget>(PlayerController, UReEchoStatsWidget::StaticClass());
	if (!StatsWidget)
	{
		return;
	}

	FReEchoStatBlock EchoStats;
	float EchoHealth = 0.0f;
	bool bHasEcho = false;
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			EchoStats = Echo->GetCurrentStats();
			EchoHealth = Echo->GetCurrentHealth();
			bHasEcho = true;
			break;
		}
	}

	StatsWidget->InitializeStats(
	    Player->Combatant->Stats, Player->Combatant->CurrentHealth, EchoStats, EchoHealth, bHasEcho);
	StatsWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleStatsClosed);
	AddWidgetToUILayer(StatsWidget, EReEchoUILayer::Screen);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(StatsWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleStatsClosed()
{
	if (StatsWidget)
	{
		StatsWidget->RemoveFromParent();
		StatsWidget = nullptr;
	}
	RestoreGameInput();
}

void AReEchoGameMode::ToggleInventoryMenu()
{
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	ShowInventoryShopMenu(false);
}

void AReEchoGameMode::ToggleShopMenu()
{
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	ShowInventoryShopMenu(true);
}

void AReEchoGameMode::ShowInventoryShopMenu(const bool bShowShop)
{
	if (StatsWidget || TraitCardChoiceWidget || RestartWidget || bRestartScreenIsTerminal)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!PlayerController || !RunSubsystem)
	{
		return;
	}

	InventoryShopWidget =
	    CreateWidget<UReEchoInventoryShopWidget>(PlayerController, UReEchoInventoryShopWidget::StaticClass());
	if (!InventoryShopWidget)
	{
		return;
	}

	InventoryShopWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleInventoryShopClosed);
	InventoryShopWidget->OnPurchaseRequested.AddDynamic(this, &AReEchoGameMode::HandleShopPurchaseRequested);
	if (bShowShop)
	{
		InventoryShopWidget->ShowShop(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
	}
	else
	{
		InventoryShopWidget->ShowInventory(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
	}
	AddWidgetToUILayer(InventoryShopWidget, EReEchoUILayer::Screen);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryShopWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleInventoryShopClosed()
{
	const bool bShouldStartNextEncounter = bContinueRunAfterShop;
	bContinueRunAfterShop = false;
	if (InventoryShopWidget)
	{
		InventoryShopWidget->RemoveFromParent();
		InventoryShopWidget = nullptr;
	}
	RestoreGameInput();
	if (bShouldStartNextEncounter)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
	}
}

void AReEchoGameMode::HandleShopPurchaseRequested(const FName ItemId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (RunSubsystem && InventoryShopWidget && RunSubsystem->PurchaseShopItem(ItemId))
	{
		RunSubsystem->SaveRun();
		InventoryShopWidget->ShowShop(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
	}
}

void AReEchoGameMode::HandleResumeRequested()
{
	bQuitConfirmationVisible = false;
	if (RestartWidget)
	{
		RestartWidget->RemoveFromParent();
		RestartWidget = nullptr;
	}
	bRestartScreenIsTerminal = false;
	RestoreGameInput();
}

void AReEchoGameMode::HandleQuitRequested()
{
	if (!bRestartScreenIsTerminal && !bQuitConfirmationVisible)
	{
		UE_LOG(LogTemp, Display, TEXT("[ReEchoStartFlow] Showing save-and-quit confirmation."));
		bQuitConfirmationVisible = true;
		if (RestartWidget)
		{
			RestartWidget->SetQuitConfirmation(true);
		}
		return;
	}

	if (!bRestartScreenIsTerminal)
	{
		UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
		const FReEchoEncounterRuntimeState EncounterState = CaptureEncounterRuntimeState();
		bool bSaved = false;
		if (RunSubsystem)
		{
			bSaved = RunSubsystem->Phase == EReEchoRunPhase::Encounter
			             ? EncounterState.bValid && RunSubsystem->SaveRun(&EncounterState)
			             : RunSubsystem->SaveRun();
		}
		if (!bSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("[ReEchoStartFlow] Save-and-quit aborted because persistence failed."));
			if (RestartWidget)
			{
				RestartWidget->ShowSaveFailure();
			}
			return;
		}
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[ReEchoStartFlow] Save-and-quit succeeded. EncounterSnapshot=%s"),
		       EncounterState.bValid ? TEXT("true") : TEXT("false"));
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
}

void AReEchoGameMode::HandleRestartRequested()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (RestartWidget)
	{
		RestartWidget->RemoveFromParent();
		RestartWidget = nullptr;
	}
	bRestartScreenIsTerminal = false;

	UGameplayStatics::SetGamePaused(this, false);
	if (PlayerController)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void AReEchoGameMode::HandleEncounterEnded()
{
	if (bEncounterTransitioning || !Player)
	{
		return;
	}
	bEncounterTransitioning = true;
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const FReEchoRecording Recording = Player->Recorder->FinishRecording(
	    Director ? Director->EncounterTime : GetDefault<UReEchoBalanceSettings>()->EncounterDuration);
	const bool bPlayerSurvived = Player->Combatant->IsAlive();
	const bool bBossKilled =
	    bPlayerSurvived && bEncounterClearedByDefeat &&
	    RunSubsystem->EncounterIndex == GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
	RunSubsystem->CompleteEncounter(Recording, bPlayerSurvived, bBossKilled);
	if (RunSubsystem->Phase == EReEchoRunPhase::Summary || RunSubsystem->Phase == EReEchoRunPhase::Failed)
	{
		RunSubsystem->DeleteSavedRun();
	}
	else
	{
		RunSubsystem->SaveRun();
	}
	if (!bPlayerSurvived)
	{
		return;
	}

	if (bBossKilled)
	{
		ShowRestartScreen(false, true);
	}
	else if (RunSubsystem->EncounterIndex < GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount())
	{
		ShowTraitCardChoice();
	}
}

void AReEchoGameMode::ShowTraitCardChoice()
{
	ClearCombatants();

	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!RunSubsystem || !PlayerController || TraitCardChoiceWidget)
	{
		return;
	}

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->Phase == EReEchoRunPhase::ForgeChoice
	                                                 ? RunSubsystem->GenerateForgeOffers()
	                                                 : RunSubsystem->GenerateTraitCardOffers(3);
	if (Offers.Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Expected three trait card offers, received %d"), Offers.Num());
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
		return;
	}

	TraitCardChoiceWidget =
	    CreateWidget<UReEchoTraitCardChoiceWidget>(PlayerController, UReEchoTraitCardChoiceWidget::StaticClass());
	if (!TraitCardChoiceWidget)
	{
		return;
	}

	TraitCardChoiceWidget->InitializeOffers(
	    Offers, RunSubsystem->TimeShards, RunSubsystem->Phase == EReEchoRunPhase::ForgeChoice);
	TraitCardChoiceWidget->OnCardSelected.AddDynamic(this, &AReEchoGameMode::HandleTraitCardSelected);
	AddWidgetToUILayer(TraitCardChoiceWidget, EReEchoUILayer::BuildChoice);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TraitCardChoiceWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	SetPlayerMenuAbilityBlocked(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleTraitCardSelected(const FName CardId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const bool bForgeChoice = RunSubsystem->Phase == EReEchoRunPhase::ForgeChoice;
	const bool bApplied = bForgeChoice ? RunSubsystem->ApplyForgeChoice(CardId) : RunSubsystem->ApplyTraitCard(CardId);
	if (!bApplied)
	{
		return;
	}
	RunSubsystem->SaveRun();

	if (TraitCardChoiceWidget)
	{
		TraitCardChoiceWidget->RemoveFromParent();
		TraitCardChoiceWidget = nullptr;
	}

	if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
	}
	else
	{
		bContinueRunAfterShop = true;
		ShowInventoryShopMenu(true);
		if (!InventoryShopWidget)
		{
			bContinueRunAfterShop = false;
			RestoreGameInput();
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
		}
	}
}

void AReEchoGameMode::SetPlayerMenuAbilityBlocked(const bool bBlocked)
{
	if (Player && Player->AbilitySystem)
	{
		Player->AbilitySystem->SetLooseGameplayTagCount(ReEchoGameplayTags::State_Menu, bBlocked ? 1 : 0);
	}
}

void AReEchoGameMode::RestoreGameInput()
{
	SetPlayerMenuAbilityBlocked(false);
	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (FixedCamera)
		{
			PlayerController->SetViewTarget(FixedCamera);
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}

void AReEchoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAwaitingStartChoice || !Director || !Player)
	{
		return;
	}
	if (!bEncounterTransitioning)
	{
		bool bAnyEnemyAlive = false;
		for (TActorIterator<AReEchoEnemyActor> EnemyIterator(GetWorld()); EnemyIterator; ++EnemyIterator)
		{
			if (EnemyIterator->IsAlive())
			{
				bAnyEnemyAlive = true;
				break;
			}
		}
		if (!bAnyEnemyAlive)
		{
			bEncounterClearedByDefeat = true;
			Director->EndEncounter();
		}
	}
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (EncounterHudWidget)
	{
		EncounterHudWidget->SetEncounterStatus(RunSubsystem ? RunSubsystem->EncounterIndex : 0,
		                                       GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount(),
		                                       Director->GetRemainingTime());
	}
}
