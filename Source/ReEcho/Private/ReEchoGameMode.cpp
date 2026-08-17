#include "ReEchoGameMode.h"

#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BillboardComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
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
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
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
#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"
#include "UI/ReEchoUIManagerSubsystem.h"
#include "UI/ReEchoWeatherWidget.h"
#include "UObject/ConstructorHelpers.h"

AReEchoGameMode::AReEchoGameMode()
{
	DefaultPawnClass = AReEchoPlayerPawn::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	EnemyRoster = CreateDefaultSubobject<UReEchoEnemyRosterComponent>(TEXT("EnemyRoster"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> ArenaBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/Scenes/ArenaGround3D.ArenaGround3D"));
	ArenaBackgroundTexture = ArenaBackgroundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArenaMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/M_ArenaBackground.M_ArenaBackground"));
	ArenaBackgroundMaterial = ArenaMaterialFinder.Object;
}

UReEchoAudioService* AReEchoGameMode::GetAudioService() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoAudioService>() : nullptr;
}

void AReEchoGameMode::SetMusicState(const FName StateId) const
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetMusicState(StateId);
	}
}

void AReEchoGameMode::SetAmbienceState(const FName StateId) const
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->SetAmbienceState(StateId);
	}
}

void AReEchoGameMode::StopAmbienceState() const
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->StopAmbienceState();
	}
}

void AReEchoGameMode::PostAudioEvent(const FName EventId, const FVector& WorldLocation) const
{
	if (UReEchoAudioService* AudioService = GetAudioService())
	{
		AudioService->PostEventById(const_cast<AReEchoGameMode*>(this), EventId, WorldLocation);
	}
}

void AReEchoGameMode::PostUiEvent(const FName EventId) const
{
	if (GetGameInstance())
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->PostUiEvent(EventId);
		}
	}
}

void AReEchoGameMode::RestoreEncounterAudioState()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (!RunSubsystem || RunSubsystem->Phase != EReEchoRunPhase::Encounter)
	{
		return;
	}
	SetMusicState(IsBossEncounter() ? FReEchoAudioEvents::MusicBoss : FReEchoAudioEvents::MusicEncounter);
	UpdateWeatherScene(RunSubsystem->EncounterIndex);
}

void AReEchoGameMode::ResumeWorldForMenuTransition()
{
	// World timers do not advance while paused. Keep the menu input policy and gameplay ability block in place
	// while allowing a next-tick callback to replace the current menu safely outside Slate's click dispatch.
	if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
	{
		UIFlow->PreparePausedScreenTransition(this);
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
	PrintGMResult(TEXT("GMStatus | GMHeal [amount, 0=full] | GMAddShards [amount] | GMWeather <Clear|Rain|Fog> | "
	                   "GMKillAll | GMGotoBoss"));
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
	SetAmbienceState(WeatherScene == EReEchoWeatherScene::Rain ? FReEchoAudioEvents::AmbienceRain
	                                                        : FReEchoAudioEvents::AmbienceArena);
	PrintGMResult(FString::Printf(TEXT("Weather=%s"), *Scene));
}

void AReEchoGameMode::GMKillAll()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	int32 KilledCount = 0;
	for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
	{
		AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(EnemyHost.Get());
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

void AReEchoGameMode::GMGotoBoss()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}

	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !Director || !Player || !Player->Combatant)
	{
		PrintGMResult(TEXT("Boss encounter cannot start because the active run is not initialized."), false);
		return;
	}
	if (RunSubsystem->Phase != EReEchoRunPhase::Encounter || bAwaitingStartChoice || bEncounterTransitioning ||
	    !Player->Combatant->IsAlive())
	{
		PrintGMResult(TEXT("GMGotoBoss requires a living player in an active encounter."), false);
		return;
	}
	if (UGameplayStatics::IsGamePaused(this))
	{
		PrintGMResult(TEXT("Resume gameplay before using GMGotoBoss."), false);
		return;
	}

	const int32 BossEncounterIndex = GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
	if (BossEncounterIndex <= 0)
	{
		PrintGMResult(TEXT("The configured Boss encounter index is invalid."), false);
		return;
	}

	RunSubsystem->EncounterIndex = BossEncounterIndex - 1;
	BeginNextEncounter();
	const bool bStartedBossEncounter = RunSubsystem->EncounterIndex == BossEncounterIndex && IsBossEncounter();
	PrintGMResult(bStartedBossEncounter
	                  ? FString::Printf(TEXT("Started Boss encounter %d."), BossEncounterIndex)
	                  : FString::Printf(TEXT("Failed to start Boss encounter %d."), BossEncounterIndex),
	              bStartedBossEncounter);
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
	const float CameraOrthoWidth = SceneWorldHeight * SceneAspectRatio * 2.0f;
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
			PostAudioEvent(FReEchoAudioEvents::CameraMove, FixedCamera->GetActorLocation());
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
		UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
		WeatherWidget = UIFlow
		                    ? Cast<UReEchoWeatherWidget>(
		                          UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Weather, false, false))
		                    : nullptr;
		if (WeatherWidget)
		{
			WeatherWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			RefreshFogRevealSources();
		}
		EncounterHudWidget = UIFlow
		                         ? Cast<UReEchoEncounterHudWidget>(
		                               UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::EncounterHud, false, false))
		                         : nullptr;
		if (EncounterHudWidget)
		{
			EncounterHudWidget->SetVisibility(ESlateVisibility::Collapsed);
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
			UReEchoUIFlowCoordinatorSubsystem* UIFlow =
			    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
			PlayerHudWidget = UIFlow
			                      ? Cast<UReEchoPlayerHudWidget>(
			                            UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::PlayerHud, false, false))
			                      : nullptr;
			if (PlayerHudWidget)
			{
				PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CharacterSprite->Sprite);
				PlayerHudWidget->SetVisibility(ESlateVisibility::Collapsed);
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	StartMenuWidget = UIFlow
	                      ? Cast<UReEchoStartMenuWidget>(
	                            UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::StartMenu, true, true))
	                      : nullptr;
	if (!StartMenuWidget)
	{
		return;
	}
	SetMusicState(FReEchoAudioEvents::MusicMenu);
	StopAmbienceState();
	const bool bHasSavedRun = RunSubsystem->HasSavedRun();
	StartMenuWidget->InitializeMenu(bHasSavedRun);
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[ReEchoStartFlow] Showing start menu. HasSavedRun=%s"),
	       bHasSavedRun ? TEXT("true") : TEXT("false"));
	StartMenuWidget->OnNewGameRequested.AddDynamic(this, &AReEchoGameMode::HandleNewGameRequested);
	StartMenuWidget->OnContinueGameRequested.AddDynamic(this, &AReEchoGameMode::HandleContinueGameRequested);
	StartMenuWidget->OnGameSettingRequested.AddDynamic(this, &AReEchoGameMode::HandleStartSettingsRequested);
	StartMenuWidget->SetVisibility(ESlateVisibility::Visible);
	SetPlayerMenuAbilityBlocked(true);
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
		PostUiEvent(FReEchoAudioEvents::UiError);
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
	PostUiEvent(FReEchoAudioEvents::UiCancel);
	if (SettingsWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::Settings);
		}
		SettingsWidget = nullptr;
	}

	if (bSettingsReturnToStartMenu && StartMenuWidget)
	{
		StartMenuWidget->SetVisibility(ESlateVisibility::Visible);
		StartMenuWidget->SetKeyboardFocus();
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->FocusScreen(UGameplayStatics::GetPlayerController(this, 0), EReEchoUIScreen::StartMenu, true);
		}
	}
	else if (RestartWidget)
	{
		RestartWidget->SetVisibility(ESlateVisibility::Visible);
		RestartWidget->SetKeyboardFocus();
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->FocusScreen(UGameplayStatics::GetPlayerController(this, 0), EReEchoUIScreen::Restart, false);
		}
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	SettingsWidget = UIFlow
	                     ? Cast<UReEchoSettingsWidget>(
	                           UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Settings, true, false))
	                     : nullptr;
	if (!SettingsWidget)
	{
		return;
	}

	bSettingsReturnToStartMenu = bReturnToStartMenu;
	SettingsWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleSettingsClosed);
}

void AReEchoGameMode::ShowLoadoutSelection()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || LoadoutSelectionWidget)
	{
		return;
	}
	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	UReEchoLoadoutSelectionWidget* NewLoadoutSelectionWidget =
	    UIFlow ? Cast<UReEchoLoadoutSelectionWidget>(
	                 UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Loadout, true, true))
	           : nullptr;
	if (!NewLoadoutSelectionWidget)
	{
		return;
	}
	if (StartMenuWidget)
	{
		UIFlow->CloseScreen(EReEchoUIScreen::StartMenu);
		StartMenuWidget = nullptr;
	}

	LoadoutSelectionWidget = NewLoadoutSelectionWidget;
	LoadoutSelectionWidget->OnLoadoutConfirmed.AddDynamic(this, &AReEchoGameMode::HandleLoadoutConfirmed);
	LoadoutSelectionWidget->SetVisibility(ESlateVisibility::Visible);
	SetPlayerMenuAbilityBlocked(true);
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
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::StartMenu);
		}
		StartMenuWidget = nullptr;
	}
	if (LoadoutSelectionWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::Loadout);
		}
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
	if (WeatherWidget)
	{
		WeatherWidget->SetWeatherScene(WeatherScene);
	}
	SetAmbienceState(WeatherScene == EReEchoWeatherScene::Rain ? FReEchoAudioEvents::AmbienceRain
	                                                        : FReEchoAudioEvents::AmbienceArena);
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
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		if (AActor* EnemyHost = Entry.Host.Get())
		{
			EnemyHost->Destroy();
		}
	}
	EnemyRoster->ResetRoster();
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->Destroy();
		}
	}
	Echoes.Reset();
	RefreshFogRevealSources();
}

void AReEchoGameMode::RefreshFogRevealSources()
{
	if (!WeatherWidget)
	{
		return;
	}

	TArray<AActor*> EchoRevealSources;
	EchoRevealSources.Reserve(Echoes.Num());
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (IsValid(Echo))
		{
			EchoRevealSources.Add(Echo);
		}
	}
	WeatherWidget->SetFogRevealSources(Player, EchoRevealSources);
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
	bBossPostEchoPhaseTriggered = false;
	RunSubsystem->BeginEncounter();
	Director->SetEndsOnDuration(!IsBossEncounter());
	SetMusicState(IsBossEncounter() ? FReEchoAudioEvents::MusicBoss : FReEchoAudioEvents::MusicEncounter);
	UpdateWeatherScene(RunSubsystem->EncounterIndex);
	if (Player)
	{
		Player->SetActorLocation(FVector(0, 0, 112));
		Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
		Player->RestoreEquippedWeapon(RunSubsystem->CurrentBuild.WeaponId);
		Player->SetAutoAttackMode(RunSubsystem->IsAutomaticAttackMode());
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
	// Plan31: resolve the full selected set (zero, one or several) and spawn one independent
	// Echo actor per recording. Each Echo owns its immutable recording and build snapshot, so its
	// playback, position, weapon and run state stay independent of the others.
	const TArray<FReEchoRecording> Recordings =
	    RunSubsystem->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	for (const FReEchoRecording& Recording : Recordings)
	{
		AReEchoEchoActor* Echo = GetWorld()->SpawnActor<AReEchoEchoActor>();
		if (Echo && Echo->InitializeEcho(
		                Recording, RunSubsystem->CurrentBuild.Stats.EchoEfficiency, RunSubsystem->GetRunDataSnapshot()))
		{
			Echoes.Add(Echo);
		}
		else if (Echo)
		{
			Echo->Destroy();
		}
	}
	RefreshFogRevealSources();
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
	Result.bBossPostEchoPhaseTriggered = bBossPostEchoPhaseTriggered;
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		if (Entry.bAlive)
		{
			if (const AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Entry.Host.Get()))
			{
				Result.Enemies.Add(Enemy->CaptureRuntimeState());
			}
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
	bBossPostEchoPhaseTriggered = SavedState.bBossPostEchoPhaseTriggered;
	Director->SetEndsOnDuration(!IsBossEncounter());
	SetMusicState(IsBossEncounter() ? FReEchoAudioEvents::MusicBoss : FReEchoAudioEvents::MusicEncounter);
	UpdateWeatherScene(RunSubsystem->EncounterIndex);

	Player->SetActorTransform(SavedState.PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
	Player->RestoreEquippedWeapon(RunSubsystem->CurrentBuild.WeaponId);
	Player->SetAutoAttackMode(RunSubsystem->IsAutomaticAttackMode());
	Player->Combatant->InitializeFromStats(SavedState.PlayerStats, true);
	Player->Combatant->RestoreCurrentHealth(SavedState.PlayerHealth);
	Player->Movement->MaxSpeed = 420.0f * SavedState.PlayerStats.MovementSpeed;
	Player->Movement->Velocity = SavedState.PlayerVelocity;
	Player->Recorder->ResumeRecording(SavedState.ActiveRecording);
	if (PlayerHudWidget)
	{
		PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CharacterSprite->Sprite);
	}

	// Plan31: resume the same selected set as a fresh encounter, one independent Echo per
	// recording. Once the Boss post-echo phase has fired, echoes must stay absent after restore.
	if (!bBossPostEchoPhaseTriggered)
	{
		const TArray<FReEchoRecording> Recordings =
		    RunSubsystem->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
		for (const FReEchoRecording& Recording : Recordings)
		{
			AReEchoEchoActor* Echo = GetWorld()->SpawnActor<AReEchoEchoActor>();
			if (Echo)
			{
				if (Echo->InitializeEcho(Recording,
				                         RunSubsystem->CurrentBuild.Stats.EchoEfficiency,
				                         RunSubsystem->GetRunDataSnapshot()))
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
	}
	RefreshFogRevealSources();

	const TSharedPtr<const FReEchoCsvDataSnapshot> DataSnapshot = RunSubsystem->GetRunDataSnapshot();
	for (const FReEchoEnemyRuntimeState& EnemyState : SavedState.Enemies)
	{
		AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>();
		if (Enemy)
		{
			const EReEchoEnemyKind SavedKind = EnemyState.Kind <= static_cast<uint8>(EReEchoEnemyKind::Boss)
			                                         ? static_cast<EReEchoEnemyKind>(EnemyState.Kind)
			                                         : EReEchoEnemyKind::Grunt;
			const FName EnemyId = SavedKind == EReEchoEnemyKind::Boss      ? FName(TEXT("M_TimeGuard"))
			                      : SavedKind == EReEchoEnemyKind::Bomber ? FName(TEXT("M_Bomber"))
			                      : SavedKind == EReEchoEnemyKind::Shield ? FName(TEXT("M_Shield"))
			                                                                : FName(TEXT("M_Grunt"));
			FReEchoEnemyDefinition Definition;
			FString CompileError;
			if (!DataSnapshot ||
			    !ReEchoEnemyDefinitionCompiler::Compile(*DataSnapshot, EnemyId, Definition, CompileError) ||
			    !Enemy->ConfigureFromDefinition(Definition, EnemyState.SpawnIndex))
			{
				UE_LOG(LogTemp, Error, TEXT("Plan44 enemy restore failed: %s"), *CompileError);
				Enemy->Destroy();
				continue;
			}
			Enemy->RestoreRuntimeState(EnemyState);
			Enemy->SetEnemyRoster(EnemyRoster);
			if (UReEchoEnemyEventsComponent* Events = Enemy->GetEnemyEventsComponent())
			{
				Events->OnBossIntent.AddUniqueDynamic(this, &AReEchoGameMode::HandleBossIntent);
			}
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

	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> DataSnapshot = RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	auto SpawnEnemy = [&](const FName EnemyId)
	{
		if (!DataSnapshot)
		{
			UE_LOG(LogTemp, Error, TEXT("Plan44 enemy spawn failed: run data snapshot is unavailable."));
			return;
		}
		FReEchoEnemyDefinition Definition;
		FString CompileError;
		if (!ReEchoEnemyDefinitionCompiler::Compile(*DataSnapshot, EnemyId, Definition, CompileError))
		{
			UE_LOG(LogTemp, Error, TEXT("Plan44 enemy spawn failed: %s"), *CompileError);
			return;
		}
		AReEchoEnemyActor* Enemy =
		    GetWorld()->SpawnActor<AReEchoEnemyActor>(GetPeripheralSpawnLocation(), FRotator::ZeroRotator);
		if (Enemy)
		{
			if (!Enemy->ConfigureFromDefinition(Definition, ++SpawnIndex))
			{
				Enemy->Destroy();
				return;
			}
			Enemy->SetEnemyRoster(EnemyRoster);
			if (UReEchoEnemyEventsComponent* Events = Enemy->GetEnemyEventsComponent())
			{
				Events->OnBossIntent.AddUniqueDynamic(this, &AReEchoGameMode::HandleBossIntent);
			}
		}
	};

	if (bBossEncounter)
	{
		SpawnEnemy(TEXT("M_TimeGuard"));
	}
	for (int32 EnemyIndex = 0; EnemyIndex < GruntCount; ++EnemyIndex)
	{
		SpawnEnemy(TEXT("M_Grunt"));
	}
	for (int32 EnemyIndex = 0; EnemyIndex < BomberCount; ++EnemyIndex)
	{
		SpawnEnemy(TEXT("M_Bomber"));
	}
}

bool AReEchoGameMode::IsBossEncounter() const
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>()
	                                                          : nullptr;
	return RunSubsystem &&
	       RunSubsystem->EncounterIndex == GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
}

void AReEchoGameMode::TriggerBossPostEchoPhase(const FReEchoBossPhaseDefinition& PhaseDefinition)
{
	if (bBossPostEchoPhaseTriggered || !IsBossEncounter() || !PhaseDefinition.bEnabled || !Player ||
	    !Player->Combatant)
	{
		return;
	}
	bBossPostEchoPhaseTriggered = true;
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->Destroy();
		}
	}
	Echoes.Reset();
	RefreshFogRevealSources();

	FReEchoStatBlock BoostedStats = Player->Combatant->Stats;
	BoostedStats.PhysicalAttack *= FMath::Max(0.0f, PhaseDefinition.PhysicalAttackMultiplier);
	BoostedStats.ElementalAttack *= FMath::Max(0.0f, PhaseDefinition.ElementalAttackMultiplier);
	BoostedStats.AttackSpeed *= FMath::Max(0.0f, PhaseDefinition.AttackSpeedMultiplier);
	BoostedStats.MovementSpeed *= FMath::Max(0.0f, PhaseDefinition.MovementSpeedMultiplier);
	Player->Combatant->InitializeFromStats(BoostedStats, false);
	Player->Movement->MaxSpeed = 420.0f * BoostedStats.MovementSpeed;
}

void AReEchoGameMode::HandleBossIntent(const FReEchoBossIntent& Intent)
{
	if (Intent.Type == EReEchoBossIntentType::EncounterPhase &&
	    Intent.PhaseDefinition.EchoPolicy == EReEchoBossEchoPolicy::RetireEncounterEchoes)
	{
		TriggerBossPostEchoPhase(Intent.PhaseDefinition);
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	RestartWidget = UIFlow
	                    ? Cast<UReEchoRestartWidget>(
	                          UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Restart, false, true))
	                    : nullptr;
	if (!RestartWidget)
	{
		return;
	}

	bRestartScreenIsTerminal = bDeathScreen || bVictoryScreen;
	bRestartScreenIsDeath = bDeathScreen;
	if (bRestartScreenIsTerminal)
	{
		SetMusicState(bVictoryScreen ? FReEchoAudioEvents::MusicVictory : FReEchoAudioEvents::MusicDeath);
		StopAmbienceState();
	}
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
	if (!bDeathScreen && !bVictoryScreen)
	{
		const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
		RestartWidget->SetAutomaticAttackMode(RunSubsystem ? RunSubsystem->IsAutomaticAttackMode() : true);
		RestartWidget->OnAutomaticAttackRequested.AddDynamic(this, &AReEchoGameMode::HandleAutomaticAttackRequested);
		RestartWidget->OnManualAttackRequested.AddDynamic(this, &AReEchoGameMode::HandleManualAttackRequested);
	}
	SetPlayerMenuAbilityBlocked(true);
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	StatsWidget = UIFlow
	                  ? Cast<UReEchoStatsWidget>(
	                        UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Stats, false, true))
	                  : nullptr;
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
	SetPlayerMenuAbilityBlocked(true);
}

void AReEchoGameMode::HandleStatsClosed()
{
	if (StatsWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::Stats);
		}
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
	ShowInventoryShopMenu(EReEchoInventoryShopMode::Inventory);
}

void AReEchoGameMode::ToggleShopMenu()
{
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	ShowInventoryShopMenu(EReEchoInventoryShopMode::ManualShop);
}

void AReEchoGameMode::ShowInventoryShopMenu(const EReEchoInventoryShopMode Mode)
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	InventoryShopWidget = UIFlow
	                          ? Cast<UReEchoInventoryShopWidget>(
	                                UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::InventoryShop, false, true))
	                          : nullptr;
	if (!InventoryShopWidget)
	{
		return;
	}

	InventoryShopWidget->OnClosed.AddUObject(this, &AReEchoGameMode::HandleInventoryShopClosed);
	InventoryShopWidget->OnPurchaseRequested.AddUObject(this, &AReEchoGameMode::HandleShopPurchaseRequested);
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission)
	{
		bPostTraitShopClosing = false;
		InventoryShopWidget->OnEchoStoreRequested.AddUObject(this, &AReEchoGameMode::HandleEchoStoreRequested);
		InventoryShopWidget->OnEchoSkipRequested.AddUObject(this, &AReEchoGameMode::HandleEchoSkipRequested);
		InventoryShopWidget->OnEchoReplaceRequested.AddUObject(this, &AReEchoGameMode::HandleEchoReplaceRequested);
		InventoryShopWidget->OnEchoSelectionRequested.AddUObject(this, &AReEchoGameMode::HandleEchoSelectionRequested);
		InventoryShopWidget->OnEchoSkipAndCloseRequested.AddUObject(
		    this, &AReEchoGameMode::HandleEchoSkipAndCloseRequested);
		InventoryShopWidget->ShowPostTraitIntermission(
		    RunSubsystem->TimeShards, RunSubsystem->InventoryItems, RunSubsystem->GetEchoStorageSummary());
	}
	else if (Mode == EReEchoInventoryShopMode::ManualShop)
	{
		InventoryShopWidget->ShowShop(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
	}
	else
	{
		InventoryShopWidget->ShowInventory(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
	}
	if (Mode != EReEchoInventoryShopMode::Inventory)
	{
		SetMusicState(FReEchoAudioEvents::MusicShop);
		StopAmbienceState();
	}
	SetPlayerMenuAbilityBlocked(true);
}

void AReEchoGameMode::HandleInventoryShopClosed()
{
	if (bPostTraitShopClosing)
	{
		return;
	}
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const bool bPostTraitIntermission =
	    InventoryShopWidget && InventoryShopWidget->GetMode() == EReEchoInventoryShopMode::PostTraitIntermission;
	const bool bManualShop =
	    InventoryShopWidget && InventoryShopWidget->GetMode() == EReEchoInventoryShopMode::ManualShop;
	if (bPostTraitIntermission && RunSubsystem && RunSubsystem->GetEchoStorageSummary().bHasPendingRecording)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "ResolveEchoBeforeClosing", "Store this echo or explicitly skip it before continuing."));
		return;
	}
	PostUiEvent(FReEchoAudioEvents::UiCancel);
	if (bPostTraitIntermission)
	{
		bPostTraitShopClosing = true;
	}
	const bool bShouldStartNextEncounter = bContinueRunAfterShop;
	bContinueRunAfterShop = false;
	if (InventoryShopWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::InventoryShop);
		}
		InventoryShopWidget = nullptr;
	}
	RestoreGameInput();
	if (bShouldStartNextEncounter)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
	}
	else if (bManualShop)
	{
		RestoreEncounterAudioState();
	}
}

void AReEchoGameMode::HandleShopPurchaseRequested(const FName ItemId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (RunSubsystem && InventoryShopWidget && RunSubsystem->PurchaseShopItem(ItemId))
	{
		PostUiEvent(FReEchoAudioEvents::UiPurchase);
		RunSubsystem->SaveRun();
		if (InventoryShopWidget->GetMode() == EReEchoInventoryShopMode::PostTraitIntermission)
		{
			InventoryShopWidget->ShowPostTraitIntermission(
			    RunSubsystem->TimeShards, RunSubsystem->InventoryItems, RunSubsystem->GetEchoStorageSummary());
		}
		else
		{
			InventoryShopWidget->ShowShop(RunSubsystem->TimeShards, RunSubsystem->InventoryItems);
		}
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
	}
}

namespace
{
FText GetEchoCommandFailureText(const EReEchoEchoStorageResult Result)
{
	switch (Result)
	{
	case EReEchoEchoStorageResult::NoPendingRecording:
		return NSLOCTEXT("ReEcho", "EchoNoPendingFailure", "There is no pending echo to resolve.");
	case EReEchoEchoStorageResult::StorageFull:
		return NSLOCTEXT("ReEcho", "EchoStorageFullFailure", "Storage is full. Choose an echo to replace.");
	case EReEchoEchoStorageResult::InvalidReplacementTarget:
		return NSLOCTEXT("ReEcho", "EchoInvalidReplacementFailure", "That stored echo is no longer available.");
	case EReEchoEchoStorageResult::ReplayLimitExceeded:
		return NSLOCTEXT("ReEcho", "EchoReplayLimitFailure", "Too many echoes were selected.");
	case EReEchoEchoStorageResult::InvalidRecordingId:
	case EReEchoEchoStorageResult::DuplicateRecordingId:
		return NSLOCTEXT("ReEcho", "EchoInvalidSelectionFailure", "The echo selection is no longer valid.");
	default:
		return NSLOCTEXT("ReEcho", "EchoCommandFailure", "The echo change was rejected.");
	}
}
}

void AReEchoGameMode::HandleEchoStoreRequested()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->StorePendingRecording();
	if (Result == EReEchoEchoStorageResult::Success)
	{
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoStored", "Echo stored.")
		           : NSLOCTEXT("ReEcho", "EchoStoreSaveFailed", "Echo stored in this session, but saving failed."));
	}
	else if (Result == EReEchoEchoStorageResult::StorageFull)
	{
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->EnterEchoReplacementMode();
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(GetEchoCommandFailureText(Result));
	}
}

void AReEchoGameMode::HandleEchoSkipRequested()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->SkipPendingRecordingStorage();
	if (Result == EReEchoEchoStorageResult::Success)
	{
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoSkipped", "Echo skipped.")
		           : NSLOCTEXT("ReEcho", "EchoSkipSaveFailed", "Echo skipped in this session, but saving failed."));
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(GetEchoCommandFailureText(Result));
	}
}

void AReEchoGameMode::HandleEchoReplaceRequested(const FGuid RecordingId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->StorePendingRecordingReplacing(RecordingId);
	if (Result == EReEchoEchoStorageResult::Success)
	{
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoReplaced", "Stored echo replaced.")
		           : NSLOCTEXT("ReEcho", "EchoReplaceSaveFailed", "Echo replaced in this session, but saving failed."));
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(GetEchoCommandFailureText(Result));
	}
}

void AReEchoGameMode::HandleEchoSelectionRequested(const TArray<FGuid>& RecordingIds)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->SetSelectedReplayIds(RecordingIds);
	if (Result == EReEchoEchoStorageResult::Success)
	{
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoSelectionSaved", "Replay selection saved.")
		           : NSLOCTEXT("ReEcho", "EchoSelectionSaveFailed", "Selection changed in this session, but saving failed."));
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(GetEchoCommandFailureText(Result));
	}
}

void AReEchoGameMode::HandleEchoSkipAndCloseRequested()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget || bPostTraitShopClosing)
	{
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->SkipPendingRecordingStorage();
	if (Result != EReEchoEchoStorageResult::Success)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(GetEchoCommandFailureText(Result));
		return;
	}
	const FReEchoEncounterRuntimeState EncounterState = CaptureEncounterRuntimeState();
	const bool bSaved = RunSubsystem->Phase == EReEchoRunPhase::Encounter
	                        ? EncounterState.bValid && RunSubsystem->SaveRun(&EncounterState)
	                        : RunSubsystem->SaveRun();
	if (!bSaved)
	{
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "EchoCloseSaveFailed", "The echo was skipped, but saving failed. The shop remains open."));
		return;
	}
	const FReEchoEchoStorageSummary Summary = RunSubsystem->GetEchoStorageSummary();
	InventoryShopWidget->SetEchoSummary(Summary);
	if (!Summary.bHasPendingRecording)
	{
		InventoryShopWidget->CompletePostTraitClose();
	}
}

void AReEchoGameMode::HandleResumeRequested()
{
	bQuitConfirmationVisible = false;
	if (RestartWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::Restart);
		}
		RestartWidget = nullptr;
	}
	bRestartScreenIsTerminal = false;
	RestoreGameInput();
}

void AReEchoGameMode::HandleAutomaticAttackRequested()
{
	ApplyAttackModeChoice(true);
}

void AReEchoGameMode::HandleManualAttackRequested()
{
	ApplyAttackModeChoice(false);
}

void AReEchoGameMode::ApplyAttackModeChoice(const bool bAutomatic)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !RestartWidget || bRestartScreenIsTerminal)
	{
		return;
	}
	RunSubsystem->SetAutomaticAttackMode(bAutomatic);
	if (Player)
	{
		Player->SetAutoAttackMode(bAutomatic);
	}
	const FReEchoEncounterRuntimeState EncounterState = CaptureEncounterRuntimeState();
	const bool bSaved = RunSubsystem->Phase == EReEchoRunPhase::Encounter
	                        ? EncounterState.bValid && RunSubsystem->SaveRun(&EncounterState)
	                        : RunSubsystem->SaveRun();
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Attack mode changed in memory, but the run save could not be updated."));
	}
	RestartWidget->SetAutomaticAttackMode(RunSubsystem->IsAutomaticAttackMode());
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
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::Restart);
		}
		RestartWidget = nullptr;
	}
	bRestartScreenIsTerminal = false;
	if (bRestartScreenIsDeath)
	{
		if (UReEchoAudioService* AudioService = GetAudioService())
		{
			AudioService->QueueEventForNextWorld(FReEchoAudioEvents::Revive);
		}
	}
	bRestartScreenIsDeath = false;

	UGameplayStatics::SetGamePaused(this, false);
	if (UReEchoUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UReEchoUIManagerSubsystem>())
	{
		UIManager->ConfigureGameplayInput(PlayerController);
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	TraitCardChoiceWidget = UIFlow
	                            ? Cast<UReEchoTraitCardChoiceWidget>(
	                                  UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::TraitChoice, false, true))
	                            : nullptr;
	if (!TraitCardChoiceWidget)
	{
		return;
	}
	SetMusicState(FReEchoAudioEvents::MusicShop);
	StopAmbienceState();

	TraitCardChoiceWidget->InitializeOffers(
	    Offers, RunSubsystem->TimeShards, RunSubsystem->Phase == EReEchoRunPhase::ForgeChoice);
	TraitCardChoiceWidget->OnCardSelected.AddDynamic(this, &AReEchoGameMode::HandleTraitCardSelected);
	SetPlayerMenuAbilityBlocked(true);
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
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	PostUiEvent(FReEchoAudioEvents::UiCardSelect);
	RunSubsystem->SaveRun();

	if (TraitCardChoiceWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		}
		TraitCardChoiceWidget = nullptr;
	}
	ResumeWorldForMenuTransition();

	if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
	}
	else
	{
		bContinueRunAfterShop = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowPostTraitShop);
	}
}

void AReEchoGameMode::ShowPostTraitShop()
{
	ShowInventoryShopMenu(EReEchoInventoryShopMode::PostTraitIntermission);
	if (InventoryShopWidget)
	{
		return;
	}

	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (RunSubsystem && RunSubsystem->GetEchoStorageSummary().bHasPendingRecording)
	{
		UE_LOG(LogTemp, Error, TEXT("Post-trait shop failed to open while an echo decision is pending."));
		return;
	}
	bContinueRunAfterShop = false;
	RestoreGameInput();
	GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
}

void AReEchoGameMode::SetPlayerMenuAbilityBlocked(const bool bBlocked)
{
	if (bBlocked && Player)
	{
		// 菜单开启时释放所有 held basic-attack 输入源（自动循环 + 物理），
		// 保证恢复游戏不会残留任一来源的陈旧 held 状态。
		Player->ReleaseAllBasicAttackInputs();
	}
	if (Player && Player->AbilitySystem)
	{
		Player->AbilitySystem->SetLooseGameplayTagCount(ReEchoGameplayTags::State_Menu, bBlocked ? 1 : 0);
	}
}

void AReEchoGameMode::RestoreGameInput()
{
	SetPlayerMenuAbilityBlocked(false);
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (FixedCamera)
		{
			PlayerController->SetViewTarget(FixedCamera);
		}
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->RestoreGameplay(this, PlayerController);
		}
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
		bool bEncounterDefeated = !EnemyRoster->HasLivingEnemies();
		if (IsBossEncounter())
		{
			bEncounterDefeated = true;
			for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
			{
				if (Entry.Archetype == EReEchoEnemyArchetype::Boss && Entry.bAlive)
				{
					bEncounterDefeated = false;
					break;
				}
			}
		}
		if (bEncounterDefeated)
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
