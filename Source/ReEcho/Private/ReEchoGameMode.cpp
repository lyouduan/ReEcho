#include "ReEchoGameMode.h"

#include "ReEcho.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystemComponent.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoHitResolver.h"
#include "Camera/CameraComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Enemy/ReEchoEnemyGameplayClassRegistry.h"
#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Run/ReEchoShopCatalog.h"
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
	static ConstructorHelpers::FClassFinder<AReEchoPlayerPawn> PlayerPrefab(
	    TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay"));
	DefaultPawnClass = PlayerPrefab.Succeeded() ? PlayerPrefab.Class.Get() : AReEchoPlayerPawn::StaticClass();
	static ConstructorHelpers::FObjectFinder<UReEcho2DPresentationCatalog> CatalogFinder(
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog"));
	PresentationCatalog = CatalogFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEchoEnemyGameplayClassRegistry> EnemyClassRegistryFinder(
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyGameplayClassRegistry."
	         "DA_EnemyGameplayClassRegistry"));
	EnemyGameplayClassRegistry = EnemyClassRegistryFinder.Object;
	PrimaryActorTick.bCanEverTick = true;
	EnemyRoster = CreateDefaultSubobject<UReEchoEnemyRosterComponent>(TEXT("EnemyRoster"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> ArenaBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/Scenes/ArenaGround3D.ArenaGround3D"));
	ArenaBackgroundTexture = ArenaBackgroundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArenaMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/M_ArenaBackground.M_ArenaBackground"));
	ArenaBackgroundMaterial = ArenaMaterialFinder.Object;
}

TSubclassOf<AReEchoEnemyActor> AReEchoGameMode::ResolveEnemyClass(const FName PresentationId) const
{
	if (EnemyGameplayClassRegistry)
	{
		if (const TSubclassOf<AReEchoEnemyActor> ResolvedClass =
		        EnemyGameplayClassRegistry->ResolveGameplayClass(PresentationId))
		{
			return ResolvedClass;
		}
	}
	return AReEchoEnemyActor::StaticClass();
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
	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
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
	PrintGMResult(TEXT("The GM console pauses while open and resumes when closed. GMStatus | GMHeal [amount, 0=full] | "
	                   "GMGod [On|Off|Toggle] | "
	                   "GMAddShards [amount] | GMSetShards [amount] | GMWeather "
	                   "<Clear|Rain|Fog> | "
	                   "GMEndEncounter | GMKillAll | GMSpawnFox [distance] | GMGotoBoss | "
	                   "GMElement <None|Flame|Lightning|Grass|Water> | "
	                   "GMReaction <Burn|Vaporize|Growth|Conduct|EnhanceGrass|EnhanceWater> [damage]"));
	PrintGMResult(TEXT("Reactions: Flame+Grass=Burn | Flame+Water=Vaporize | Lightning+Grass=Growth | "
	                   "Lightning+Water=Conduct | Grass+Water=EnhanceGrass | Water+Grass=EnhanceWater"));
}

AReEchoEnemyActor* AReEchoGameMode::FindNearestLivingEnemyForGM() const
{
	if (!EnemyRoster)
	{
		return nullptr;
	}
	const FVector Origin = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	AReEchoEnemyActor* Nearest = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
	{
		AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(EnemyHost.Get());
		if (!Enemy || !Enemy->IsAlive())
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(Origin, Enemy->GetActorLocation());
		if (!Nearest || DistanceSquared < NearestDistanceSquared)
		{
			Nearest = Enemy;
			NearestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

FReEchoAttackIdentity AReEchoGameMode::MakeGMElementAttack()
{
	FReEchoAttackIdentity Attack;
	Attack.Source = Player;
	Attack.Sequence = ++GMElementAttackSequence;
	Attack.SourceFaction = EReEchoCombatFaction::PlayerSide;
	return Attack;
}

void AReEchoGameMode::GMElement(const FString& Element)
{
	if (!EnsureGMCommandAvailable() || !Player)
	{
		PrintGMResult(TEXT("GMElement requires an active player."), false);
		return;
	}

	EReEchoElement ParsedElement = EReEchoElement::None;
	if (Element.Equals(TEXT("Flame"), ESearchCase::IgnoreCase) || Element.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))
	{
		ParsedElement = EReEchoElement::Flame;
	}
	else if (Element.Equals(TEXT("Lightning"), ESearchCase::IgnoreCase) ||
	         Element.Equals(TEXT("Electricity"), ESearchCase::IgnoreCase))
	{
		ParsedElement = EReEchoElement::Lightning;
	}
	else if (Element.Equals(TEXT("Grass"), ESearchCase::IgnoreCase))
	{
		ParsedElement = EReEchoElement::Grass;
	}
	else if (Element.Equals(TEXT("Water"), ESearchCase::IgnoreCase))
	{
		ParsedElement = EReEchoElement::Water;
	}
	else if (!Element.Equals(TEXT("None"), ESearchCase::IgnoreCase) &&
	         !Element.Equals(TEXT("Clear"), ESearchCase::IgnoreCase))
	{
		PrintGMResult(TEXT("Usage: GMElement <None|Flame|Lightning|Grass|Water>"), false);
		return;
	}

	Player->SetDebugOutgoingElementOverride(ParsedElement);
	PrintGMResult(
	    ParsedElement == EReEchoElement::None
	        ? TEXT("Player element override=Off; weapon-authored elements restored.")
	        : FString::Printf(TEXT("Player element override=%s; all subsequent player hits use this element."),
	                          *ReEchoElementReaction::GetElementId(ParsedElement).ToString()));
}

void AReEchoGameMode::GMReaction(const FString& Reaction, const float Damage)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	AReEchoEnemyActor* Target = FindNearestLivingEnemyForGM();
	if (!Target)
	{
		PrintGMResult(TEXT("GMReaction requires at least one living enemy."), false);
		return;
	}

	EReEchoElement Attachment = EReEchoElement::None;
	EReEchoElement Trigger = EReEchoElement::None;
	bool bPrepareAllLivingEnemies = false;
	if (Reaction.Equals(TEXT("Burn"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Grass;
		Trigger = EReEchoElement::Flame;
	}
	else if (Reaction.Equals(TEXT("Vaporize"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Water;
		Trigger = EReEchoElement::Flame;
	}
	else if (Reaction.Equals(TEXT("Growth"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Grass;
		Trigger = EReEchoElement::Lightning;
	}
	else if (Reaction.Equals(TEXT("Conduct"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Water;
		Trigger = EReEchoElement::Lightning;
		bPrepareAllLivingEnemies = true;
	}
	else if (Reaction.Equals(TEXT("EnhanceGrass"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Water;
		Trigger = EReEchoElement::Grass;
	}
	else if (Reaction.Equals(TEXT("EnhanceWater"), ESearchCase::IgnoreCase))
	{
		Attachment = EReEchoElement::Grass;
		Trigger = EReEchoElement::Water;
	}
	else
	{
		PrintGMResult(TEXT("Usage: GMReaction <Burn|Vaporize|Growth|Conduct|EnhanceGrass|EnhanceWater> [damage]"),
		              false);
		return;
	}

	TArray<AReEchoEnemyActor*> PreparedTargets;
	if (bPrepareAllLivingEnemies)
	{
		for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
		{
			if (AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(EnemyHost.Get()); Enemy && Enemy->IsAlive())
			{
				PreparedTargets.Add(Enemy);
			}
		}
	}
	else
	{
		PreparedTargets.Add(Target);
	}

	const FVector SourceLocation = Player ? Player->GetActorLocation() : Target->GetActorLocation();
	for (AReEchoEnemyActor* PreparedTarget : PreparedTargets)
	{
		PreparedTarget->GetCombatantComponent()->ResetElementState();
		PreparedTarget->ReceiveElementalDamage(0.0f, Attachment, SourceLocation, 1.0f, MakeGMElementAttack());
	}
	Target->ReceiveElementalDamage(FMath::Max(0.0f, Damage), Trigger, SourceLocation, 1.0f, MakeGMElementAttack());
	PrintGMResult(FString::Printf(
	    TEXT("Triggered %s on %s; prepared targets=%d."), *Reaction, *Target->GetName(), PreparedTargets.Num()));
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
	const EReEchoElement ElementOverride = Player ? Player->GetDebugOutgoingElementOverride() : EReEchoElement::None;
	const FString ElementLabel = ElementOverride == EReEchoElement::None
	                                 ? TEXT("Weapon")
	                                 : ReEchoElementReaction::GetElementId(ElementOverride).ToString();
	PrintGMResult(FString::Printf(TEXT("Encounter=%d, HP=%.0f/%.0f, God=%s, Element=%s, TimeShards=%d, Echoes=%d"),
	                              RunSubsystem ? RunSubsystem->EncounterIndex : 0,
	                              Health,
	                              MaximumHealth,
	                              Player && Player->Combatant && Player->Combatant->IsDebugInvulnerable() ? TEXT("On")
	                                                                                                      : TEXT("Off"),
	                              *ElementLabel,
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

void AReEchoGameMode::GMGod(const FString& Mode)
{
	if (!EnsureGMCommandAvailable() || !Player || !Player->Combatant)
	{
		PrintGMResult(TEXT("GMGod requires an active player."), false);
		return;
	}

	const bool bCurrentlyEnabled = Player->Combatant->IsDebugInvulnerable();
	bool bEnable = !bCurrentlyEnabled;
	if (Mode.Equals(TEXT("On"), ESearchCase::IgnoreCase) || Mode.Equals(TEXT("1")))
	{
		bEnable = true;
	}
	else if (Mode.Equals(TEXT("Off"), ESearchCase::IgnoreCase) || Mode.Equals(TEXT("0")))
	{
		bEnable = false;
	}
	else if (!Mode.Equals(TEXT("Toggle"), ESearchCase::IgnoreCase))
	{
		PrintGMResult(TEXT("Usage: GMGod <On|Off|Toggle>"), false);
		return;
	}

	Player->Combatant->SetDebugInvulnerable(bEnable);
	PrintGMResult(FString::Printf(TEXT("Player invulnerability=%s."), bEnable ? TEXT("On") : TEXT("Off")));
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
	// #8-B: 商店打开时，GM 改动碎片后即时刷新持有碎片显示。
	if (InventoryShopWidget)
	{
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
	}
}

void AReEchoGameMode::GMSetShards(const int32 Amount)
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
	RunSubsystem->TimeShards = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Amount), 0, MAX_int32));
	PrintGMResult(FString::Printf(TEXT("TimeShards=%d"), RunSubsystem->TimeShards));
	// #8-B: 商店打开时，GM 改动碎片后即时刷新持有碎片显示。
	if (InventoryShopWidget)
	{
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
	}
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

void AReEchoGameMode::GMEndEncounter()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}

	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !Director || !Player || !Player->Combatant)
	{
		PrintGMResult(TEXT("The active encounter is not initialized."), false);
		return;
	}
	if (RunSubsystem->Phase != EReEchoRunPhase::Encounter || bAwaitingStartChoice || bEncounterTransitioning ||
	    !Player->Combatant->IsAlive())
	{
		PrintGMResult(TEXT("GMEndEncounter requires a living player in an active encounter."), false);
		return;
	}
	if (IsBossEncounter())
	{
		PrintGMResult(TEXT("GMEndEncounter cannot clear a Boss encounter; use GMKillAll instead."), false);
		return;
	}

	const int32 EncounterIndex = RunSubsystem->EncounterIndex;
	const float RemainingTime = Director->GetRemainingTime();
	Director->EndEncounter();
	PrintGMResult(
	    FString::Printf(TEXT("Ended encounter %d with %.1f seconds remaining; normal completion flow started."),
	                    EncounterIndex,
	                    RemainingTime));
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
		    Enemy ? Enemy->GetActorLocation() - Enemy->GetFacingDirection() * 100.0f : FVector::ZeroVector;
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

void AReEchoGameMode::GMSpawnFox(const float Distance)
{
	if (!EnsureGMCommandAvailable() || !Player || !Player->Combatant || !Player->Combatant->IsAlive())
	{
		PrintGMResult(TEXT("GMSpawnFox requires a living player."), false);
		return;
	}
	const float SafeDistance = FMath::Clamp(Distance, 150.0f, 1000.0f);
	const FVector PlayerLocation = Player->GetActorLocation();
	FVector SpawnLocation = PlayerLocation + FVector(SafeDistance, 0.0f, 0.0f);
	if (ArenaScene)
	{
		const FVector2D ArenaCenter = ArenaScene->GetArenaCenter();
		FVector2D TowardCenter = ArenaCenter - FVector2D(PlayerLocation.X, PlayerLocation.Y);
		if (TowardCenter.IsNearlyZero())
		{
			TowardCenter = FVector2D(1.0f, 0.0f);
		}
		const FVector2D Desired =
		    FVector2D(PlayerLocation.X, PlayerLocation.Y) + TowardCenter.GetSafeNormal() * SafeDistance;
		const FVector2D HalfExtents = ArenaScene->GetEnemySpawnHalfExtents();
		SpawnLocation.X = FMath::Clamp(Desired.X, ArenaCenter.X - HalfExtents.X, ArenaCenter.X + HalfExtents.X);
		SpawnLocation.Y = FMath::Clamp(Desired.Y, ArenaCenter.Y - HalfExtents.Y, ArenaCenter.Y + HalfExtents.Y);
		SpawnLocation.Z = ArenaScene->GetGameplayPlaneWorldZ();
	}
	const bool bSpawned = SpawnConfiguredEnemy(TEXT("M_FOX"), SpawnLocation);
	PrintGMResult(bSpawned ? FString::Printf(TEXT("Spawned M_FOX %.0f cm from the player."),
	                                         FVector::Dist2D(PlayerLocation, SpawnLocation))
	                       : TEXT("Failed to spawn M_FOX from the production enemy definition."),
	              bSpawned);
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

	const int32 BossEncounterIndex = GetTotalEncounterCount();
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

void AReEchoGameMode::GMGrantCard(const FName CardId)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	if (CardId.IsNone())
	{
		PrintGMResult(TEXT("Usage: GMGrantCard <CardId> (e.g. G_2_17 for 静默刻度)."), false);
		return;
	}

	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		UE_LOG(LogReEcho, Warning, TEXT("[GMGrantCard] RunSubsystem unavailable for CardId=%s"), *CardId.ToString());
		PrintGMResult(TEXT("Run subsystem is unavailable."), false);
		return;
	}

	UE_LOG(LogReEcho, Warning, TEXT("[GMGrantCard] invoking DebugGrantCard for CardId=%s"), *CardId.ToString());

	const bool bGranted = RunSubsystem->DebugGrantCard(CardId);
	PrintGMResult(bGranted ? FString::Printf(TEXT("Granted card %s."), *CardId.ToString())
	                       : FString::Printf(TEXT("Failed to grant card %s (not found / conflict / locked run)."),
	                                         *CardId.ToString()),
	              bGranted);
}

void AReEchoGameMode::StartPlay()
{
	Super::StartPlay();
	if (UReEchoRuntimeAssetPreloader* Preloader =
	        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRuntimeAssetPreloader>() : nullptr)
	{
		Preloader->RequestPreload(FSimpleDelegate());
	}
	Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	int32 ArenaSceneCount = 0;
	for (TActorIterator<AReEchoArenaSceneActor> It(GetWorld()); It; ++It)
	{
		ArenaScene = *It;
		++ArenaSceneCount;
	}
	FString ArenaFailure;
	if (ArenaSceneCount != 1 || !ArenaScene || !ArenaScene->HasValidConfiguration(&ArenaFailure))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Expected one valid ArenaScene; found %d. %s"),
		       ArenaSceneCount,
		       *ArenaFailure);
		return;
	}
	const FVector2D PlayerHalfExtents = ArenaScene->GetPlayerHalfExtents();
	const FVector2D EnemySpawnHalfExtents = ArenaScene->GetEnemySpawnHalfExtents();
	ArenaSceneWorldHeight = EnemySpawnHalfExtents.X * 2.0f;
	ArenaSceneWorldWidth = EnemySpawnHalfExtents.Y * 2.0f;
	if (Player)
	{
		Player->ConfigureArenaBounds(ArenaScene->GetArenaCenter(), PlayerHalfExtents);
	}
	int32 ArenaCameraCount = 0;
	for (TActorIterator<AReEchoArenaCameraActor> It(GetWorld()); It; ++It)
	{
		ArenaCameraActor = *It;
		++ArenaCameraCount;
	}
	if (ArenaCameraCount != 1 || !ArenaCameraActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[ArenaCamera] Expected one ArenaCameraActor; found %d."), ArenaCameraCount);
		return;
	}
	ArenaCameraActor->Configure(Player, ArenaScene);
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetViewTarget(ArenaCameraActor);
		PostAudioEvent(FReEchoAudioEvents::CameraMove, ArenaCameraActor->GetActorLocation());
	}
	Director = GetWorld()->SpawnActor<AReEchoEncounterDirector>();
	Director->OnFixedStep.AddDynamic(this, &AReEchoGameMode::HandleFixedStep);
	Director->OnEncounterEnded.AddDynamic(this, &AReEchoGameMode::HandleEncounterEnded);
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		    GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
		WeatherWidget = UIFlow ? Cast<UReEchoWeatherWidget>(
		                             UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Weather, false, false))
		                       : nullptr;
		if (WeatherWidget)
		{
			WeatherWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			RefreshFogRevealSources();
		}
		EncounterHudWidget = UIFlow ? Cast<UReEchoEncounterHudWidget>(UIFlow->OpenScreen(
		                                  PlayerController, EReEchoUIScreen::EncounterHud, false, false))
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
			PlayerHudWidget = UIFlow ? Cast<UReEchoPlayerHudWidget>(UIFlow->OpenScreen(
			                               PlayerController, EReEchoUIScreen::PlayerHud, false, false))
			                         : nullptr;
			if (PlayerHudWidget)
			{
				PlayerHudWidget->InitializePlayerHud(
				    Player->Combatant, Player->CombatEvents, Player->GetPortraitTexture());
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	StartMenuWidget =
	    UIFlow
	        ? Cast<UReEchoStartMenuWidget>(UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::StartMenu, true, true))
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
	StartMenuWidget->OnAboutRequested.AddDynamic(this, &AReEchoGameMode::HandleStartAboutRequested);
	StartMenuWidget->OnQuitRequested.AddDynamic(this, &AReEchoGameMode::HandleStartQuitRequested);
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
	RequestBeginSelectedRun();
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

void AReEchoGameMode::HandleStartAboutRequested()
{
	if (!StartMenuWidget || AboutWidget)
	{
		return;
	}

	// Keep the start menu visible as the background behind the About panel (so it reads as
	// "on the main interface" rather than over the battle level). The About screen is already
	// a modal dialog (UIOnly input mode + focus lock in UReEchoUIFlowCoordinatorSubsystem), so
	// the start menu behind it receives no input. Do NOT call SetIsEnabled(false) here: the
	// start menu's RenderOpacity is bound to IsEnabled, and disabling it would turn the menu
	// transparent and let the battle scene show through as a ghost behind the About panel.
	ShowAboutScreen(true);
}

void AReEchoGameMode::HandleStartQuitRequested()
{
	UE_LOG(LogTemp, Display, TEXT("[ReEchoStartFlow] Start menu quit requested."));
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
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

void AReEchoGameMode::HandleAboutClosed()
{
	PostUiEvent(FReEchoAudioEvents::UiCancel);
	if (AboutWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::About);
		}
		AboutWidget = nullptr;
	}

	if (bAboutReturnToStartMenu && StartMenuWidget)
	{
		StartMenuWidget->SetVisibility(ESlateVisibility::Visible);
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->FocusScreen(UGameplayStatics::GetPlayerController(this, 0), EReEchoUIScreen::StartMenu, false);
		}
		StartMenuWidget->SetKeyboardFocus();
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
	bAboutReturnToStartMenu = false;
}

void AReEchoGameMode::ShowSettingsScreen(const bool bReturnToStartMenu)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || SettingsWidget)
	{
		return;
	}

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	SettingsWidget =
	    UIFlow
	        ? Cast<UReEchoSettingsWidget>(UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Settings, true, false))
	        : nullptr;
	if (!SettingsWidget)
	{
		return;
	}

	bSettingsReturnToStartMenu = bReturnToStartMenu;
	SettingsWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleSettingsClosed);
}

void AReEchoGameMode::ShowAboutScreen(const bool bReturnToStartMenu)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || AboutWidget)
	{
		return;
	}

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	AboutWidget =
	    UIFlow ? Cast<UReEchoAboutWidget>(UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::About, true, false))
	           : nullptr;
	if (!AboutWidget)
	{
		return;
	}

	bAboutReturnToStartMenu = bReturnToStartMenu;
	if (AboutWidget)
	{
		AboutWidget->OnClosed.AddDynamic(this, &AReEchoGameMode::HandleAboutClosed);
	}
}

void AReEchoGameMode::ShowLoadoutSelection()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || LoadoutSelectionWidget)
	{
		return;
	}
	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
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
	RequestBeginSelectedRun();
}

void AReEchoGameMode::RequestBeginSelectedRun()
{
	if (bBeginSelectedRunRequested || bBeginSelectedRunStarted)
	{
		return;
	}
	bBeginSelectedRunRequested = true;
	UReEchoRuntimeAssetPreloader* Preloader =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRuntimeAssetPreloader>() : nullptr;
	if (!Preloader)
	{
		UE_LOG(LogReEcho, Warning, TEXT("[ReEchoStartFlow] Runtime asset preloader unavailable; continuing."));
		HandleRuntimeAssetPreloadComplete();
		return;
	}
	if (!Preloader->IsPreloadComplete())
	{
		UE_LOG(LogReEcho, Display, TEXT("[ReEchoStartFlow] Waiting for first-encounter presentation assets."));
	}
	Preloader->RequestPreload(
	    FSimpleDelegate::CreateUObject(this, &AReEchoGameMode::HandleRuntimeAssetPreloadComplete));
}

void AReEchoGameMode::HandleRuntimeAssetPreloadComplete()
{
	if (!bBeginSelectedRunRequested || bBeginSelectedRunStarted)
	{
		return;
	}
	bBeginSelectedRunStarted = true;
	UReEchoRuntimeAssetPreloader* Preloader =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRuntimeAssetPreloader>() : nullptr;
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[ReEchoStartFlow] First-encounter asset gate released. Success=%s"),
	       Preloader && Preloader->DidPreloadSucceed() ? TEXT("true") : TEXT("false"));
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
	ClearEnemyRoster();
	ClearEchoes();
}

void AReEchoGameMode::ClearEnemyRoster()
{
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		if (AActor* EnemyHost = Entry.Host.Get())
		{
			EnemyHost->Destroy();
		}
	}
	EnemyRoster->ResetRoster();
	EnemySpawnIndex = 0;
}

void AReEchoGameMode::ClearEchoes()
{
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

bool AReEchoGameMode::ResolveNextStageTransition(FReEchoStageTransitionDecision& OutDecision, FString& OutError) const
{
	const UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	if (!RunSubsystem || !Snapshot.IsValid())
	{
		OutDecision = {};
		OutError = TEXT("Run data snapshot is unavailable.");
		return false;
	}
	return ReEchoStageTransition::Resolve(*Snapshot, RunSubsystem->EncounterIndex, OutDecision, OutError);
}

void AReEchoGameMode::SetEnemyEncounterSimulationSuspended(const bool bSuspended)
{
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		if (!Entry.bAlive)
		{
			continue;
		}
		if (AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Entry.Host.Get()))
		{
			Enemy->SetEncounterSimulationSuspended(bSuspended);
		}
	}
}

FVector AReEchoGameMode::ResolveStageEntryLocation() const
{
	if (!ArenaScene)
	{
		return Player ? Player->GetActorLocation() : FVector::ZeroVector;
	}
	const FVector2D Center = ArenaScene->GetArenaCenter();
	const float HalfHeight = Player && Player->Collision ? Player->Collision->GetScaledBoxExtent().Z : 0.0f;
	return FVector(Center.X, Center.Y, ArenaScene->GetGameplayPlaneWorldZ() + HalfHeight);
}

void AReEchoGameMode::PrepareEncounterIntermission()
{
	FReEchoStageTransitionDecision Transition;
	FString Error;
	if (!ResolveNextStageTransition(Transition, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[StageTransition] Intermission rejected: %s"), *Error);
		ClearCombatants();
		return;
	}

	ClearEchoes();
	if (Player && Player->Combatant)
	{
		Player->Combatant->ResetElementState();
	}
	if (Player && Player->Movement)
	{
		Player->Movement->StopMovementImmediately();
	}
	SetPlayerMenuAbilityBlocked(true);
	if (Transition.bPreserveEnemyRoster)
	{
		SetEnemyEncounterSimulationSuspended(true);
	}
	else
	{
		ClearEnemyRoster();
	}
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(
	    LogTemp,
	    Display,
	    TEXT("[StageTransition] intermission previous=%s next=%s sameStage=%s keepRoster=%s player=(%.1f,%.1f,%.1f)"),
	    *Transition.PreviousStageId.ToString(),
	    *Transition.NextStageId.ToString(),
	    Transition.bSameStage ? TEXT("true") : TEXT("false"),
	    Transition.bPreserveEnemyRoster ? TEXT("true") : TEXT("false"),
	    PlayerLocation.X,
	    PlayerLocation.Y,
	    PlayerLocation.Z);
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
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->EncounterIndex >= RunSubsystem->GetTotalEncounterCount())
	{
		return;
	}
	FReEchoStageTransitionDecision Transition;
	FString TransitionError;
	if (!ResolveNextStageTransition(Transition, TransitionError))
	{
		UE_LOG(LogTemp, Error, TEXT("[StageTransition] Next encounter rejected: %s"), *TransitionError);
		ClearCombatants();
		return;
	}
	if (!Transition.bPreserveEnemyRoster)
	{
		ClearEnemyRoster();
	}
	ClearEchoes();
	bEncounterTransitioning = false;
	bEncounterClearedByDefeat = false;
	bBossPostEchoPhaseTriggered = false;
	RunSubsystem->BeginEncounter();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
	if (!Encounter || !ConfigureEncounterSpawns(RunSubsystem->EncounterIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter %d could not start from table data."), RunSubsystem->EncounterIndex);
		return;
	}
	Director->ConfigureEncounter(Encounter->DurationSeconds, Encounter->EndCondition == TEXT("Duration"));
	SetMusicState(IsBossEncounter() ? FReEchoAudioEvents::MusicBoss : FReEchoAudioEvents::MusicEncounter);
	UpdateWeatherScene(RunSubsystem->EncounterIndex);
	if (Player)
	{
		if (!Transition.bPreservePlayerLocation)
		{
			Player->SetActorLocation(ResolveStageEntryLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (Player->Movement)
		{
			Player->Movement->StopMovementImmediately();
		}
		Player->ConfigureCharacter(RunSubsystem->CurrentBuild.CharacterId);
		Player->RestoreEquippedWeapon(RunSubsystem->CurrentBuild.WeaponId);
		Player->SetAutoAttackMode(RunSubsystem->IsAutomaticAttackMode());
		const FReEchoStatBlock& Stats = RunSubsystem->CurrentBuild.Stats;
		const bool bFillHealthOnEnter = !Transition.bSameStage;
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[StageTransition] enter encounter %d sameStage=%s fillHealth=%s"),
		       RunSubsystem->EncounterIndex,
		       Transition.bSameStage ? TEXT("true") : TEXT("false"),
		       bFillHealthOnEnter ? TEXT("true") : TEXT("false"));
		Player->Combatant->InitializeFromStats(Stats, bFillHealthOnEnter);
		Player->Movement->MaxSpeed = 420.0f * Stats.MovementSpeed;
		if (PlayerHudWidget)
		{
			PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CombatEvents, Player->GetPortraitTexture());
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
			Echo->ConfigureCardRules(RunSubsystem->GetCardRules(), RunSubsystem->CurrentBuild.Stats);
			Echoes.Add(Echo);
		}
		else if (Echo)
		{
			Echo->Destroy();
		}
	}
	RefreshFogRevealSources();
	SetEnemyEncounterSimulationSuspended(false);
	RestoreGameInput();
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[StageTransition] started previous=%s next=%s keepRoster=%s keepPlayerLocation=%s roster=%d "
	            "player=(%.1f,%.1f,%.1f)"),
	       *Transition.PreviousStageId.ToString(),
	       *Transition.NextStageId.ToString(),
	       Transition.bPreserveEnemyRoster ? TEXT("true") : TEXT("false"),
	       Transition.bPreservePlayerLocation ? TEXT("true") : TEXT("false"),
	       EnemyRoster->GetLivingEnemyCount(),
	       PlayerLocation.X,
	       PlayerLocation.Y,
	       PlayerLocation.Z);
	Director->StartEncounter();
	ProcessScheduledSpawnEvents(0.0f);
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
	Result.NextScheduledSpawnEventIndex = EncounterWaveScheduler.GetNextEventIndex();
	Result.PendingSpawnBatches = PendingSpawnBatches;
	Result.SpawnResolveSequence = EncounterSpawnSequence;
	Result.ReservedSpawnLocations = EncounterSpawnLocations;
	const TSharedPtr<const FReEchoCsvDataSnapshot> DataSnapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* Encounter =
	    DataSnapshot.IsValid() ? DataSnapshot->FindEncounter(CurrentEncounterId) : nullptr;
	const float WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Encounter)
	{
		for (const float StartedAt : RecentRangedBurstWorldTimes)
		{
			const float Remaining = Encounter->RangedBurstWindowSeconds - (WorldTimeSeconds - StartedAt);
			if (Remaining > 0.0f)
			{
				Result.RangedBurstWindowRemainingSeconds.Add(Remaining);
			}
		}
	}
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
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
	if (!Encounter || !ConfigureEncounterSpawns(RunSubsystem->EncounterIndex))
	{
		UE_LOG(
		    LogTemp, Error, TEXT("Saved encounter %d has no valid table configuration."), RunSubsystem->EncounterIndex);
		return;
	}
	Director->ConfigureEncounter(Encounter->DurationSeconds, Encounter->EndCondition == TEXT("Duration"));
	EncounterWaveScheduler.RestoreNextEventIndex(SavedState.NextScheduledSpawnEventIndex);
	PendingSpawnBatches = SavedState.PendingSpawnBatches;
	EncounterSpawnSequence = FMath::Max(0, SavedState.SpawnResolveSequence);
	EncounterSpawnLocations = SavedState.ReservedSpawnLocations;
	RecentRangedBurstWorldTimes.Reset();
	const float ResumeWorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	for (const float Remaining : SavedState.RangedBurstWindowRemainingSeconds)
	{
		if (Remaining > 0.0f && Remaining <= Encounter->RangedBurstWindowSeconds)
		{
			RecentRangedBurstWorldTimes.Add(ResumeWorldTimeSeconds - (Encounter->RangedBurstWindowSeconds - Remaining));
		}
	}
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
		PlayerHudWidget->InitializePlayerHud(Player->Combatant, Player->CombatEvents, Player->GetPortraitTexture());
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
				if (Echo->InitializeEcho(
				        Recording, RunSubsystem->CurrentBuild.Stats.EchoEfficiency, RunSubsystem->GetRunDataSnapshot()))
				{
					Echo->ConfigureCardRules(RunSubsystem->GetCardRules(), RunSubsystem->CurrentBuild.Stats);
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
		EnemySpawnIndex = FMath::Max(EnemySpawnIndex, EnemyState.SpawnIndex);
		const EReEchoEnemyKind SavedKind = EnemyState.Kind <= static_cast<uint8>(EReEchoEnemyKind::Elite)
		                                       ? static_cast<EReEchoEnemyKind>(EnemyState.Kind)
		                                       : EReEchoEnemyKind::Grunt;
		const FName EnemyId = !EnemyState.EnemyId.IsNone()            ? EnemyState.EnemyId
		                      : SavedKind == EReEchoEnemyKind::Boss   ? FName(TEXT("M_TimeGuard"))
		                      : SavedKind == EReEchoEnemyKind::Slime  ? FName(TEXT("M_SLIME"))
		                      : SavedKind == EReEchoEnemyKind::Ranged ? FName(TEXT("M_RABBIT"))
		                      : SavedKind == EReEchoEnemyKind::Elite  ? FName(TEXT("M_FOX"))
		                      : SavedKind == EReEchoEnemyKind::Bomber ? FName(TEXT("M_Bomber"))
		                      : SavedKind == EReEchoEnemyKind::Shield ? FName(TEXT("M_Shield"))
		                                                              : FName(TEXT("M_Grunt"));
		FReEchoEnemyDefinition Definition;
		FString CompileError;
		if (!DataSnapshot || !ReEchoEnemyDefinitionCompiler::Compile(*DataSnapshot, EnemyId, Definition, CompileError))
		{
			UE_LOG(LogTemp, Error, TEXT("Plan48 enemy restore failed: %s"), *CompileError);
			continue;
		}
		AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>(
		    ResolveEnemyClass(Definition.PresentationId), EnemyState.Transform.GetLocation(), FRotator::ZeroRotator);
		if (!Enemy)
		{
			continue;
		}
		Enemy->SetPresentationCatalog(PresentationCatalog);
		if (ArenaScene)
		{
			Enemy->ConfigureGameplayPlane(ArenaScene->GetGameplayPlaneWorldZ());
		}
		if (!Enemy->ConfigureFromDefinition(Definition, EnemyState.SpawnIndex))
		{
			Enemy->Destroy();
			continue;
		}
		Enemy->RestoreRuntimeState(EnemyState);
		Enemy->SetEnemyId(EnemyId);
		Enemy->SetEnemyRoster(EnemyRoster);
		if (UReEchoEnemyEventsComponent* Events = Enemy->GetEnemyEventsComponent())
		{
			Events->OnBossIntent.AddUniqueDynamic(this, &AReEchoGameMode::HandleBossIntent);
		}
	}
	Director->ResumeEncounter(SavedState.EncounterTime);
}

bool AReEchoGameMode::ConfigureEncounterSpawns(const int32 EncounterIndex)
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(EncounterIndex) : nullptr;
	if (!Encounter)
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter %d has no enabled table definition."), EncounterIndex);
		return false;
	}
	CurrentEncounterId = Encounter->Id;
	EncounterSpawnSequence = 0;
	EncounterSpawnLocations.Reset();
	RecentRangedBurstWorldTimes.Reset();
	PendingSpawnBatches.Reset();
	FString Error;
	if (!EncounterWaveScheduler.Configure(*Snapshot, CurrentEncounterId, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter wave setup failed: %s"), *Error);
		return false;
	}
	return true;
}

bool AReEchoGameMode::CanStartEnemySpecial(const FName EnemyId, const int32 SpawnIndex, const float WorldTimeSeconds)
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounter(CurrentEncounterId) : nullptr;
	const FReEchoCsvEnemyRow* Enemy = Snapshot.IsValid() ? Snapshot->FindEnemy(EnemyId) : nullptr;
	if (!Encounter || !Enemy)
	{
		return false;
	}

	if (Enemy->Archetype == TEXT("Ranged"))
	{
		RecentRangedBurstWorldTimes.RemoveAll(
		    [WorldTimeSeconds, Encounter](const float StartedAt)
		    {
			    return WorldTimeSeconds - StartedAt >= Encounter->RangedBurstWindowSeconds;
		    });
		return RecentRangedBurstWorldTimes.Num() < Encounter->RangedBurstLimit;
	}
	if (Enemy->Archetype == TEXT("Elite"))
	{
		int32 ActiveEliteSkills = 0;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (!Entry.bAlive || Entry.SpawnIndex == SpawnIndex || Entry.Archetype != EReEchoEnemyArchetype::Elite)
			{
				continue;
			}
			const AReEchoEnemyActor* OtherEnemy = Cast<AReEchoEnemyActor>(Entry.Host.Get());
			const UReEchoEnemyLogicComponent* OtherLogic = OtherEnemy ? OtherEnemy->GetEnemyLogicComponent() : nullptr;
			if (OtherLogic && OtherLogic->GetSnapshot().SpecialActionPhase != EReEchoEnemySpecialActionPhase::None)
			{
				++ActiveEliteSkills;
			}
		}
		return ActiveEliteSkills < Encounter->EliteSkillConcurrency;
	}
	return true;
}

void AReEchoGameMode::NotifyEnemySpecialStarted(const FName EnemyId,
                                                const int32 SpawnIndex,
                                                const float WorldTimeSeconds)
{
	(void)SpawnIndex;
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEnemyRow* Enemy = Snapshot.IsValid() ? Snapshot->FindEnemy(EnemyId) : nullptr;
	if (Enemy && Enemy->Archetype == TEXT("Ranged"))
	{
		RecentRangedBurstWorldTimes.Add(WorldTimeSeconds);
	}
}

void AReEchoGameMode::ProcessScheduledSpawnEvents(const float EncounterSeconds)
{
	for (const FReEchoScheduledSpawnEvent& Event : EncounterWaveScheduler.AdvanceTo(EncounterSeconds))
	{
		if (Event.Type == EReEchoScheduledSpawnEventType::Warning)
		{
			PrepareScheduledSpawnBatch(Event);
			UE_LOG(LogTemp,
			       Display,
			       TEXT("[EncounterSpawn] warning wave=%s role=%s count=%d spawn=%.2f"),
			       *Event.WaveId.ToString(),
			       *Event.EnemyRole.ToString(),
			       Event.Count,
			       Event.SpawnSeconds);
			const FReEchoPendingSpawnBatchState* Pending = PendingSpawnBatches.FindByPredicate(
			    [&Event](const FReEchoPendingSpawnBatchState& Candidate)
			    {
				    return Candidate.WaveId == Event.WaveId && Candidate.EnemyRole == Event.EnemyRole;
			    });
			if (Pending && GetWorld())
			{
				const float DisplaySeconds = FMath::Max(0.15f, Event.SpawnSeconds - Event.EventSeconds);
				for (const FVector& Location : Pending->Locations)
				{
					DrawDebugSphere(GetWorld(), Location, 65.0f, 12, FColor::Orange, false, DisplaySeconds, 0, 5.0f);
				}
			}
			continue;
		}
		SpawnScheduledBatch(Event);
	}
}

void AReEchoGameMode::PrepareScheduledSpawnBatch(const FReEchoScheduledSpawnEvent& Event)
{
	if (Event.EnemyRole == TEXT("Boss") || PendingSpawnBatches.ContainsByPredicate(
	                                           [&Event](const FReEchoPendingSpawnBatchState& Candidate)
	                                           {
		                                           return Candidate.WaveId == Event.WaveId &&
		                                                  Candidate.EnemyRole == Event.EnemyRole;
	                                           }))
	{
		return;
	}

	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounter(CurrentEncounterId) : nullptr;
	const FReEchoCsvSpawnPolicyRow* Policy = Snapshot.IsValid() ? Snapshot->FindEnabledSpawnPolicy() : nullptr;
	const FReEchoCsvSpawnProfileRow* Profile =
	    Snapshot.IsValid() ? Snapshot->FindSpawnProfileByRole(Event.EnemyRole) : nullptr;
	if (!Snapshot.IsValid() || !Encounter || !Policy || !Profile || !Player)
	{
		UE_LOG(LogTemp, Error, TEXT("[EncounterSpawn] warning rejected because runtime data is unavailable."));
		return;
	}

	FReEchoPendingSpawnBatchState Pending;
	Pending.WaveId = Event.WaveId;
	Pending.EnemyRole = Event.EnemyRole;
	Pending.EnemyId = Event.EnemyId;
	for (int32 Index = 0; Index < Event.Count; ++Index)
	{
		FReEchoSpawnResolveRequest Request;
		Request.PlayerAnchor = Player->GetActorLocation() + Player->GetVelocity() * Policy->AnchorLeadSeconds;
		Request.PlayerAnchor.X =
		    FMath::Clamp(Request.PlayerAnchor.X, -ArenaSceneWorldHeight * 0.5f, ArenaSceneWorldHeight * 0.5f);
		Request.PlayerAnchor.Y =
		    FMath::Clamp(Request.PlayerAnchor.Y, -ArenaSceneWorldWidth * 0.5f, ArenaSceneWorldWidth * 0.5f);
		Request.bHasEchoAnchor = Echoes.Num() > 0 && IsValid(Echoes[0]);
		Request.EchoAnchor = Request.bHasEchoAnchor
		                         ? Echoes[0]->EvaluateRecordedPosition(Event.SpawnSeconds + Policy->AnchorLeadSeconds)
		                         : FVector::ZeroVector;
		Request.EchoAnchorRatio = Encounter->EchoAnchorRatio;
		Request.ArenaHalfX = FMath::Max(100.0f, ArenaSceneWorldHeight * 0.5f);
		Request.ArenaHalfY = FMath::Max(100.0f, ArenaSceneWorldWidth * 0.5f);
		Request.Seed = 1337 + Encounter->EncounterIndex * 7919;
		Request.Sequence = EncounterSpawnSequence++;
		Request.ExistingLocations = EncounterSpawnLocations;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (Entry.bAlive && Entry.Host.IsValid())
			{
				Request.ExistingLocations.Add(Entry.Host->GetActorLocation());
			}
		}

		FReEchoResolvedSpawn Resolved;
		FString Error;
		if (!FReEchoSpawnResolver::Resolve(*Profile, *Policy, Request, Resolved, Error))
		{
			UE_LOG(LogTemp,
			       Warning,
			       TEXT("[EncounterSpawn] warning wave=%s role=%s index=%d rejected: %s"),
			       *Event.WaveId.ToString(),
			       *Event.EnemyRole.ToString(),
			       Index,
			       *Error);
			continue;
		}
		Pending.Locations.Add(Resolved.Location);
		EncounterSpawnLocations.Add(Resolved.Location);
	}
	PendingSpawnBatches.Add(MoveTemp(Pending));
}

void AReEchoGameMode::SpawnScheduledBatch(const FReEchoScheduledSpawnEvent& Event)
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounter(CurrentEncounterId) : nullptr;
	if (!Snapshot.IsValid() || !Encounter || !Player)
	{
		UE_LOG(LogTemp, Error, TEXT("[EncounterSpawn] commit rejected because runtime data is unavailable."));
		return;
	}

	if (Event.EnemyRole == TEXT("Boss"))
	{
		SpawnConfiguredEnemy(Event.EnemyId, FVector(800.0f, 0.0f, 50.0f));
		return;
	}
	PrepareScheduledSpawnBatch(Event);
	const int32 PendingIndex = PendingSpawnBatches.IndexOfByPredicate(
	    [&Event](const FReEchoPendingSpawnBatchState& Candidate)
	    {
		    return Candidate.WaveId == Event.WaveId && Candidate.EnemyRole == Event.EnemyRole;
	    });
	if (!PendingSpawnBatches.IsValidIndex(PendingIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[EncounterSpawn] wave %s has no prepared batch."), *Event.WaveId.ToString());
		return;
	}
	const FReEchoPendingSpawnBatchState Pending = PendingSpawnBatches[PendingIndex];

	int32 LivingCount = 0;
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		LivingCount +=
		    Entry.bAlive && (Encounter->bBossCountsTowardUnitLimit || Entry.Archetype != EReEchoEnemyArchetype::Boss)
		        ? 1
		        : 0;
	}
	const int32 AllowedCount = FMath::Clamp(Encounter->ActiveUnitLimit - LivingCount, 0, Pending.Locations.Num());
	if (AllowedCount < Pending.Locations.Num())
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[EncounterSpawn] wave=%s role=%s truncated %d->%d by active unit limit %d."),
		       *Event.WaveId.ToString(),
		       *Event.EnemyRole.ToString(),
		       Pending.Locations.Num(),
		       AllowedCount,
		       Encounter->ActiveUnitLimit);
	}

	for (int32 Index = 0; Index < AllowedCount; ++Index)
	{
		SpawnConfiguredEnemy(Pending.EnemyId, Pending.Locations[Index]);
	}
	PendingSpawnBatches.RemoveAt(PendingIndex);
}

bool AReEchoGameMode::SpawnConfiguredEnemy(const FName EnemyId, const FVector& SpawnLocation)
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	FReEchoEnemyDefinition Definition;
	FString CompileError;
	if (!Snapshot.IsValid() || !ReEchoEnemyDefinitionCompiler::Compile(*Snapshot, EnemyId, Definition, CompileError))
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy spawn failed for %s: %s"), *EnemyId.ToString(), *CompileError);
		return false;
	}
	const int32 NextSpawnIndex = EnemySpawnIndex + 1;
	AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>(
	    ResolveEnemyClass(Definition.PresentationId), SpawnLocation, FRotator::ZeroRotator);
	if (Enemy)
	{
		Enemy->SetPresentationCatalog(PresentationCatalog);
		EnemySpawnIndex = NextSpawnIndex;
		if (ArenaScene)
		{
			Enemy->ConfigureGameplayPlane(ArenaScene->GetGameplayPlaneWorldZ());
		}
	}
	if (!Enemy || !Enemy->ConfigureFromDefinition(Definition, NextSpawnIndex))
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
		return false;
	}
	Enemy->SetEnemyRoster(EnemyRoster);
	Enemy->SetEnemyId(EnemyId);
	if (UReEchoEnemyEventsComponent* Events = Enemy->GetEnemyEventsComponent())
	{
		Events->OnBossIntent.AddUniqueDynamic(this, &AReEchoGameMode::HandleBossIntent);
	}
	return true;
}

int32 AReEchoGameMode::GetTotalEncounterCount() const
{
	const UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	return RunSubsystem ? RunSubsystem->GetTotalEncounterCount()
	                    : GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
}

bool AReEchoGameMode::IsBossEncounter() const
{
	const UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() && RunSubsystem ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
	return Encounter && Encounter->EndCondition == TEXT("BossOrPlayerDeath");
}

void AReEchoGameMode::TriggerBossPostEchoPhase(const FReEchoBossPhaseDefinition& PhaseDefinition)
{
	if (bBossPostEchoPhaseTriggered || !IsBossEncounter() || !PhaseDefinition.bEnabled || !Player || !Player->Combatant)
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
	ProcessScheduledSpawnEvents(Director->EncounterTime);
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->AdvanceEcho(Director->EncounterTime);
		}
	}
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const FReEchoCardEncounterTickResult CardTick = RunSubsystem->AdvanceCardEncounter(Director->EncounterTime);
	const FReEchoCardRuleSnapshot Rules = RunSubsystem->GetCardRules();
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr;
		if (!Enemy)
		{
			continue;
		}
		for (const float StunDuration : CardTick.EnemyStunDurations)
		{
			Enemy->ApplyCardStun(StunDuration);
		}
		if (Rules.EnemyElementImmunitySeconds >= 0.0f)
		{
			Enemy->GetCombatantComponent()->ClampElementImmunityDuration(
			    GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f, Rules.EnemyElementImmunitySeconds);
		}
		bool bInsideSlowAura = false;
		for (AReEchoEchoActor* Echo : Echoes)
		{
			if (Echo && Echo->IsCombatTargetAlive() &&
			    FVector::DistSquared2D(Echo->GetActorLocation(), Enemy->GetActorLocation()) <= FMath::Square(400.0f))
			{
				bInsideSlowAura = true;
				if (CardTick.EchoAuraPulseCount > 0 && (Rules.bWaterEchoAura || Rules.bGrassEchoAura))
				{
					FReEchoElementHitContext Context;
					Context.Attack.Source = Echo;
					Context.Attack.Sequence = CardTick.CardState.Runtime.LastEchoAuraPulseIndex;
					Context.SourceLocation = Echo->GetActorLocation();
					Context.ReactionEfficiency = RunSubsystem->CurrentBuild.Stats.ReactionEfficiency;
					Context.SourceElementalAttack = 0.0f;
					if (Rules.bWaterEchoAura)
					{
						ReEchoHitResolver::ResolveElementHit(*Enemy, EReEchoElement::Water, 0.0f, Context);
					}
					if (Rules.bGrassEchoAura)
					{
						ReEchoHitResolver::ResolveElementHit(*Enemy, EReEchoElement::Grass, 0.0f, Context);
					}
				}
			}
		}
		Enemy->SetCardMovementMultiplier(bInsideSlowAura ? 1.0f - Rules.EchoSlowAura : 1.0f);
	}
	if (RunSubsystem->ConsumeCardEchoRemovalRequest())
	{
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	RestartWidget =
	    UIFlow ? Cast<UReEchoRestartWidget>(UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Restart, false, true))
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
	RestartWidget->OnExitToMainMenuRequested.AddDynamic(this, &AReEchoGameMode::HandleExitToMainMenuRequested);
	RestartWidget->OnExitWithoutSavingRequested.AddDynamic(this, &AReEchoGameMode::HandleExitWithoutSavingRequested);
	RestartWidget->OnCancelExitRequested.AddDynamic(this, &AReEchoGameMode::HandleCancelExitRequested);
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
	if (bRestartScreenIsTerminal)
	{
		return;
	}
	if (RestartWidget)
	{
		if (bQuitConfirmationVisible)
		{
			HandleCancelExitRequested();
		}
		else
		{
			HandleResumeRequested();
		}
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	StatsWidget =
	    UIFlow ? Cast<UReEchoStatsWidget>(UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::Stats, false, true))
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
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] GameMode: ToggleInventoryMenu called; InventoryShopWidget=%s"),
	       InventoryShopWidget ? TEXT("open") : TEXT("closed"));
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	ShowInventoryShopMenu(EReEchoInventoryShopMode::Inventory);
}

void AReEchoGameMode::ToggleShopMenu()
{
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] GameMode: ToggleShopMenu called; InventoryShopWidget=%s"),
	       InventoryShopWidget ? TEXT("open") : TEXT("closed"));
	if (InventoryShopWidget)
	{
		HandleInventoryShopClosed();
		return;
	}
	ShowInventoryShopMenu(EReEchoInventoryShopMode::ManualShop);
}

void AReEchoGameMode::ShowInventoryShopMenu(const EReEchoInventoryShopMode Mode)
{
	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] ShowInventoryShopMenu entered Mode=%d; guard(Stats=%d Trait=%d Restart=%d Terminal=%d)"),
	       (int32)Mode,
	       StatsWidget ? 1 : 0,
	       TraitCardChoiceWidget ? 1 : 0,
	       RestartWidget ? 1 : 0,
	       bRestartScreenIsTerminal ? 1 : 0);
	if (StatsWidget || TraitCardChoiceWidget || RestartWidget || bRestartScreenIsTerminal)
	{
		UE_LOG(LogReEcho, Warning, TEXT("[AttrPanel] ShowInventoryShopMenu guard TRIPPED - early return"));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!PlayerController || !RunSubsystem)
	{
		return;
	}

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	InventoryShopWidget = UIFlow ? Cast<UReEchoInventoryShopWidget>(UIFlow->OpenScreen(
	                                   PlayerController, EReEchoUIScreen::InventoryShop, false, true))
	                             : nullptr;
	if (!InventoryShopWidget)
	{
		UE_LOG(
		    LogReEcho, Warning, TEXT("[AttrPanel] InventoryShopWidget invalid (OpenScreen returned non-shop or null)"));
		return;
	}

	UE_LOG(LogReEcho,
	       Log,
	       TEXT("[AttrPanel] InventoryShopWidget valid; Player=%s Combatant=%s"),
	       Player ? TEXT("valid") : TEXT("null"),
	       (Player && Player->Combatant) ? TEXT("valid") : TEXT("null"));
	if (Player && Player->Combatant)
	{
		// 展示"下一场将带着的属性"：购买改属性卡后立即反映，而非上一场的 Combatant->Stats。
		InventoryShopWidget->SetPlayerStats(RunSubsystem ? RunSubsystem->CurrentBuild.Stats : Player->Combatant->Stats);
	}

	InventoryShopWidget->OnClosed.AddUObject(this, &AReEchoGameMode::HandleInventoryShopClosed);
	InventoryShopWidget->OnPurchaseRequested.AddUObject(this, &AReEchoGameMode::HandleShopPurchaseRequested);
	InventoryShopWidget->OnRefreshRequested.AddUObject(this, &AReEchoGameMode::HandleShopRefreshRequested);
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission)
	{
		bPostTraitShopClosing = false;
		InventoryShopWidget->OnEchoStoreRequested.AddUObject(this, &AReEchoGameMode::HandleEchoStoreRequested);
		InventoryShopWidget->OnEchoSkipRequested.AddUObject(this, &AReEchoGameMode::HandleEchoSkipRequested);
		InventoryShopWidget->OnEchoReplaceRequested.AddUObject(this, &AReEchoGameMode::HandleEchoReplaceRequested);
		InventoryShopWidget->OnEchoSelectionRequested.AddUObject(this, &AReEchoGameMode::HandleEchoSelectionRequested);
		InventoryShopWidget->OnEchoSkipAndCloseRequested.AddUObject(this,
		                                                            &AReEchoGameMode::HandleEchoSkipAndCloseRequested);
		RefreshShopPresentation(RunSubsystem, Mode);
	}
	else if (Mode == EReEchoInventoryShopMode::ManualShop)
	{
		RefreshShopPresentation(RunSubsystem, Mode);
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
		InventoryShopWidget->ShowEchoStatus(NSLOCTEXT(
		    "ReEcho", "ResolveEchoBeforeClosing", "Store this echo or explicitly skip it before continuing."));
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
	if (bShouldStartNextEncounter)
	{
		ResumeWorldForMenuTransition();
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
	}
	else
	{
		RestoreGameInput();
		if (bManualShop)
		{
			RestoreEncounterAudioState();
		}
	}
}

void AReEchoGameMode::HandleShopPurchaseRequested(const FName ItemId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	// 背包重装：点击“拥有但未装备”的配件 → 直接重新装备，不扣钱、不走购买防重复。
	if (RunSubsystem->OwnedPartIds.Contains(ItemId))
	{
		FString EquipError;
		if (!RunSubsystem->TryEquipPurchasedPart(ItemId, EquipError))
		{
			UE_LOG(LogTemp,
			       Warning,
			       TEXT("[ReEchoShop] Owned part '%s' could not be re-equipped: %s"),
			       *ItemId.ToString(),
			       *EquipError);
			PostUiEvent(FReEchoAudioEvents::UiError);
			return;
		}
		RunSubsystem->SaveRun();
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
		return;
	}

	// 新购：走完整购买流程（防重复/扣钱/入背包），购买即装备。
	if (RunSubsystem->PurchaseShopItem(ItemId))
	{
		// 购买即装备：武器配件报价的 ItemId 与 PartId 同名，购买后立即装入对应槽位。
		if (RunSubsystem->OwnedPartIds.Contains(ItemId))
		{
			FString EquipError;
			if (!RunSubsystem->TryEquipPurchasedPart(ItemId, EquipError))
			{
				UE_LOG(LogTemp,
				       Warning,
				       TEXT("[ReEchoShop] Purchased part '%s' could not be equipped: %s"),
				       *ItemId.ToString(),
				       *EquipError);
			}
		}
		PostUiEvent(FReEchoAudioEvents::UiPurchase);
		RunSubsystem->SaveRun();
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
	}
	else
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
	}
}

void AReEchoGameMode::HandleShopRefreshRequested()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget || !RunSubsystem->TryConsumeShopRefresh(ReEchoShopRefreshPrice))
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	PostUiEvent(FReEchoAudioEvents::UiPurchase);
	RunSubsystem->SaveRun();
	RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
}

void AReEchoGameMode::RefreshShopPresentation(UReEchoRunSubsystem* RunSubsystem, const EReEchoInventoryShopMode Mode)
{
	if (!RunSubsystem || !InventoryShopWidget)
	{
		return;
	}

	const FReEchoCardRuleSnapshot Rules = RunSubsystem->GetCardRules();
	const FReEchoCardRuntimeState& Runtime = RunSubsystem->CurrentBuild.CardState.Runtime;
	InventoryShopWidget->SetWeaponPartShopView(RunSubsystem->GetWeaponPartShopView());
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission)
	{
		InventoryShopWidget->ShowPostTraitIntermission(RunSubsystem->TimeShards,
		                                               RunSubsystem->InventoryItems,
		                                               RunSubsystem->GetEchoStorageSummary(),
		                                               Rules.ShopDiscount,
		                                               Runtime.FreeShopRefreshes,
		                                               !Rules.bDisableShopRefresh,
		                                               !Rules.bDisableExtraCardPurchase,
		                                               Runtime.ShopRefreshSequence);
	}
	else
	{
		InventoryShopWidget->ShowShop(RunSubsystem->TimeShards,
		                              RunSubsystem->InventoryItems,
		                              Rules.ShopDiscount,
		                              Runtime.FreeShopRefreshes,
		                              !Rules.bDisableShopRefresh,
		                              !Rules.bDisableExtraCardPurchase,
		                              Runtime.ShopRefreshSequence);
	}
	// #8-A: 商店每次刷新(打开/购买/刷新)都重设主角属性面板，并读取 CurrentBuild.Stats
	// 以反映已购属性卡，而非上一场的 Combatant->Stats。
	InventoryShopWidget->SetPlayerStats(RunSubsystem->CurrentBuild.Stats);
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
	if (!RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(FName(ReEchoEchoStorage::StorageUnlockCardId)))
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "EchoStorageCardRequired", "需要先获得“时空锚点”才能存储回响。"));
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
	if (!RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(FName(ReEchoEchoStorage::StorageUnlockCardId)))
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "EchoReplaceCardRequired", "需要先获得“时空锚点”才能替换回响。"));
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
	if (!RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(FName(ReEchoEchoStorage::StorageUnlockCardId)))
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "EchoSelectionCardRequired", "需要先获得“时空锚点”才能选择存储回响。"));
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->SetSelectedReplayIds(RecordingIds);
	if (Result == EReEchoEchoStorageResult::Success)
	{
		if (RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_3_02")))
		{
			if (RecordingIds.Num() == 1)
			{
				RunSubsystem->SetCardAnchorRecording(RecordingIds[0]);
			}
			else
			{
				RunSubsystem->ClearCardAnchorRecording();
			}
		}
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoSelectionSaved", "Replay selection saved.")
		           : NSLOCTEXT(
		                 "ReEcho", "EchoSelectionSaveFailed", "Selection changed in this session, but saving failed."));
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
		InventoryShopWidget->ShowEchoStatus(NSLOCTEXT(
		    "ReEcho", "EchoCloseSaveFailed", "The echo was skipped, but saving failed. The shop remains open."));
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
	bExitToMainMenuAfterConfirmation = false;
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
	if (TraitCardChoiceWidget)
	{
		SetPlayerMenuAbilityBlocked(true);
	}
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
		bExitToMainMenuAfterConfirmation = false;
		if (RestartWidget)
		{
			RestartWidget->SetQuitConfirmation(true, false);
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
	CompletePauseExit();
}

void AReEchoGameMode::HandleExitToMainMenuRequested()
{
	if (bRestartScreenIsTerminal || bQuitConfirmationVisible)
	{
		return;
	}
	bQuitConfirmationVisible = true;
	bExitToMainMenuAfterConfirmation = true;
	if (RestartWidget)
	{
		RestartWidget->SetQuitConfirmation(true, true);
	}
}

void AReEchoGameMode::HandleExitWithoutSavingRequested()
{
	if (!bQuitConfirmationVisible || bRestartScreenIsTerminal)
	{
		return;
	}
	CompletePauseExit();
}

void AReEchoGameMode::HandleCancelExitRequested()
{
	if (!bQuitConfirmationVisible || bRestartScreenIsTerminal)
	{
		return;
	}
	bQuitConfirmationVisible = false;
	bExitToMainMenuAfterConfirmation = false;
	if (RestartWidget)
	{
		RestartWidget->SetQuitConfirmation(false);
	}
}

void AReEchoGameMode::CompletePauseExit()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!bExitToMainMenuAfterConfirmation)
	{
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
		return;
	}

	bQuitConfirmationVisible = false;
	bExitToMainMenuAfterConfirmation = false;
	UGameplayStatics::SetGamePaused(this, false);
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void AReEchoGameMode::HandleRestartRequested()
{
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
	bRestartScreenIsDeath = false;

	// #12 死亡/胜利后"重新开始"不再重载关卡，改为就地清理并进入新游戏流程
	// （角色/武器选择界面），避免重载后 StartPlay 无条件弹出主菜单（与 #11 同类回归）。
	// 后续由 BeginNextEncounter 在开局时完整复位玩家与战场。
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (RunSubsystem)
	{
		RunSubsystem->DeleteSavedRun();
	}
	// 复位会阻挡再次开局的标志位（首次开局时已置为 true）。
	bBeginSelectedRunRequested = false;
	bBeginSelectedRunStarted = false;
	// 清除当前战场残存（敌人 / Echo），开局时会重新生成。
	ClearCombatants();
	UGameplayStatics::SetGamePaused(this, false);
	ShowLoadoutSelection();
}

void AReEchoGameMode::HandleEncounterEnded()
{
	if (bEncounterTransitioning || !Player)
	{
		return;
	}
	bEncounterTransitioning = true;
	// #9 修正：倒计时必须显示到 0 才结束本局。先把 HUD 强制刷成剩余 0 秒（让玩家看到"0"，
	// 而非冻结在上一个"1"帧），再用一个短暂 settle 节拍让"0"可见，最后收起 HUD 并弹出选卡/结算。
	UReEchoRunSubsystem* RunSubsystemForHud = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (EncounterHudWidget)
	{
		EncounterHudWidget->SetEncounterStatus(
		    RunSubsystemForHud ? RunSubsystemForHud->EncounterIndex : 0, GetTotalEncounterCount(), 0.0f);
	}
	// [EncounterEnded] 选卡/结算入口：记录此刻真实剩余时间，与上面的 [EncounterTimer][END] 对照。
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[EncounterEnded] enter -> card/shop/victory; EncounterTime=%.3f Remaining=%.3f EncounterIndex=%d"),
	       Director ? Director->EncounterTime : -1.0f,
	       Director ? Director->GetRemainingTime() : -1.0f,
	       GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>()
	           ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>()->EncounterIndex
	           : -1);
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const FReEchoRecording Recording = Player->Recorder->FinishRecording(
	    Director ? Director->EncounterTime : GetDefault<UReEchoBalanceSettings>()->EncounterDuration);
	const bool bPlayerSurvived = Player->Combatant->IsAlive();
	const bool bBossKilled = bPlayerSurvived && bEncounterClearedByDefeat &&
	                         RunSubsystem->EncounterIndex == RunSubsystem->GetTotalEncounterCount();
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
		// 死亡：保留 HUD 可见（显示"剩余 0 秒"+"关卡 X/Y"），不收起。
		return;
	}
	const float EncounterEndSettleSeconds = 0.5f;
	FTimerDelegate SettleDelegate = FTimerDelegate::CreateUObject(this, &AReEchoGameMode::ProceedToPostEncounterUI);
	GetWorldTimerManager().SetTimer(EncounterEndSettleTimerHandle, SettleDelegate, EncounterEndSettleSeconds, false);
}

void AReEchoGameMode::ProceedToPostEncounterUI()
{
	GetWorldTimerManager().ClearTimer(EncounterEndSettleTimerHandle);
	// #9 保留 HUD 可见（显示"剩余 0 秒"+"关卡 X/Y"），不收起。
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const bool bBossKilled =
	    bEncounterClearedByDefeat && RunSubsystem->EncounterIndex == RunSubsystem->GetTotalEncounterCount();
	if (bBossKilled)
	{
		ShowRestartScreen(false, true);
	}
	else if (RunSubsystem->EncounterIndex < RunSubsystem->GetTotalEncounterCount())
	{
		PrepareEncounterIntermission();
		ShowTraitCardChoice();
	}
}

void AReEchoGameMode::ShowTraitCardChoice()
{
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

	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	TraitCardChoiceWidget = UIFlow ? Cast<UReEchoTraitCardChoiceWidget>(UIFlow->OpenScreen(
	                                     PlayerController, EReEchoUIScreen::TraitChoice, false, true))
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
	ResumeWorldForMenuTransition();
	GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
}

void AReEchoGameMode::BuildMinimapView(FReEchoMinimapView& OutView) const
{
	OutView = FReEchoMinimapView();
	if (!Player)
	{
		return;
	}
	OutView.ArenaCenter = Player->GetArenaCenter2D();
	OutView.ArenaHalfExtents = Player->GetArenaHalfExtents2D();
	const FVector PlayerLocation = Player->GetActorLocation();
	OutView.PlayerLocation = FVector2D(PlayerLocation.X, PlayerLocation.Y);

	static const FLinearColor Palette[] = {FLinearColor::Red,
	                                       FLinearColor::Green,
	                                       FLinearColor::Blue,
	                                       FLinearColor::Yellow,
	                                       FLinearColor(0.0f, 1.0f, 1.0f),
	                                       FLinearColor(1.0f, 0.0f, 1.0f)};
	for (const TObjectPtr<AReEchoEchoActor>& Echo : Echoes)
	{
		if (!IsValid(Echo))
		{
			continue;
		}
		FReEchoMinimapEchoEntry Entry;
		const FVector EchoLocation = Echo->GetActorLocation();
		Entry.CurrentLocation = FVector2D(EchoLocation.X, EchoLocation.Y);
		Entry.PathPoints = Echo->GetRecordedPath();
		Entry.Color = Palette[OutView.Echoes.Num() % 6];
		OutView.Echoes.Add(Entry);
	}
	OutView.bValid = true;
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
		if (ArenaCameraActor)
		{
			ArenaCameraActor->Configure(Player, ArenaScene);
			PlayerController->SetViewTarget(ArenaCameraActor);
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
	if (EncounterHudWidget)
	{
		FReEchoMinimapView View;
		BuildMinimapView(View);
		EncounterHudWidget->SetMinimapView(View);
	}
	if (!bEncounterTransitioning && IsBossEncounter())
	{
		bool bEncounterDefeated = true;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (Entry.Archetype == EReEchoEnemyArchetype::Boss && Entry.bAlive)
			{
				bEncounterDefeated = false;
				break;
			}
		}
		if (bEncounterDefeated)
		{
			// [EncounterTimer] Boss 被提前击败：真实计时尚未到 0 即结束，剩余 > 0 属预期。
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[EncounterTimer][END] reason=boss-defeat EncounterTime=%.3f Remaining=%.3f"),
			       Director ? Director->EncounterTime : -1.0f,
			       Director ? Director->GetRemainingTime() : -1.0f);
			bEncounterClearedByDefeat = true;
			Director->EndEncounter();
		}
	}
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (EncounterHudWidget)
	{
		EncounterHudWidget->SetEncounterStatus(
		    RunSubsystem ? RunSubsystem->EncounterIndex : 0, GetTotalEncounterCount(), Director->GetRemainingTime());
	}
}
