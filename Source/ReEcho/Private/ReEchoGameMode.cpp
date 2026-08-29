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
#include "Data/ReEchoCsvDataRegistry.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Encounter/ReEchoEncounterFlowSettings.h"
#include "Enemies/ReEchoEnemyEventsComponent.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "DrawDebugHelpers.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoTimeShardPickupActor.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Player/ReEchoPlayerController.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Weapons/ReEchoWeaponActor.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/Scene/ReEchoArenaSceneCatalog.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "Presentation/Scene/ReEchoArenaSceneSpawnAnchor.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"
#include "Presentation/Enemy/ReEchoEnemyGameplayClassRegistry.h"
#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioService.h"
#include "Run/ReEchoRunSubsystem.h"
#include "Run/ReEchoRunStatsTracker.h"
#include "Run/ReEchoShopCatalog.h"
#include "UI/ReEchoEncounterHudWidget.h"
#include "UI/ReEchoEncounterTransitionWidget.h"
#include "UI/ReEchoInventoryShopWidget.h"
#include "UI/ReEchoLoadoutSelectionWidget.h"
#include "UI/ReEchoPlayerHudWidget.h"
#include "UI/ReEchoRestartWidget.h"
#include "UI/ReEchoSettingsWidget.h"
#include "UI/ReEchoStartMenuWidget.h"
#include "UI/ReEchoStatsWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"
#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "UI/ReEchoUIManagerSubsystem.h"
#include "UI/ReEchoWeatherWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "MediaSoundComponent.h"

AReEchoGameMode::AReEchoGameMode()
{
	PlayerControllerClass = AReEchoPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<AReEchoPlayerPawn> PlayerPrefab(
	    TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_PlayerGameplay"));
	DefaultPawnClass = PlayerPrefab.Succeeded() ? PlayerPrefab.Class.Get() : AReEchoPlayerPawn::StaticClass();
	static ConstructorHelpers::FClassFinder<AReEchoEchoActor> EchoPrefab(
	    TEXT("/Game/ReEcho/Gameplay/CharacterPrefabs/BP_EchoGameplay"));
	EchoGameplayClass = EchoPrefab.Succeeded() ? EchoPrefab.Class.Get() : nullptr;
	static ConstructorHelpers::FClassFinder<AReEchoTimeShardPickupActor> TimeShardPickupPrefab(
	    TEXT("/Game/ReEcho/Gameplay/Pickups/BP_TimeShardPickup"));
	TimeShardPickupClass = TimeShardPickupPrefab.Succeeded() ? TimeShardPickupPrefab.Class.Get()
	                                                         : AReEchoTimeShardPickupActor::StaticClass();
	static ConstructorHelpers::FClassFinder<UReEchoEncounterFlowSettings> EncounterFlowSettingsPrefab(
	    TEXT("/Game/ReEcho/Gameplay/Encounter/BP_EncounterFlowSettings"));
	EncounterFlowSettingsClass = EncounterFlowSettingsPrefab.Succeeded() ? EncounterFlowSettingsPrefab.Class.Get()
	                                                                     : UReEchoEncounterFlowSettings::StaticClass();
	static ConstructorHelpers::FObjectFinder<UReEcho2DPresentationCatalog> CatalogFinder(
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyPresentationCatalog.DA_EnemyPresentationCatalog"));
	PresentationCatalog = CatalogFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEchoEnemyGameplayClassRegistry> EnemyClassRegistryFinder(
	    TEXT("/Game/ReEcho/DataAsset/Enemy/Catalogs/DA_EnemyGameplayClassRegistry."
	         "DA_EnemyGameplayClassRegistry"));
	EnemyGameplayClassRegistry = EnemyClassRegistryFinder.Object;
	static ConstructorHelpers::FObjectFinder<UReEchoArenaSceneCatalog> ArenaSceneCatalogFinder(
	    TEXT("/Game/ReEcho/Scene/DA_ArenaSceneCatalog.DA_ArenaSceneCatalog"));
	ArenaSceneCatalog = ArenaSceneCatalogFinder.Object;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	EnemyRoster = CreateDefaultSubobject<UReEchoEnemyRosterComponent>(TEXT("EnemyRoster"));
	EncounterTransitionMediaSound = CreateDefaultSubobject<UMediaSoundComponent>(TEXT("EncounterTransitionMediaSound"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> ArenaBackgroundFinder(
	    TEXT("/Game/ReEcho/Textures/Scenes/ArenaGround3D.ArenaGround3D"));
	ArenaBackgroundTexture = ArenaBackgroundFinder.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArenaMaterialFinder(
	    TEXT("/Game/ReEcho/Materials/M_ArenaBackground.M_ArenaBackground"));
	ArenaBackgroundMaterial = ArenaMaterialFinder.Object;
}

TSubclassOf<AReEchoEchoActor> AReEchoGameMode::ResolveEchoClass() const
{
	if (EchoGameplayClass)
	{
		return EchoGameplayClass;
	}
	return TSubclassOf<AReEchoEchoActor>(AReEchoEchoActor::StaticClass());
}

AReEchoEchoActor* AReEchoGameMode::SpawnEchoActor()
{
	return GetWorld() ? GetWorld()->SpawnActor<AReEchoEchoActor>(ResolveEchoClass()) : nullptr;
}

#if WITH_DEV_AUTOMATION_TESTS
TSubclassOf<AReEchoEchoActor> AReEchoGameMode::ResolveEchoClassForTests() const
{
	return ResolveEchoClass();
}

AReEchoEchoActor* AReEchoGameMode::SpawnEchoActorForTests()
{
	return SpawnEchoActor();
}

void AReEchoGameMode::SetEchoGameplayClassForTests(TSubclassOf<AReEchoEchoActor> InClass)
{
	EchoGameplayClass = InClass;
}

bool AReEchoGameMode::ShouldGrantPostEntryInvulnerabilityForTests(const int32 EncounterIndex,
                                                                  const float DurationSeconds)
{
	return ShouldGrantPostEntryInvulnerability(EncounterIndex, DurationSeconds);
}
#endif

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
		FName VariantId = NAME_None;
		if (StateId == FReEchoAudioEvents::MusicEncounter)
		{
			const UReEchoRunSubsystem* RunSubsystem =
			    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
			const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
			    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
			const FReEchoCsvEncounterRow* Encounter =
			    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
			VariantId = Encounter ? Encounter->StageId : NAME_None;
		}
		AudioService->SetMusicStateVariant(StateId, VariantId);
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
	                   "<Clear|Rain|Fog> | GMScene <SC01|SC02|SC03|SC04> | GMMoveSpeed <cm/s> | "
	                   "GMEndEncounter | GMTransition4 | GMKillAll | GMSpawnFox <count> [distance] | "
	                   "GMGotoEncounter <1-based index> | GMGotoBoss | "
	                   "GMBossSkill <Skill01|Skill02|Skill02Moving|Skill03|Skill04> | "
	                   "GMEchoBorn | GMEchoSummon | "
	                   "GMElement <None|Flame|Lightning|Grass|Water> | "
	                   "GMEnemyElementAll <None|Grass|Water> | "
	                   "GMReaction <Burn|Vaporize|Growth|Conduct|EnhanceGrass|EnhanceWater> | "
	                   "GMWeapon <WeaponId> | GMEquipRune <PartId> | GMUnequipRune <SlotTypeId> | "
	                   "GMShowEnemyHealth <On|Off|Toggle> | "
	                   "GMShowEnemyRange <On|Off|Toggle> | "
	                   "GMBossDamageRange <On|Off|Toggle>"));
	PrintGMResult(TEXT("Reactions: Flame+Grass=Burn | Flame+Water=Vaporize | Lightning+Grass=Growth | "
	                   "Lightning+Water=Conduct | Grass+Water=EnhanceGrass | Water+Grass=EnhanceWater"));
}

bool AReEchoGameMode::TryResolveGMSceneId(const FString& Scene, FName& OutSceneId)
{
	FString Normalized = Scene.TrimStartAndEnd().ToUpper();
	if (Normalized.StartsWith(TEXT("SC")))
	{
		Normalized.RightChopInline(2);
	}
	if (!Normalized.IsNumeric())
	{
		return false;
	}
	const int32 SceneNumber = FCString::Atoi(*Normalized);
	if (SceneNumber < 1 || SceneNumber > 4)
	{
		return false;
	}
	OutSceneId = FName(*FString::Printf(TEXT("SC%02d"), SceneNumber));
	return true;
}

bool AReEchoGameMode::IsValidGMMoveSpeed(const float Speed)
{
	return FMath::IsFinite(Speed) && Speed > 0.0f;
}

void AReEchoGameMode::GMScene(const FString& Scene)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	FName SceneId = NAME_None;
	if (!TryResolveGMSceneId(Scene, SceneId))
	{
		PrintGMResult(TEXT("Usage: GMScene <SC01|SC02|SC03|SC04>"), false);
		return;
	}
	if (!ArenaSceneRegistry.Contains(SceneId))
	{
		PrintGMResult(FString::Printf(TEXT("GMScene cannot find registered SceneId=%s."), *SceneId.ToString()), false);
		return;
	}
	FReEchoCsvStageRow DebugStage;
	DebugStage.Id = TEXT("GMScene");
	DebugStage.SceneId = SceneId;
	FString Error;
	if (!ApplyArenaSceneForStage(DebugStage, Error))
	{
		PrintGMResult(FString::Printf(TEXT("GMScene %s failed: %s"), *SceneId.ToString(), *Error), false);
		return;
	}
	PrintGMResult(
	    FString::Printf(TEXT("Arena scene is now %s; current Stage/Encounter is unchanged."), *SceneId.ToString()));
}

void AReEchoGameMode::GMMoveSpeed(const float Speed)
{
	if (!EnsureGMCommandAvailable() || !Player || !Player->Movement)
	{
		PrintGMResult(TEXT("GMMoveSpeed requires an active player."), false);
		return;
	}
	if (!IsValidGMMoveSpeed(Speed))
	{
		PrintGMResult(TEXT("Usage: GMMoveSpeed <positive cm/s>"), false);
		return;
	}
	const float PreviousSpeed = Player->Movement->MaxSpeed;
	Player->Movement->MaxSpeed = Speed;
	PrintGMResult(
	    FString::Printf(TEXT("Player movement speed %.1f -> %.1f cm/s."), PreviousSpeed, Player->Movement->MaxSpeed));
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

bool AReEchoGameMode::TryResolveGMEnemyAttachment(const FString& Element, EReEchoElement& OutElement)
{
	if (Element.Equals(TEXT("Grass"), ESearchCase::IgnoreCase))
	{
		OutElement = EReEchoElement::Grass;
		return true;
	}
	if (Element.Equals(TEXT("Water"), ESearchCase::IgnoreCase))
	{
		OutElement = EReEchoElement::Water;
		return true;
	}
	if (Element.Equals(TEXT("None"), ESearchCase::IgnoreCase) || Element.Equals(TEXT("Clear"), ESearchCase::IgnoreCase))
	{
		OutElement = EReEchoElement::None;
		return true;
	}
	return false;
}

void AReEchoGameMode::GMEnemyElementAll(const FString& Element)
{
	if (!EnsureGMCommandAvailable() || !EnemyRoster)
	{
		PrintGMResult(TEXT("GMEnemyElementAll requires an active encounter."), false);
		return;
	}

	EReEchoElement Attachment = EReEchoElement::None;
	if (!TryResolveGMEnemyAttachment(Element, Attachment))
	{
		PrintGMResult(TEXT("Usage: GMEnemyElementAll <None|Grass|Water>"), false);
		return;
	}

	int32 AppliedCount = 0;
	for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
	{
		AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(EnemyHost.Get());
		UReEchoCombatantComponent* Combatant = Enemy ? Enemy->GetCombatantComponent() : nullptr;
		if (!Enemy || !Enemy->IsAlive() || !Combatant)
		{
			continue;
		}
		FReEchoElementState State = Combatant->GetElementState();
		State.Attached = Attachment;
		Combatant->RestoreElementState(State);
		++AppliedCount;
	}

	const FString AttachmentLabel =
	    Attachment == EReEchoElement::None ? TEXT("None") : ReEchoElementReaction::GetElementLabel(Attachment);
	PrintGMResult(FString::Printf(TEXT("Set enemy attachment=%s on %d living enemies; no damage or reactions fired."),
	                              *AttachmentLabel,
	                              AppliedCount));
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

	uint8 SemanticValue = 0;
	if (!UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(FName(*Reaction), SemanticValue))
	{
		PrintGMResult(TEXT("Usage: GMReaction <Burn|Vaporize|Growth|Conduct|EnhanceGrass|EnhanceWater>"), false);
		return;
	}
	UReEchoCombatVfxComponent* TargetVfx = Target->FindComponentByClass<UReEchoCombatVfxComponent>();
	if (Reaction.Equals(TEXT("Conduct"), ESearchCase::IgnoreCase))
	{
		AReEchoEnemyActor* NearestNeighbor = nullptr;
		float NearestDistanceSquared = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
		{
			AReEchoEnemyActor* Candidate = Cast<AReEchoEnemyActor>(EnemyHost.Get());
			if (!Candidate || Candidate == Target || !Candidate->IsAlive())
			{
				continue;
			}
			const float DistanceSquared =
			    FVector::DistSquared2D(Target->GetActorLocation(), Candidate->GetActorLocation());
			if (!NearestNeighbor || DistanceSquared < NearestDistanceSquared)
			{
				NearestNeighbor = Candidate;
				NearestDistanceSquared = DistanceSquared;
			}
		}
		const bool bPlayed =
		    TargetVfx && NearestNeighbor && TargetVfx->PlayConductLinkForDebug(Target, NearestNeighbor);
		PrintGMResult(
		    bPlayed ? FString::Printf(
		                  TEXT("Previewed Conduct from %s to %s through production world endpoints; combat state "
		                       "unchanged."),
		                  *Target->GetName(),
		                  *NearestNeighbor->GetName())
		            : TEXT("GMReaction Conduct requires at least two living enemies and a valid Electricity system."),
		    bPlayed);
		return;
	}
	const bool bPlayed = TargetVfx && TargetVfx->PlayElementReactionForDebug(SemanticValue, Target);
	PrintGMResult(bPlayed ? FString::Printf(TEXT("Previewed %s VFX directly on %s; combat state unchanged."),
	                                        *Reaction,
	                                        *Target->GetName())
	                      : FString::Printf(TEXT("Failed to preview %s VFX on %s."), *Reaction, *Target->GetName()),
	              bPlayed);
	(void)Damage;
}

void AReEchoGameMode::GMEquipRune(const FName PartId)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	if (PartId.IsNone())
	{
		PrintGMResult(TEXT("Usage: GMEquipRune <PartId>  (e.g. P_LONGSWORD_METEOR_SWORDBLADE)"), false);
		return;
	}

	// Authoritative run build (matches shop semantics; survives save/load). Non-fatal if no run is active yet.
	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (RunSubsystem)
	{
		TArray<FName> Desired;
		Desired.Reserve(RunSubsystem->CurrentBuild.EquippedParts.Num() + 1);
		for (const FReEchoEquippedPartSnapshot& Equipped : RunSubsystem->CurrentBuild.EquippedParts)
		{
			Desired.Add(Equipped.PartId);
		}
		if (!Desired.Contains(PartId))
		{
			Desired.Add(PartId);
			FString RunError;
			if (!RunSubsystem->TryEquipParts(Desired, RunError))
			{
				PrintGMResult(
				    FString::Printf(TEXT("GMEquipRune: run build not updated (%s); still applying to live weapon"),
				                    *RunError),
				    false);
			}
		}
	}

	// Reflect immediately on the live weapon actor so the rune is active without re-attacking.
	AReEchoWeaponActor* Weapon = nullptr;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(PC->GetPawn()))
		{
			Weapon = PlayerPawn->GetWeapon();
		}
	}
	if (!Weapon)
	{
		PrintGMResult(TEXT("GMEquipRune: no live weapon actor (start an encounter first)"), false);
		return;
	}
	FString OutError;
	if (Weapon->EquipRune(PartId, OutError))
	{
		PostUiEvent(FReEchoAudioEvents::UiEquip);
		PrintGMResult(FString::Printf(TEXT("Equipped rune %s on weapon"), *PartId.ToString()), true);
	}
	else
	{
		PrintGMResult(FString::Printf(TEXT("GMEquipRune %s failed: %s"), *PartId.ToString(), *OutError), false);
	}
}

void AReEchoGameMode::GMWeapon(const FName WeaponId)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	if (WeaponId.IsNone())
	{
		PrintGMResult(TEXT("Usage: GMWeapon <WeaponId>  (e.g. W_J_01 / W_J_08 / W_J_09)"), false);
		return;
	}

	AReEchoPlayerPawn* PlayerPawn = Player;
	if (!PlayerPawn)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PlayerPawn = Cast<AReEchoPlayerPawn>(PC->GetPawn());
		}
	}
	AReEchoWeaponActor* LiveWeapon = PlayerPawn ? PlayerPawn->GetWeapon() : nullptr;
	if (!PlayerPawn || !LiveWeapon)
	{
		PrintGMResult(TEXT("GMWeapon requires a live player weapon (start an encounter first)."), false);
		return;
	}

	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvWeaponRow* Weapon = Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(WeaponId) : nullptr;
	if (!Weapon)
	{
		PrintGMResult(
		    FString::Printf(TEXT("GMWeapon failed: WeaponId '%s' is unknown or disabled."), *WeaponId.ToString()),
		    false);
		return;
	}

	if (RunSubsystem && !RunSubsystem->CurrentBuild.WeaponDomainRevision.IsEmpty())
	{
		const FReEchoBuildSnapshot PreviousBuild = RunSubsystem->CurrentBuild;
		FReEchoBuildSnapshot CandidateBuild;
		FString SelectError;
		if (!ReEchoWeaponRuntime::TrySelectWeapon(*Snapshot, PreviousBuild, WeaponId, CandidateBuild, SelectError))
		{
			PrintGMResult(FString::Printf(TEXT("GMWeapon %s failed: %s"), *WeaponId.ToString(), *SelectError), false);
			return;
		}
		if (!PlayerPawn->InitializeWeaponFromBuild(CandidateBuild, Snapshot))
		{
			PlayerPawn->InitializeWeaponFromBuild(PreviousBuild, Snapshot);
			PrintGMResult(FString::Printf(TEXT("GMWeapon %s failed: live weapon rejected the candidate build."),
			                              *WeaponId.ToString()),
			              false);
			return;
		}
		RunSubsystem->CurrentBuild = MoveTemp(CandidateBuild);
	}
	else if (!LiveWeapon->SelectWeaponById(WeaponId))
	{
		PrintGMResult(
		    FString::Printf(TEXT("GMWeapon %s failed: live weapon could not select it."), *WeaponId.ToString()), false);
		return;
	}

	PostUiEvent(FReEchoAudioEvents::UiEquip);
	PrintGMResult(FString::Printf(TEXT("Switched weapon to %s (%s)."), *WeaponId.ToString(), *Weapon->DisplayName),
	              true);
}

void AReEchoGameMode::GMUnequipRune(const FName SlotTypeId)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	if (SlotTypeId.IsNone())
	{
		PrintGMResult(TEXT("Usage: GMUnequipRune <SlotTypeId>  (e.g. Blade / Grip / Muzzle / GunAction / Arrowhead)"),
		              false);
		return;
	}

	// Sync authoritative run build first (drop every equipped part in that slot).
	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (RunSubsystem)
	{
		const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
		TArray<FName> Desired;
		for (const FReEchoEquippedPartSnapshot& Equipped : RunSubsystem->CurrentBuild.EquippedParts)
		{
			const FReEchoCsvPartRow* Part = Snapshot ? Snapshot->Parts.Find(Equipped.PartId) : nullptr;
			if (Part && Part->SlotTypeId == SlotTypeId)
			{
				continue;
			}
			Desired.Add(Equipped.PartId);
		}
		FString RunError;
		if (!RunSubsystem->TryEquipParts(Desired, RunError))
		{
			PrintGMResult(FString::Printf(TEXT("GMUnequipRune: run build not updated (%s)"), *RunError), false);
		}
	}

	// Reflect on the live weapon actor.
	AReEchoWeaponActor* Weapon = nullptr;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AReEchoPlayerPawn* PlayerPawn = Cast<AReEchoPlayerPawn>(PC->GetPawn()))
		{
			Weapon = PlayerPawn->GetWeapon();
		}
	}
	if (!Weapon)
	{
		PrintGMResult(TEXT("GMUnequipRune: no live weapon actor (start an encounter first)"), false);
		return;
	}
	FString OutError;
	if (Weapon->UnequipRune(SlotTypeId, OutError))
	{
		PostUiEvent(FReEchoAudioEvents::UiUnequip);
		PrintGMResult(FString::Printf(TEXT("Unequipped rune in slot %s"), *SlotTypeId.ToString()), true);
	}
	else
	{
		PrintGMResult(FString::Printf(TEXT("GMUnequipRune %s failed: %s"), *SlotTypeId.ToString(), *OutError), false);
	}
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

void AReEchoGameMode::GMShowEnemyHealth(const FString& Mode)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	bool bEnable = !bShowEnemyHealthDebug;
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
		PrintGMResult(TEXT("Usage: GMShowEnemyHealth <On|Off|Toggle>"), false);
		return;
	}
	bShowEnemyHealthDebug = bEnable;
	PrintGMResult(FString::Printf(TEXT("Enemy health overlay=%s."), bEnable ? TEXT("On") : TEXT("Off")));
}

void AReEchoGameMode::GMShowEnemyRange(const FString& Mode)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	bool bEnable = !bShowEnemyRangeDebug;
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
		PrintGMResult(TEXT("Usage: GMShowEnemyRange <On|Off|Toggle>"), false);
		return;
	}
	bShowEnemyRangeDebug = bEnable;
	PrintGMResult(FString::Printf(TEXT("Enemy damage-range overlay=%s (red=contact, orange=ranged max)."),
	                              bEnable ? TEXT("On") : TEXT("Off")));
}

void AReEchoGameMode::GMBossDamageRange(const FString& Mode)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	bool bEnable = !bShowBossDamageRangeDebug;
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
		PrintGMResult(TEXT("Usage: GMBossDamageRange <On|Off|Toggle>"), false);
		return;
	}
	bShowBossDamageRangeDebug = bEnable;
	PrintGMResult(FString::Printf(
	    TEXT("Boss skill damage-range debug=%s (red=Skill02 projectile, green=Skill03 AOE, cyan=beam/rectangle)."),
	    bEnable ? TEXT("On") : TEXT("Off")));
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
	PrintGMResult(FString::Printf(TEXT("Player God mode=%s (damage numbers remain visible; HP is preserved)."),
	                              bEnable ? TEXT("On") : TEXT("Off")));
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

void AReEchoGameMode::GMTransition4()
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
		PrintGMResult(TEXT("GMTransition4 requires a living player in an active encounter."), false);
		return;
	}
	if (IsBossEncounter())
	{
		PrintGMResult(TEXT("GMTransition4 is only available for ordinary timed encounters."), false);
		return;
	}

	const float TargetRemainingTime = FMath::Min(4.0f, Director->GetEncounterDuration());
	Director->ResumeEncounter(Director->GetEncounterDuration() - TargetRemainingTime);
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[EncounterTransition] GMTransition4 remaining=%.3f encounter=%d"),
	       Director->GetRemainingTime(),
	       RunSubsystem->EncounterIndex);
	PrintGMResult(FString::Printf(TEXT("Encounter %d advanced to %.1f seconds remaining."),
	                              RunSubsystem->EncounterIndex,
	                              Director->GetRemainingTime()));
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

void AReEchoGameMode::ResolveGMSpawnFoxRequest(
    const float CountOrDistance, const float Distance, int32& OutCount, float& OutDistance, bool& bOutLegacyDistance)
{
	constexpr int32 MaximumCount = 16;
	constexpr float DefaultDistance = 350.0f;
	constexpr float MinimumDistance = 150.0f;
	constexpr float MaximumDistance = 1000.0f;
	bOutLegacyDistance = Distance < 0.0f && CountOrDistance > static_cast<float>(MaximumCount);
	OutCount = bOutLegacyDistance ? 1 : FMath::Clamp(FMath::RoundToInt(CountOrDistance), 1, MaximumCount);
	const float RequestedDistance =
	    Distance >= 0.0f ? Distance : (bOutLegacyDistance ? CountOrDistance : DefaultDistance);
	OutDistance = FMath::Clamp(RequestedDistance, MinimumDistance, MaximumDistance);
}

TArray<FVector> AReEchoGameMode::BuildGMSpawnFoxLocations(const FVector& PlayerLocation,
                                                          const FBox2D& SpawnWorldBounds,
                                                          const float GameplayPlaneWorldZ,
                                                          const int32 Count,
                                                          const float Distance,
                                                          const bool bHasValidBounds)
{
	TArray<FVector> Locations;
	Locations.Reserve(Count);
	const FVector2D ArenaCenter = bHasValidBounds ? SpawnWorldBounds.GetCenter() : FVector2D::ZeroVector;
	FVector2D InwardDirection =
	    bHasValidBounds ? ArenaCenter - FVector2D(PlayerLocation.X, PlayerLocation.Y) : FVector2D(1.0f, 0.0f);
	if (InwardDirection.IsNearlyZero())
	{
		InwardDirection = FVector2D(1.0f, 0.0f);
	}
	InwardDirection.Normalize();
	constexpr float ArcHalfAngleDegrees = 70.0f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = Count > 1 ? static_cast<float>(Index) / static_cast<float>(Count - 1) : 0.5f;
		const float AngleDegrees = FMath::Lerp(-ArcHalfAngleDegrees, ArcHalfAngleDegrees, Alpha);
		const FVector2D RadialDirection = InwardDirection.GetRotated(AngleDegrees);
		FVector Location(PlayerLocation.X + RadialDirection.X * Distance,
		                 PlayerLocation.Y + RadialDirection.Y * Distance,
		                 bHasValidBounds ? GameplayPlaneWorldZ : PlayerLocation.Z);
		if (!bHasValidBounds || SpawnWorldBounds.IsInside(FVector2D(Location.X, Location.Y)))
		{
			Locations.Add(Location);
		}
	}
	for (int32 GridIndex = 0; bHasValidBounds && Locations.Num() < Count && GridIndex < 256; ++GridIndex)
	{
		constexpr int32 GridSide = 16;
		const float AlphaX = (static_cast<float>(GridIndex % GridSide) + 0.5f) / GridSide;
		const float AlphaY = (static_cast<float>(GridIndex / GridSide) + 0.5f) / GridSide;
		const FVector Candidate(FMath::Lerp(SpawnWorldBounds.Min.X, SpawnWorldBounds.Max.X, AlphaX),
		                        FMath::Lerp(SpawnWorldBounds.Min.Y, SpawnWorldBounds.Max.Y, AlphaY),
		                        GameplayPlaneWorldZ);
		// M_FOX has the largest normal-enemy radius (65 cm); keep test spawns from overlapping each other.
		constexpr float FoxMinimumCenterSpacing = 130.0f;
		if (!Locations.ContainsByPredicate(
		        [&Candidate](const FVector& Existing)
		        {
			        return FVector::Dist2D(Existing, Candidate) < FoxMinimumCenterSpacing;
		        }))
		{
			Locations.Add(Candidate);
		}
	}
	return Locations;
}

void AReEchoGameMode::GMSpawnFox(const float CountOrDistance, const float Distance)
{
	if (!EnsureGMCommandAvailable() || !Player || !Player->Combatant || !Player->Combatant->IsAlive())
	{
		PrintGMResult(TEXT("GMSpawnFox requires a living player."), false);
		return;
	}
	int32 SafeCount = 1;
	float SafeDistance = 350.0f;
	bool bLegacyDistance = false;
	ResolveGMSpawnFoxRequest(CountOrDistance, Distance, SafeCount, SafeDistance, bLegacyDistance);
	const FVector PlayerLocation = Player->GetActorLocation();
	FBox2D SpawnWorldBounds(ForceInit);
	FString BoundsError;
	const bool bHasArena = ArenaScene && ArenaScene->GetEnemySpawnWorldBounds(SpawnWorldBounds, &BoundsError);
	if (!bHasArena)
	{
		PrintGMResult(
		    FString::Printf(TEXT("GMSpawnFox rejected: active Arena wall bounds are invalid: %s"), *BoundsError),
		    false);
		return;
	}
	const float GameplayPlaneWorldZ = ArenaScene->GetGameplayPlaneWorldZ();
	const TArray<FVector> SpawnLocations = BuildGMSpawnFoxLocations(
	    PlayerLocation, SpawnWorldBounds, GameplayPlaneWorldZ, SafeCount, SafeDistance, bHasArena);
	int32 SuccessCount = 0;
	for (const FVector& SpawnLocation : SpawnLocations)
	{
		if (SpawnConfiguredEnemy(TEXT("M_FOX"), SpawnLocation))
		{
			++SuccessCount;
		}
	}
	const int32 FailureCount = SafeCount - SuccessCount;
	PrintGMResult(FString::Printf(TEXT("GMSpawnFox%s requested %.0f, used count=%d distance=%.0f cm: "
	                                   "%d succeeded, %d failed."),
	                              bLegacyDistance ? TEXT(" legacy-distance") : TEXT(""),
	                              CountOrDistance,
	                              SafeCount,
	                              SafeDistance,
	                              SuccessCount,
	                              FailureCount),
	              FailureCount == 0);
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

	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[BossVictoryTrace][GMGotoBoss] before jump currentEncounter=%d bossEncounter=%d roster=%d "
	            "encounterTime=%.3f"),
	       RunSubsystem->EncounterIndex,
	       BossEncounterIndex,
	       EnemyRoster ? EnemyRoster->GetEntries().Num() : -1,
	       Director ? Director->EncounterTime : -1.0f);
	RunSubsystem->EncounterIndex = BossEncounterIndex - 1;
	BeginNextEncounter();
	const bool bStartedBossEncounter = RunSubsystem->EncounterIndex == BossEncounterIndex && IsBossEncounter();
	int32 PendingBossBatchCount = 0;
	int32 PendingBossLocationCount = 0;
	for (const FReEchoPendingSpawnBatchState& Pending : PendingSpawnBatches)
	{
		if (Pending.EnemyRole == TEXT("Boss"))
		{
			++PendingBossBatchCount;
			PendingBossLocationCount += Pending.Locations.Num();
		}
	}
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[BossVictoryTrace][GMGotoBoss] after jump encounter=%d isBossEncounter=%s roster=%d "
	            "pendingBossBatches=%d pendingBossLocations=%d scheduler=%d/%d encounterTime=%.3f"),
	       RunSubsystem->EncounterIndex,
	       bStartedBossEncounter ? TEXT("true") : TEXT("false"),
	       EnemyRoster ? EnemyRoster->GetEntries().Num() : -1,
	       PendingBossBatchCount,
	       PendingBossLocationCount,
	       EncounterWaveScheduler.GetNextEventIndex(),
	       EncounterWaveScheduler.GetEventCount(),
	       Director ? Director->EncounterTime : -1.0f);
	PrintGMResult(bStartedBossEncounter
	                  ? FString::Printf(TEXT("Started Boss encounter %d."), BossEncounterIndex)
	                  : FString::Printf(TEXT("Failed to start Boss encounter %d."), BossEncounterIndex),
	              bStartedBossEncounter);
}

void AReEchoGameMode::GMGotoEncounter(const int32 EncounterNumber)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}

	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !Director || !Player || !Player->Combatant)
	{
		PrintGMResult(TEXT("Encounter jump cannot start because the active run is not initialized."), false);
		return;
	}
	if (RunSubsystem->Phase != EReEchoRunPhase::Encounter || bAwaitingStartChoice || bEncounterTransitioning ||
	    !Player->Combatant->IsAlive())
	{
		PrintGMResult(TEXT("GMGotoEncounter requires a living player in an active encounter."), false);
		return;
	}

	const int32 TotalEncounterCount = GetTotalEncounterCount();
	if (EncounterNumber < 1 || EncounterNumber > TotalEncounterCount)
	{
		PrintGMResult(FString::Printf(TEXT("GMGotoEncounter target must be between 1 and %d."), TotalEncounterCount),
		              false);
		return;
	}

	const int32 PreviousEncounter = RunSubsystem->EncounterIndex;
	ClearEnemyRoster();
	ClearEchoes();
	RunSubsystem->EncounterIndex = EncounterNumber - 1;
	BeginNextEncounter();
	const bool bStartedTarget = RunSubsystem->Phase == EReEchoRunPhase::Encounter &&
	                            RunSubsystem->EncounterIndex == EncounterNumber &&
	                            Director->GetEncounterDuration() > 0.0f;
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[GMGotoEncounter] previous=%d requested=%d actual=%d active=%s"),
	       PreviousEncounter,
	       EncounterNumber,
	       RunSubsystem->EncounterIndex,
	       bStartedTarget ? TEXT("true") : TEXT("false"));
	PrintGMResult(bStartedTarget
	                  ? FString::Printf(TEXT("Started encounter %d of %d."), EncounterNumber, TotalEncounterCount)
	                  : FString::Printf(TEXT("Failed to start encounter %d."), EncounterNumber),
	              bStartedTarget);
}

void AReEchoGameMode::GMBossSkill(const FString& Skill)
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}

	FName AbilityId = NAME_None;
	if (Skill.Equals(TEXT("Skill01"), ESearchCase::IgnoreCase) || Skill.Equals(TEXT("1")))
	{
		AbilityId = TEXT("M_SHEEP_MeleeSweep");
	}
	else if (Skill.Equals(TEXT("Skill02"), ESearchCase::IgnoreCase) ||
	         Skill.Equals(TEXT("Skill02Stationary"), ESearchCase::IgnoreCase) || Skill.Equals(TEXT("2")))
	{
		AbilityId = TEXT("M_SHEEP_StationaryVolley");
	}
	else if (Skill.Equals(TEXT("Skill02Moving"), ESearchCase::IgnoreCase) ||
	         Skill.Equals(TEXT("2M"), ESearchCase::IgnoreCase))
	{
		AbilityId = TEXT("M_SHEEP_MovingSpread");
	}
	else if (Skill.Equals(TEXT("Skill03"), ESearchCase::IgnoreCase) || Skill.Equals(TEXT("3")))
	{
		AbilityId = TEXT("M_SHEEP_BlinkSlam");
	}
	else if (Skill.Equals(TEXT("Skill04"), ESearchCase::IgnoreCase) || Skill.Equals(TEXT("4")))
	{
		AbilityId = TEXT("M_SHEEP_PrayerBeam");
	}
	else
	{
		PrintGMResult(TEXT("Usage: GMBossSkill <Skill01|Skill02|Skill02Moving|Skill03|Skill04>"), false);
		return;
	}

	AReEchoEnemyActor* Boss = nullptr;
	if (EnemyRoster)
	{
		for (const TWeakObjectPtr<AActor>& EnemyHost : EnemyRoster->GetLivingEnemyActors())
		{
			AReEchoEnemyActor* Candidate = Cast<AReEchoEnemyActor>(EnemyHost.Get());
			if (Candidate && Candidate->IsAlive() && Candidate->GetEnemyId() == TEXT("M_SHEEP"))
			{
				Boss = Candidate;
				break;
			}
		}
	}
	if (!Boss || !Boss->GetEnemyLogicComponent())
	{
		PrintGMResult(TEXT("GMBossSkill requires a living M_SHEEP Boss."), false);
		return;
	}

	const bool bQueued = Boss->GetEnemyLogicComponent()->DebugQueueBossAbility(AbilityId);
	PrintGMResult(bQueued ? FString::Printf(TEXT("Queued Boss ability %s through the normal skill state machine."),
	                                        *AbilityId.ToString())
	                      : FString::Printf(TEXT("Boss ability %s is unavailable in the active definition."),
	                                        *AbilityId.ToString()),
	              bQueued);
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

void AReEchoGameMode::GMEchoBorn()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	int32 PlayedCount = 0;
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (IsValid(Echo) && Echo->IsCombatTargetAlive() && Echo->PlayBornVfx())
		{
			++PlayedCount;
		}
	}
	PrintGMResult(PlayedCount > 0 ? FString::Printf(TEXT("Replayed Echo Born VFX on %d living Echo(es)."), PlayedCount)
	                              : TEXT("GMEchoBorn requires at least one living Echo."),
	              PlayedCount > 0);
}

void AReEchoGameMode::GMEchoSummon()
{
	if (!EnsureGMCommandAvailable())
	{
		return;
	}
	int32 PlayedCount = 0;
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (!IsValid(Echo) || !Echo->IsCombatTargetAlive())
		{
			continue;
		}
		Echo->PrepareBornRevealAtCurrentLocation();
		if (!Echo->BeginDeferredBornReveal())
		{
			Echo->CompleteDeferredBornReveal();
			continue;
		}
		++PlayedCount;
		const TWeakObjectPtr<AReEchoEchoActor> WeakEcho(Echo);
		FTimerHandle RevealTimer;
		GetWorldTimerManager().SetTimer(
		    RevealTimer,
		    [WeakEcho]()
		    {
			    if (AReEchoEchoActor* ActiveEcho = WeakEcho.Get())
			    {
				    ActiveEcho->CompleteDeferredBornReveal();
			    }
		    },
		    GetStage01To02EchoRevealDelaySeconds(),
		    false);
	}
	PrintGMResult(PlayedCount > 0
	                  ? FString::Printf(TEXT("Replayed Echo summon reveal on %d living Echo(es); reveal delay %.1fs."),
	                                    PlayedCount,
	                                    GetStage01To02EchoRevealDelaySeconds())
	                  : TEXT("GMEchoSummon requires at least one living Echo and a valid Echo Born system."),
	              PlayedCount > 0);
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
	if (UReEchoRunSubsystem* RunSubsystem =
	        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr)
	{
		RunSubsystem->OnCardGrantCommitted.RemoveAll(this);
		RunSubsystem->OnCardGrantCommitted.AddUObject(this, &AReEchoGameMode::HandleCardGrantCommitted);
	}
	int32 ArenaSceneCount = 0;
	for (TActorIterator<AReEchoArenaSceneActor> It(GetWorld()); It; ++It)
	{
		ArenaScene = *It;
		++ArenaSceneCount;
	}
	FString ArenaFailure;
	if (ArenaSceneCount > 1 || (ArenaScene && !ArenaScene->HasValidConfiguration(&ArenaFailure)))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Expected at most one valid migration ArenaScene; found %d. %s"),
		       ArenaSceneCount,
		       *ArenaFailure);
		return;
	}
	int32 ArenaSceneSpawnAnchorCount = 0;
	for (TActorIterator<AReEchoArenaSceneSpawnAnchor> It(GetWorld()); It; ++It)
	{
		ArenaSceneSpawnTransform = It->GetActorTransform();
		++ArenaSceneSpawnAnchorCount;
	}
	if (ArenaSceneSpawnAnchorCount > 1 || (!ArenaScene && ArenaSceneSpawnAnchorCount != 1))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Expected one ArenaSceneSpawnAnchor when no migration Arena exists; found %d."),
		       ArenaSceneSpawnAnchorCount);
		return;
	}
	if (ArenaScene)
	{
		ArenaSceneSpawnTransform = ArenaScene->GetActorTransform();
	}
	if (!InitializeArenaSceneRegistry(ArenaFailure))
	{
		UE_LOG(LogTemp, Error, TEXT("[ArenaScene] Registry initialization failed: %s"), *ArenaFailure);
		return;
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
	if (ArenaScene)
	{
		RefreshArenaSceneConsumers();
	}
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
	const TArray<FReEchoSaveSlotSummary> SaveSlots = RunSubsystem->GetSaveSlotSummaries();
	const bool bHasSavedRun = SaveSlots.ContainsByPredicate(
	    [](const FReEchoSaveSlotSummary& Slot)
	    {
		    return Slot.bOccupied;
	    });
	StartMenuWidget->InitializeMenu(SaveSlots);
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[ReEchoStartFlow] Showing start menu. HasSavedRun=%s"),
	       bHasSavedRun ? TEXT("true") : TEXT("false"));
	StartMenuWidget->OnNewGameRequested.AddDynamic(this, &AReEchoGameMode::HandleNewGameRequested);
	StartMenuWidget->OnContinueGameRequested.AddDynamic(this, &AReEchoGameMode::HandleContinueGameRequested);
	StartMenuWidget->OnSaveSlotRequested.AddDynamic(this, &AReEchoGameMode::HandleSaveSlotRequested);
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
	if (!RunSubsystem->SelectFirstEmptySaveSlot())
	{
		const TArray<FReEchoSaveSlotSummary> SaveSlots = RunSubsystem->GetSaveSlotSummaries();
		const FReEchoSaveSlotSummary* OldestSaveSlot = nullptr;
		for (const FReEchoSaveSlotSummary& SaveSlot : SaveSlots)
		{
			if (!SaveSlot.bOccupied)
			{
				continue;
			}

			if (!OldestSaveSlot || SaveSlot.SavedAtUtc < OldestSaveSlot->SavedAtUtc)
			{
				OldestSaveSlot = &SaveSlot;
			}
		}

		if (!OldestSaveSlot || !RunSubsystem->SelectSaveSlot(OldestSaveSlot->SlotIndex))
		{
			PostUiEvent(FReEchoAudioEvents::UiError);
			UE_LOG(LogTemp, Warning, TEXT("[ReEchoStartFlow] New game failed: no replaceable save slot found."));
			return;
		}

		UE_LOG(LogTemp,
		       Display,
		       TEXT("[ReEchoStartFlow] Replacing oldest save slot. Slot=%d SavedAtUtc=%s"),
		       OldestSaveSlot->SlotIndex + 1,
		       *OldestSaveSlot->SavedAtUtc.ToIso8601());
		RunSubsystem->DeleteSavedRun();
	}
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
			StartMenuWidget->InitializeMenu(RunSubsystem ? RunSubsystem->GetSaveSlotSummaries()
			                                             : TArray<FReEchoSaveSlotSummary>());
		}
		return;
	}
	RequestBeginSelectedRun();
}

void AReEchoGameMode::HandleSaveSlotRequested(const int32 SlotIndex)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const TArray<FReEchoSaveSlotSummary> Summaries = RunSubsystem->GetSaveSlotSummaries();
	const FReEchoSaveSlotSummary* Summary = Summaries.FindByPredicate(
	    [SlotIndex](const FReEchoSaveSlotSummary& Candidate)
	    {
		    return Candidate.SlotIndex == SlotIndex;
	    });
	if (!Summary)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	if (Summary->bOccupied)
	{
		if (!RunSubsystem->LoadSavedRunFromSlot(SlotIndex))
		{
			PostUiEvent(FReEchoAudioEvents::UiError);
			StartMenuWidget->InitializeMenu(RunSubsystem->GetSaveSlotSummaries());
			return;
		}
		RequestBeginSelectedRun();
		return;
	}
	// Every empty row represents the single safe "new save" action. Always
	// allocate the first empty physical slot so an accidental click cannot
	// create a hole or skip over an earlier available slot.
	if (!RunSubsystem->SelectFirstEmptySaveSlot())
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	ShowLoadoutSelection();
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
	ResetEncounterTransitionPresentation();
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	// A run never spans across BeginSelectedRun, so the display-only stats restart with it.
	if (UReEchoRunStatsSubsystem* Stats = GetGameInstance()->GetSubsystem<UReEchoRunStatsSubsystem>())
	{
		Stats->ResetRunStats();
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
	if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
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
	ConnectionLineSideByPair.Reset();
	ClearTimeShardPickups();
	ClearEnemyRoster();
	ClearEchoes();
}

int32 AReEchoGameMode::ClearTimeShardPickupsInWorld(UWorld* World)
{
	if (!World)
	{
		return 0;
	}
	int32 ClearedCount = 0;
	for (TActorIterator<AReEchoTimeShardPickupActor> It(World); It; ++It)
	{
		AReEchoTimeShardPickupActor* Pickup = *It;
		if (IsValid(Pickup) && !Pickup->IsActorBeingDestroyed() && Pickup->Destroy())
		{
			++ClearedCount;
		}
	}
	return ClearedCount;
}

void AReEchoGameMode::ClearTimeShardPickups()
{
	const int32 ClearedCount = ClearTimeShardPickupsInWorld(GetWorld());
	if (ClearedCount > 0)
	{
		UE_LOG(LogReEcho, Display, TEXT("[TimeShardPickup] cleared %d encounter-scoped pickup(s)"), ClearedCount);
	}
}

#if WITH_DEV_AUTOMATION_TESTS
int32 AReEchoGameMode::ClearTimeShardPickupsInWorldForTests(UWorld* World)
{
	return ClearTimeShardPickupsInWorld(World);
}
#endif

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
	if (Player)
	{
		if (UReEchoCombatVfxComponent* PlayerVfx = Player->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			PlayerVfx->ClearEchoConnectionLinks();
		}
	}
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

bool AReEchoGameMode::InitializeArenaSceneRegistry(FString& OutError)
{
	if (!ArenaSceneCatalog || !ArenaSceneCatalog->BuildRegistry(ArenaSceneRegistry, OutError))
	{
		if (!ArenaSceneCatalog)
		{
			OutError = TEXT("Arena Scene Catalog is not configured.");
		}
		return false;
	}
	ActiveArenaSceneId = ArenaScene ? ArenaScene->GetSceneId() : NAME_None;
	if (ArenaScene && (ActiveArenaSceneId.IsNone() || !ArenaSceneRegistry.Contains(ActiveArenaSceneId)))
	{
		OutError =
		    FString::Printf(TEXT("Placed Arena Scene has unregistered SceneId=%s."), *ActiveArenaSceneId.ToString());
		ArenaSceneRegistry.Reset();
		ActiveArenaSceneId = NAME_None;
		return false;
	}
	return true;
}

bool AReEchoGameMode::PrepareArenaSceneForStage(const FReEchoCsvStageRow& Stage, FString& OutError)
{
	OutError.Reset();
	if (Stage.SceneId.IsNone())
	{
		OutError = FString::Printf(TEXT("StageId=%s has an empty SceneId."), *Stage.Id.ToString());
		return false;
	}
	if (ArenaScene && ActiveArenaSceneId == Stage.SceneId)
	{
		if (PendingArenaScene)
		{
			PendingArenaScene->Destroy();
			PendingArenaScene = nullptr;
			PendingArenaSceneId = NAME_None;
		}
		return true;
	}
	if (PendingArenaScene && PendingArenaSceneId == Stage.SceneId)
	{
		return true;
	}
	if (PendingArenaScene)
	{
		PendingArenaScene->Destroy();
		PendingArenaScene = nullptr;
		PendingArenaSceneId = NAME_None;
	}
	const TSubclassOf<AReEchoArenaSceneActor>* SceneClassReference = ArenaSceneRegistry.Find(Stage.SceneId);
	if (!SceneClassReference)
	{
		OutError = FString::Printf(
		    TEXT("StageId=%s references unknown SceneId=%s."), *Stage.Id.ToString(), *Stage.SceneId.ToString());
		return false;
	}
	UClass* SceneClass = SceneClassReference->Get();
	if (!SceneClass || !SceneClass->IsChildOf(AReEchoArenaSceneActor::StaticClass()))
	{
		OutError = FString::Printf(TEXT("StageId=%s SceneId=%s could not load a valid Arena class."),
		                           *Stage.Id.ToString(),
		                           *Stage.SceneId.ToString());
		return false;
	}
	const FTransform SpawnTransform = ArenaScene ? ArenaScene->GetActorTransform() : ArenaSceneSpawnTransform;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AReEchoArenaSceneActor* NewArenaScene =
	    GetWorld()->SpawnActor<AReEchoArenaSceneActor>(SceneClass, SpawnTransform, SpawnParameters);
	FString ConfigurationError;
	if (!NewArenaScene || NewArenaScene->GetSceneId() != Stage.SceneId ||
	    !NewArenaScene->HasValidConfiguration(&ConfigurationError))
	{
		if (NewArenaScene)
		{
			NewArenaScene->Destroy();
		}
		OutError = FString::Printf(TEXT("StageId=%s SceneId=%s produced an invalid Arena: %s"),
		                           *Stage.Id.ToString(),
		                           *Stage.SceneId.ToString(),
		                           *ConfigurationError);
		return false;
	}
	NewArenaScene->SetActorEnableCollision(false);
	NewArenaScene->SetActorHiddenInGame(true);
	PendingArenaScene = NewArenaScene;
	PendingArenaSceneId = Stage.SceneId;
	return true;
}

bool AReEchoGameMode::ApplyArenaSceneForStage(const FReEchoCsvStageRow& Stage, FString& OutError)
{
	if (!PrepareArenaSceneForStage(Stage, OutError))
	{
		return false;
	}
	if (ArenaScene && ActiveArenaSceneId == Stage.SceneId)
	{
		return true;
	}
	if (!PendingArenaScene || PendingArenaSceneId != Stage.SceneId)
	{
		OutError = FString::Printf(TEXT("StageId=%s SceneId=%s has no prepared Arena candidate."),
		                           *Stage.Id.ToString(),
		                           *Stage.SceneId.ToString());
		return false;
	}
	AReEchoArenaSceneActor* PreviousArenaScene = ArenaScene;
	ArenaScene = PendingArenaScene;
	ActiveArenaSceneId = Stage.SceneId;
	PendingArenaScene = nullptr;
	PendingArenaSceneId = NAME_None;
	ArenaScene->SetActorHiddenInGame(false);
	ArenaScene->SetActorEnableCollision(true);
	RefreshArenaSceneConsumers();
	if (PreviousArenaScene)
	{
		PreviousArenaScene->SetActorEnableCollision(false);
		PreviousArenaScene->SetActorHiddenInGame(true);
		PreviousArenaScene->Destroy();
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[ArenaScene] Applied StageId=%s SceneId=%s Arena=%s"),
	       *Stage.Id.ToString(),
	       *Stage.SceneId.ToString(),
	       *GetNameSafe(ArenaScene));
	return true;
}

void AReEchoGameMode::RefreshArenaSceneConsumers()
{
	if (!ArenaScene)
	{
		return;
	}
	const FVector2D PlayerHalfExtents = ArenaScene->GetPlayerHalfExtents();
	FBox2D SpawnWorldBounds(ForceInit);
	if (ArenaScene->GetEnemySpawnWorldBounds(SpawnWorldBounds))
	{
		const FVector2D SpawnSize = SpawnWorldBounds.GetSize();
		ArenaSceneWorldHeight = SpawnSize.X;
		ArenaSceneWorldWidth = SpawnSize.Y;
	}
	if (Player)
	{
		Player->ConfigureArenaBounds(ArenaScene->GetArenaCenter(), PlayerHalfExtents);
	}
	if (ArenaCameraActor)
	{
		ArenaCameraActor->Configure(Player, ArenaScene);
	}
	if (EnemyRoster)
	{
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Entry.Host.Get()))
			{
				Enemy->ConfigureGameplayPlane(ArenaScene->GetGameplayPlaneWorldZ());
			}
		}
	}
}

void AReEchoGameMode::PrepareEncounterIntermission()
{
	FReEchoStageTransitionDecision Transition;
	FString Error;
	if (!ResolveNextStageTransition(Transition, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[StageTransition] Intermission rejected: %s"), *Error);
		return;
	}
	const UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvEncounterRow* NextEncounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex + 1) : nullptr;
	const FReEchoCsvStageRow* NextStage =
	    NextEncounter && Snapshot.IsValid() ? Snapshot->FindStage(NextEncounter->StageId) : nullptr;
	if (!NextEncounter || !NextStage || !PrepareArenaSceneForStage(*NextStage, Error))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Intermission preparation rejected StageId=%s SceneId=%s: %s"),
		       NextEncounter ? *NextEncounter->StageId.ToString() : TEXT("None"),
		       NextStage ? *NextStage->SceneId.ToString() : TEXT("None"),
		       *Error);
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
	// 守门式安全网：推进遭遇前若特质卡选择屏仍打开（意外孤儿屏），先关闭并清空 pending 状态，
	// 避免 Phase 已被推进到非 CardChoice 后屏仍可交互却刷新/确认双双失效的软锁。
	if (TraitCardChoiceWidget)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[TraitChoice] BeginNextEncounter invoked while trait choice screen still open (encounter=%d); "
		            "closing orphan screen and clearing pending offers to avoid soft-lock."),
		       GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>()
		           ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>()->EncounterIndex
		           : INDEX_NONE);
		CloseTraitCardChoiceScreen();
	}
	ResetEncounterTransitionPresentation();
	if (PrepareNextEncounter(false))
	{
		ActivatePreparedEncounter();
	}
}

bool AReEchoGameMode::PrepareNextEncounter(const bool bDeferActivation)
{
	bPreparedEncounterAwaitingActivation = false;
	ConnectionLineSideByPair.Reset();
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->EncounterIndex >= RunSubsystem->GetTotalEncounterCount())
	{
		return false;
	}
	FReEchoStageTransitionDecision Transition;
	FString TransitionError;
	if (!ResolveNextStageTransition(Transition, TransitionError))
	{
		UE_LOG(LogTemp, Error, TEXT("[StageTransition] Next encounter rejected: %s"), *TransitionError);
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> PreBeginSnapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* NextEncounter =
	    PreBeginSnapshot.IsValid() ? PreBeginSnapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex + 1) : nullptr;
	const FReEchoCsvStageRow* NextStage =
	    NextEncounter && PreBeginSnapshot.IsValid() ? PreBeginSnapshot->FindStage(NextEncounter->StageId) : nullptr;
	FString SceneError;
	if (!NextEncounter || !NextStage || !PrepareArenaSceneForStage(*NextStage, SceneError))
	{
		const FName StageId = NextEncounter ? NextEncounter->StageId : NAME_None;
		const FName SceneId = NextStage ? NextStage->SceneId : NAME_None;
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Encounter start blocked StageId=%s SceneId=%s: %s"),
		       *StageId.ToString(),
		       *SceneId.ToString(),
		       *SceneError);
		return false;
	}
	ClearTimeShardPickups();
	if (!Transition.bPreserveEnemyRoster)
	{
		ClearEnemyRoster();
	}
	ClearEchoes();
	if (!ApplyArenaSceneForStage(*NextStage, SceneError))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[ArenaScene] Prepared Arena commit failed StageId=%s SceneId=%s: %s"),
		       *NextStage->Id.ToString(),
		       *NextStage->SceneId.ToString(),
		       *SceneError);
		return false;
	}
	bEncounterTransitioning = false;
	bEncounterClearedByDefeat = false;
	bBossSuccessfullySpawnedThisEncounter = false;
	bBossPostEchoPhaseTriggered = false;
	RunSubsystem->BeginEncounter();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
	if (!Encounter || !ConfigureEncounterSpawns(RunSubsystem->EncounterIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Encounter %d could not start from table data."), RunSubsystem->EncounterIndex);
		return false;
	}
	Director->ConfigureEncounter(Encounter->DurationSeconds, Encounter->EndCondition == TEXT("Duration"));
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
	}
	// Plan31: resolve the full selected set (zero, one or several) and spawn one independent
	// Echo actor per recording. Each Echo owns its immutable recording and build snapshot, so its
	// playback, position, weapon and run state stay independent of the others.
	const TArray<FReEchoRecording> Recordings =
	    RunSubsystem->ResolveReplayRecordings(ReEchoTimeAnchor::MaximumResolvedEchoes);
	for (const FReEchoRecording& Recording : Recordings)
	{
		AReEchoEchoActor* Echo = SpawnEchoActor();
		if (Echo && Echo->InitializeEcho(
		                Recording, RunSubsystem->CurrentBuild.Stats.EchoEfficiency, RunSubsystem->GetRunDataSnapshot()))
		{
			Echo->ConfigureCardRules(RunSubsystem->GetCardRules(), RunSubsystem->CurrentBuild.Stats);
			if (bDeferActivation)
			{
				Echo->PrepareDeferredBornReveal(0.0f);
			}
			Echoes.Add(Echo);
		}
		else if (Echo)
		{
			Echo->Destroy();
		}
	}
	RefreshFogRevealSources();
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[StageTransition] prepared previous=%s next=%s keepRoster=%s keepPlayerLocation=%s roster=%d "
	            "player=(%.1f,%.1f,%.1f)"),
	       *Transition.PreviousStageId.ToString(),
	       *Transition.NextStageId.ToString(),
	       Transition.bPreserveEnemyRoster ? TEXT("true") : TEXT("false"),
	       Transition.bPreservePlayerLocation ? TEXT("true") : TEXT("false"),
	       EnemyRoster->GetLivingEnemyCount(),
	       PlayerLocation.X,
	       PlayerLocation.Y,
	       PlayerLocation.Z);
	bPreparedEncounterAwaitingActivation = true;
	return true;
}

void AReEchoGameMode::ActivatePreparedEncounter()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!bPreparedEncounterAwaitingActivation || !RunSubsystem || RunSubsystem->Phase != EReEchoRunPhase::Encounter ||
	    !Director)
	{
		UE_LOG(LogReEcho, Error, TEXT("[StageTransition] prepared encounter activation rejected."));
		return;
	}
	CompleteStage01To02EchoReveal();
	bPreparedEncounterAwaitingActivation = false;
	SetEncounterTransitionWorldPaused(false);
	if (Player && Player->Recorder)
	{
		Player->Recorder->BeginRecording(RunSubsystem->EncounterIndex,
		                                 TEXT("GrayboxArena"),
		                                 1337 + RunSubsystem->EncounterIndex,
		                                 RunSubsystem->CurrentBuild);
	}
	SetMusicState(IsBossEncounter() ? FReEchoAudioEvents::MusicBoss : FReEchoAudioEvents::MusicEncounter);
	SetEnemyEncounterSimulationSuspended(false);
	RestoreGameInput();
	GrantPostEntryInvulnerability(RunSubsystem->EncounterIndex);
	Director->StartEncounter();
	ProcessScheduledSpawnEvents(0.0f);
	GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::CaptureActiveSaveSlotPreview);
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[StageTransition] activated encounter=%d echoes=%d afterCamera=%s"),
	       RunSubsystem->EncounterIndex,
	       Echoes.Num(),
	       EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Completed ? TEXT("true")
	                                                                                                : TEXT("false"));
}

void AReEchoGameMode::CaptureActiveSaveSlotPreview()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->GetActiveSaveSlotIndex() == INDEX_NONE || !GEngine || !GEngine->GameViewport ||
	    !GEngine->GameViewport->Viewport)
	{
		return;
	}
	TArray<FColor> Bitmap;
	if (!GetViewportScreenShot(GEngine->GameViewport->Viewport, Bitmap) || Bitmap.IsEmpty())
	{
		UE_LOG(LogReEcho, Warning, TEXT("Save preview capture failed: viewport returned no pixels."));
		return;
	}
	const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	TArray<uint8> PngBytes;
	FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, PngBytes);
	if (!RunSubsystem->WriteActiveSaveSlotPreview(PngBytes))
	{
		UE_LOG(LogReEcho, Warning, TEXT("Save preview capture failed while writing PNG."));
	}
}

bool AReEchoGameMode::ShouldGrantPostEntryInvulnerability(const int32 EncounterIndex, const float DurationSeconds)
{
	return EncounterIndex > 0 && DurationSeconds > 0.0f;
}

float AReEchoGameMode::ResolvePostEntryInvulnerabilitySeconds() const
{
	const UReEchoEncounterFlowSettings* Settings =
	    EncounterFlowSettingsClass ? EncounterFlowSettingsClass->GetDefaultObject<UReEchoEncounterFlowSettings>()
	                               : GetDefault<UReEchoEncounterFlowSettings>();
	return Settings ? FMath::Max(0.0f, Settings->PostEntryInvulnerabilitySeconds) : 0.0f;
}

void AReEchoGameMode::GrantPostEntryInvulnerability(const int32 EncounterIndex)
{
	const float DurationSeconds = ResolvePostEntryInvulnerabilitySeconds();
	if (!ShouldGrantPostEntryInvulnerability(EncounterIndex, DurationSeconds) || !Player || !Player->Combatant ||
	    !GetWorld())
	{
		return;
	}

	Player->Combatant->GrantTimedInvulnerability(GetWorld()->GetTimeSeconds(), DurationSeconds);
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[EncounterEntryProtection] encounter=%d duration=%.3f player=%s"),
	       EncounterIndex,
	       DurationSeconds,
	       *GetNameSafe(Player));
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
	Result.PlayerHealth = Player->Combatant->GetEffectiveCurrentHealth();
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

	bEncounterTransitioning = false;
	bEncounterClearedByDefeat = false;
	bBossSuccessfullySpawnedThisEncounter = false;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = RunSubsystem->GetRunDataSnapshot();
	const FReEchoCsvEncounterRow* Encounter =
	    Snapshot.IsValid() ? Snapshot->FindEncounterByIndex(RunSubsystem->EncounterIndex) : nullptr;
	const FReEchoCsvStageRow* Stage =
	    Encounter && Snapshot.IsValid() ? Snapshot->FindStage(Encounter->StageId) : nullptr;
	FString SceneError;
	if (!Encounter || !Stage || !PrepareArenaSceneForStage(*Stage, SceneError))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("Saved encounter %d has no valid table/Arena configuration StageId=%s SceneId=%s: %s"),
		       RunSubsystem->EncounterIndex,
		       Encounter ? *Encounter->StageId.ToString() : TEXT("None"),
		       Stage ? *Stage->SceneId.ToString() : TEXT("None"),
		       *SceneError);
		return;
	}
	const FReEchoEncounterRuntimeState SavedState = RunSubsystem->ConsumePendingEncounterResume();
	bBossPostEchoPhaseTriggered = SavedState.bBossPostEchoPhaseTriggered;
	ClearCombatants();
	if (!ApplyArenaSceneForStage(*Stage, SceneError) || !ConfigureEncounterSpawns(RunSubsystem->EncounterIndex))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("Saved encounter %d failed after Arena preparation StageId=%s SceneId=%s: %s"),
		       RunSubsystem->EncounterIndex,
		       *Stage->Id.ToString(),
		       *Stage->SceneId.ToString(),
		       *SceneError);
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
	Player->Combatant->SetOverhealCapacityFraction(RunSubsystem->GetCardRules().bOverhealCapacity ? 0.3f : 0.0f);
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
		    RunSubsystem->ResolveReplayRecordings(ReEchoTimeAnchor::MaximumResolvedEchoes);
		for (const FReEchoRecording& Recording : Recordings)
		{
			AReEchoEchoActor* Echo = SpawnEchoActor();
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
		                      : SavedKind == EReEchoEnemyKind::Boss   ? FName(TEXT("M_SHEEP"))
		                      : SavedKind == EReEchoEnemyKind::Slime  ? FName(TEXT("M_SLIME"))
		                      : SavedKind == EReEchoEnemyKind::Ranged ? FName(TEXT("M_RABBIT"))
		                      : SavedKind == EReEchoEnemyKind::Elite  ? FName(TEXT("M_FOX"))
		                      : SavedKind == EReEchoEnemyKind::Bomber ? FName(TEXT("M_Bomber"))
		                      : SavedKind == EReEchoEnemyKind::Shield ? FName(TEXT("M_Shield"))
		                                                              : FName(TEXT("M_Grunt"));
		FReEchoEnemyDefinition Definition;
		FString CompileError;
		if (!DataSnapshot || !ReEchoEnemyDefinitionCompiler::Compile(
		                         *DataSnapshot, EnemyId, Definition, CompileError, RunSubsystem->EncounterIndex))
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
		ConfigureEnemyRuntimeBindings(Enemy);
		if (Definition.Archetype == EReEchoEnemyArchetype::Boss)
		{
			bBossSuccessfullySpawnedThisEncounter = true;
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
		if (Event.EnemyRole == TEXT("Boss"))
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[BossVictoryTrace][SpawnEvent] type=%s wave=%s enemy=%s processTime=%.3f "
			            "eventTime=%.3f spawnTime=%.3f scheduler=%d/%d"),
			       Event.Type == EReEchoScheduledSpawnEventType::Warning ? TEXT("Warning") : TEXT("Commit"),
			       *Event.WaveId.ToString(),
			       *Event.EnemyId.ToString(),
			       EncounterSeconds,
			       Event.EventSeconds,
			       Event.SpawnSeconds,
			       EncounterWaveScheduler.GetNextEventIndex(),
			       EncounterWaveScheduler.GetEventCount());
		}
		if (Event.Type == EReEchoScheduledSpawnEventType::Warning)
		{
			PrepareScheduledSpawnBatch(Event);
			const FReEchoPendingSpawnBatchState* Pending = PendingSpawnBatches.FindByPredicate(
			    [&Event](const FReEchoPendingSpawnBatchState& Candidate)
			    {
				    return Candidate.WaveId == Event.WaveId && Candidate.EnemyRole == Event.EnemyRole;
			    });
			UE_LOG(LogTemp,
			       Display,
			       TEXT("[EncounterSpawn] warning wave=%s role=%s requested=%d reserved=%d spawn=%.2f"),
			       *Event.WaveId.ToString(),
			       *Event.EnemyRole.ToString(),
			       Event.Count,
			       Pending ? Pending->Locations.Num() : 0,
			       Event.SpawnSeconds);
			if (Pending && GetWorld())
			{
				const float FixedStepGraceSeconds =
				    1.0f / FMath::Max(1.0f, GetDefault<UReEchoBalanceSettings>()->FixedStepHz);
				const float DisplaySeconds =
				    FMath::Max(0.15f, Event.SpawnSeconds - Event.EventSeconds) + FixedStepGraceSeconds;
				for (const FVector& Location : Pending->Locations)
				{
					DrawDebugSphere(GetWorld(), Location, 65.0f, 12, FColor::Red, false, DisplaySeconds, 0, 5.0f);
				}
			}
			continue;
		}
		SpawnScheduledBatch(Event);
	}
}

void AReEchoGameMode::PrepareScheduledSpawnBatch(const FReEchoScheduledSpawnEvent& Event)
{
	if (PendingSpawnBatches.ContainsByPredicate(
	        [&Event](const FReEchoPendingSpawnBatchState& Candidate)
	        {
		        return Candidate.WaveId == Event.WaveId && Candidate.EnemyRole == Event.EnemyRole;
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
	const FReEchoCsvEnemyRow* Enemy = Snapshot.IsValid() ? Snapshot->FindEnemy(Event.EnemyId) : nullptr;
	if (!Snapshot.IsValid() || !Encounter || !Policy || !Profile || !Enemy || !Player)
	{
		UE_LOG(LogTemp, Error, TEXT("[EncounterSpawn] warning rejected because runtime data is unavailable."));
		return;
	}
	const float GameplayPlaneWorldZ = ArenaScene ? ArenaScene->GetGameplayPlaneWorldZ() : 0.0f;
	const float SpawnCenterWorldZ = GameplayPlaneWorldZ + Enemy->CollisionHalfHeightCm;
	int32 LivingCount = 0;
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		LivingCount +=
		    Entry.bAlive && (Encounter->bBossCountsTowardUnitLimit || Entry.Archetype != EReEchoEnemyArchetype::Boss)
		        ? 1
		        : 0;
	}
	int32 ReservedCount = 0;
	for (const FReEchoPendingSpawnBatchState& ExistingBatch : PendingSpawnBatches)
	{
		if (!Encounter->bBossCountsTowardUnitLimit && ExistingBatch.EnemyRole == TEXT("Boss"))
		{
			continue;
		}
		ReservedCount += ExistingBatch.Locations.Num();
	}
	const bool bCountsTowardUnitLimit = Event.EnemyRole != TEXT("Boss") || Encounter->bBossCountsTowardUnitLimit;
	const int32 ReservationCount = ReEchoSpawnCapacity::CalculateReservationCount(
	    Encounter->ActiveUnitLimit, LivingCount, ReservedCount, Event.Count, bCountsTowardUnitLimit);
	if (ReservationCount < Event.Count)
	{
		UE_LOG(LogTemp,
		       Display,
		       TEXT("[EncounterSpawn] warning wave=%s role=%s reserved %d->%d by active unit limit %d."),
		       *Event.WaveId.ToString(),
		       *Event.EnemyRole.ToString(),
		       Event.Count,
		       ReservationCount,
		       Encounter->ActiveUnitLimit);
	}

	FReEchoPendingSpawnBatchState Pending;
	Pending.WaveId = Event.WaveId;
	Pending.EnemyRole = Event.EnemyRole;
	Pending.EnemyId = Event.EnemyId;
	FBox2D SpawnWorldBounds(ForceInit);
	FString SpawnBoundsError;
	if (!ArenaScene || !ArenaScene->GetEnemySpawnWorldBounds(SpawnWorldBounds, &SpawnBoundsError))
	{
		UE_LOG(LogTemp,
		       Error,
		       TEXT("[EncounterSpawn] warning wave=%s role=%s SceneId=%s rejected: invalid wall-derived bounds: %s"),
		       *Event.WaveId.ToString(),
		       *Event.EnemyRole.ToString(),
		       *ActiveArenaSceneId.ToString(),
		       *SpawnBoundsError);
		return;
	}
	for (int32 Index = 0; Index < ReservationCount; ++Index)
	{
		FReEchoSpawnResolveRequest Request;
		Request.PlayerAnchor = Player->GetActorLocation() + Player->GetVelocity() * Policy->AnchorLeadSeconds;
		Request.PlayerAnchor.X = FMath::Clamp(Request.PlayerAnchor.X, SpawnWorldBounds.Min.X, SpawnWorldBounds.Max.X);
		Request.PlayerAnchor.Y = FMath::Clamp(Request.PlayerAnchor.Y, SpawnWorldBounds.Min.Y, SpawnWorldBounds.Max.Y);
		Request.bHasEchoAnchor = Echoes.Num() > 0 && IsValid(Echoes[0]);
		Request.EchoAnchor = Request.bHasEchoAnchor
		                         ? Echoes[0]->EvaluateRecordedPosition(Event.SpawnSeconds + Policy->AnchorLeadSeconds)
		                         : FVector::ZeroVector;
		Request.EchoAnchorRatio = Encounter->EchoAnchorRatio;
		Request.SpawnWorldBounds = SpawnWorldBounds;
		Request.SpawnCenterWorldZ = SpawnCenterWorldZ;
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

	int32 SuccessCount = 0;
	for (int32 Index = 0; Index < Pending.Locations.Num(); ++Index)
	{
		if (SpawnConfiguredEnemy(Pending.EnemyId, Pending.Locations[Index], RunSubsystem->EncounterIndex))
		{
			++SuccessCount;
		}
		else
		{
			UE_LOG(LogTemp,
			       Error,
			       TEXT("[EncounterSpawn] committed warning failed wave=%s role=%s index=%d enemy=%s."),
			       *Event.WaveId.ToString(),
			       *Event.EnemyRole.ToString(),
			       Index,
			       *Pending.EnemyId.ToString());
		}
	}
	if (Pending.EnemyRole == TEXT("Boss"))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[BossVictoryTrace][BossCommit] encounter=%d wave=%s enemy=%s requested=%d success=%d "
		            "rosterAfter=%d encounterTime=%.3f"),
		       RunSubsystem->EncounterIndex,
		       *Pending.WaveId.ToString(),
		       *Pending.EnemyId.ToString(),
		       Pending.Locations.Num(),
		       SuccessCount,
		       EnemyRoster ? EnemyRoster->GetEntries().Num() : -1,
		       Director ? Director->EncounterTime : -1.0f);
		if (SuccessCount == 0)
		{
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("[BossVictoryTrace][BossCommitFailed] encounter=%d wave=%s; victory remains suppressed."),
			       RunSubsystem->EncounterIndex,
			       *Pending.WaveId.ToString());
		}
	}
	PendingSpawnBatches.RemoveAt(PendingIndex);
}

bool AReEchoGameMode::SpawnConfiguredEnemy(const FName EnemyId, const FVector& SpawnLocation, int32 CombatIndex)
{
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunSubsystem ? RunSubsystem->GetRunDataSnapshot() : nullptr;
	FReEchoEnemyDefinition Definition;
	FString CompileError;
	if (!Snapshot.IsValid() ||
	    !ReEchoEnemyDefinitionCompiler::Compile(*Snapshot, EnemyId, Definition, CompileError, CombatIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy spawn failed for %s: %s"), *EnemyId.ToString(), *CompileError);
		return false;
	}
	const int32 NextSpawnIndex = EnemySpawnIndex + 1;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>(
	    ResolveEnemyClass(Definition.PresentationId), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
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
	ConfigureEnemyRuntimeBindings(Enemy);
	if (Definition.Archetype == EReEchoEnemyArchetype::Boss)
	{
		bBossSuccessfullySpawnedThisEncounter = true;
	}
	const UBoxComponent* RootCollision = Cast<UBoxComponent>(Enemy->GetRootComponent());
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[EncounterSpawn] active enemy=%s spawnIndex=%d world=%.3f encounter=%.3f alive=%s damageable=%s "
	            "actorCollision=%s rootCollision=%s"),
	       *EnemyId.ToString(),
	       NextSpawnIndex,
	       GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f,
	       Director ? Director->EncounterTime : -1.0f,
	       Enemy->IsCombatTargetAlive() ? TEXT("true") : TEXT("false"),
	       Enemy->CanBeDamaged() ? TEXT("true") : TEXT("false"),
	       Enemy->GetActorEnableCollision() ? TEXT("true") : TEXT("false"),
	       RootCollision && RootCollision->IsCollisionEnabled() ? TEXT("true") : TEXT("false"));
	return true;
}

void AReEchoGameMode::ConfigureEnemyRuntimeBindings(AReEchoEnemyActor* Enemy)
{
	if (!Enemy)
	{
		return;
	}
	if (UReEchoEnemyEventsComponent* Events = Enemy->GetEnemyEventsComponent())
	{
		Events->OnBossIntent.AddUniqueDynamic(this, &AReEchoGameMode::HandleBossIntent);
	}
	if (UReEchoCombatEventsComponent* CombatEvents = Enemy->GetCombatEventsComponent())
	{
		CombatEvents->OnDeath.AddUniqueDynamic(this, &AReEchoGameMode::HandleEnemyDeathShardDrop);
		CombatEvents->OnElementReactionResolved.AddUniqueDynamic(this,
		                                                         &AReEchoGameMode::HandleCardElementReactionResolved);
		CombatEvents->OnHurt.AddUniqueDynamic(this, &AReEchoGameMode::HandleRunStatsEnemyHurt);
		CombatEvents->OnDeath.AddUniqueDynamic(this, &AReEchoGameMode::HandleRunStatsEnemyDeath);
		CombatEvents->OnElementReactionResolved.AddUniqueDynamic(this,
		                                                         &AReEchoGameMode::HandleRunStatsElementReaction);
	}
}

void AReEchoGameMode::HandleRunStatsEnemyHurt(const FReEchoDamageEvent& Event)
{
	if (UReEchoRunStatsSubsystem* Stats =
	        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunStatsSubsystem>() : nullptr)
	{
		Stats->RecordEnemyHurt(Event);
	}
}

void AReEchoGameMode::HandleRunStatsEnemyDeath(const FReEchoDamageEvent& Event)
{
	if (UReEchoRunStatsSubsystem* Stats =
	        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunStatsSubsystem>() : nullptr)
	{
		Stats->RecordEnemyDeath(Event);
	}
}

void AReEchoGameMode::HandleRunStatsElementReaction(const FReEchoElementReactionResolvedEvent& Event)
{
	if (UReEchoRunStatsSubsystem* Stats =
	        GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunStatsSubsystem>() : nullptr)
	{
		Stats->RecordElementReaction();
	}
}

void AReEchoGameMode::HandleCardElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event)
{
	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const bool bPlayerOrEchoSource = Cast<AReEchoPlayerPawn>(Event.Attack.Source.Get()) != nullptr ||
	                                 Cast<AReEchoEchoActor>(Event.Attack.Source.Get()) != nullptr;
	if (!RunSubsystem || !bPlayerOrEchoSource)
	{
		return;
	}
	const FReEchoCardRuleSnapshot Rules = RunSubsystem->GetCardRules();
	if (Rules.bConductDamageGrowth && Event.ReactionBehaviorId == TEXT("Reaction.Conduct"))
	{
		TArray<int32> AffectedSpawnIndices;
		for (const AActor* Target : Event.AffectedTargets)
		{
			if (const AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Target))
			{
				AffectedSpawnIndices.Add(Enemy->GetSpawnIndex());
			}
		}
		RunSubsystem->NotifyCardReactionAffectedTargets(Event.ReactionId, AffectedSpawnIndices);
	}
	if (!Rules.bVaporizeWaterSplash || Event.ReactionBehaviorId != TEXT("Reaction.Vaporize") || !Event.PrimaryTarget ||
	    bHandlingVaporizeWaterSplash)
	{
		return;
	}
	TGuardValue<bool> Guard(bHandlingVaporizeWaterSplash, true);
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr;
		if (!Enemy || Enemy == Event.PrimaryTarget ||
		    FVector::DistSquared2D(Enemy->GetActorLocation(), Event.PrimaryTarget->GetActorLocation()) >
		        FMath::Square(300.0f))
		{
			continue;
		}
		FReEchoElementHitContext Context;
		Context.Attack = Event.Attack;
		Context.SourceLocation = Event.PrimaryTarget->GetActorLocation();
		Context.ReactionEfficiency = RunSubsystem->CurrentBuild.Stats.ReactionEfficiency;
		Context.SourceElementalAttack = 0.0f;
		ReEchoHitResolver::ResolveElementHit(*Enemy, EReEchoElement::Water, 0.0f, Context);
	}
}

AReEchoTimeShardPickupActor*
AReEchoGameMode::SpawnTimeShardPickup(const FVector& Location, const int32 Amount, const float LifetimeSeconds)
{
	if (!GetWorld() || Amount <= 0)
	{
		return nullptr;
	}
	const TSubclassOf<AReEchoTimeShardPickupActor> PickupClass =
	    TimeShardPickupClass ? TimeShardPickupClass
	                         : TSubclassOf<AReEchoTimeShardPickupActor>(AReEchoTimeShardPickupActor::StaticClass());
	AReEchoTimeShardPickupActor* Pickup =
	    GetWorld()->SpawnActor<AReEchoTimeShardPickupActor>(PickupClass, Location, FRotator::ZeroRotator);
	if (Pickup)
	{
		Pickup->InitializePickup(Amount, LifetimeSeconds);
	}
	return Pickup;
}

bool AReEchoGameMode::TryGetActiveArenaGameplayPlaneZ(float& OutGameplayPlaneZ) const
{
	if (!IsValid(ArenaScene))
	{
		return false;
	}
	OutGameplayPlaneZ = ArenaScene->GetGameplayPlaneWorldZ();
	return true;
}

void AReEchoGameMode::HandleEnemyDeathShardDrop(const FReEchoDamageEvent& Event)
{
	const AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Event.Target);
	UReEchoRunSubsystem* RunSubsystem =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	if (!Enemy || !RunSubsystem || !GetWorld())
	{
		return;
	}

	const int32 DropAmount = RunSubsystem->ResolveEnemyDeathTimeShardDrop(Enemy->GetEnemyId(), Enemy->GetSpawnIndex());
	if (DropAmount <= 0)
	{
		return;
	}

	if (AReEchoTimeShardPickupActor* Pickup = SpawnTimeShardPickup(Enemy->GetActorLocation(), DropAmount, 0.0f))
	{
		const FVector SpawnLocation = Pickup->GetActorLocation();
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[TimeShardDrop] spawned actor=%s enemy=%s spawn=%d amount=%d location=(%.1f,%.1f,%.1f)"),
		       *Pickup->GetName(),
		       *Enemy->GetEnemyId().ToString(),
		       Enemy->GetSpawnIndex(),
		       DropAmount,
		       SpawnLocation.X,
		       SpawnLocation.Y,
		       SpawnLocation.Z);
	}
	else
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("[TimeShardDrop] spawn failed enemy=%s spawn=%d amount=%d"),
		       *Enemy->GetEnemyId().ToString(),
		       Enemy->GetSpawnIndex(),
		       DropAmount);
	}
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

bool AReEchoGameMode::ShouldCompleteBossEncounter(const bool bBossSuccessfullySpawned, const int32 LivingBossCount)
{
	return bBossSuccessfullySpawned && LivingBossCount <= 0;
}

void AReEchoGameMode::TriggerBossPostEchoPhase(const FReEchoBossPhaseDefinition& PhaseDefinition)
{
	if (bBossPostEchoPhaseTriggered || !IsBossEncounter() || !PhaseDefinition.bEnabled || !Player || !Player->Combatant)
	{
		return;
	}
	bBossPostEchoPhaseTriggered = true;
	if (UReEchoCombatVfxComponent* PlayerVfx = Player->FindComponentByClass<UReEchoCombatVfxComponent>())
	{
		PlayerVfx->ClearEchoConnectionLinks();
	}
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
	if (UReEchoCombatVfxComponent* PlayerVfx = Player->FindComponentByClass<UReEchoCombatVfxComponent>())
	{
		TArray<AActor*> LivingEchoActors;
		for (AReEchoEchoActor* Echo : Echoes)
		{
			if (Echo && Echo->IsCombatTargetAlive())
			{
				LivingEchoActors.Add(Echo);
			}
		}
		PlayerVfx->SyncEchoConnectionLinks(Rules.bConnectionLineDamage, LivingEchoActors);
	}
	Player->Combatant->SetOverhealCapacityFraction(Rules.bOverhealCapacity ? 0.3f : 0.0f);
	if (CardTick.EchoAuraPulseCount > 0)
	{
		PlayEchoCardAuraPulse(Rules);
	}
	if (CardTick.EchoHeadCursePulseCount > 0)
	{
		TArray<AReEchoEnemyActor*> LivingEnemies;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr)
			{
				LivingEnemies.Add(Enemy);
			}
		}
		AReEchoEchoActor* SourceEcho = nullptr;
		for (AReEchoEchoActor* Echo : Echoes)
		{
			if (Echo && Echo->IsCombatTargetAlive())
			{
				SourceEcho = Echo;
				break;
			}
		}
		if (SourceEcho && !LivingEnemies.IsEmpty())
		{
			for (int32 PulseOffset = 0; PulseOffset < CardTick.EchoHeadCursePulseCount; ++PulseOffset)
			{
				FRandomStream Random(HashCombine(RunSubsystem->EncounterIndex,
				                                 CardTick.CardState.Runtime.LastEchoHeadCursePulseIndex - PulseOffset));
				AReEchoEnemyActor* Target = LivingEnemies[Random.RandRange(0, LivingEnemies.Num() - 1)];
				FReEchoTimedStatusCommand Curse;
				Curse.StatusId = TEXT("Z_Cursed");
				Curse.CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
				Curse.DurationSeconds = 10.0f;
				Curse.Attack.Source = SourceEcho;
				Curse.Attack.Sequence = CardTick.CardState.Runtime.LastEchoHeadCursePulseIndex - PulseOffset;
				Target->GetCombatantComponent()->ApplyTimedStatus(Curse);
			}
		}
	}
	if (CardTick.EasterRandomStunPulseCount > 0 && CardTick.EasterRandomStunRadiusCm > 0.0f)
	{
		TArray<AReEchoEnemyActor*> NearbyEnemies;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr;
			if (Enemy && FVector::Dist2D(Player->GetActorLocation(), Enemy->GetActorLocation()) <=
			                 CardTick.EasterRandomStunRadiusCm)
			{
				NearbyEnemies.Add(Enemy);
			}
		}
		for (int32 PulseOffset = 0; PulseOffset < CardTick.EasterRandomStunPulseCount && !NearbyEnemies.IsEmpty();
		     ++PulseOffset)
		{
			const int32 PulseIndex = CardTick.CardState.Runtime.LastEasterStunPulseIndex - PulseOffset;
			FRandomStream Random(RunSubsystem->BuildCardEffectRandomSeed(TEXT("G_4_5_STUN_TARGET"), PulseIndex));
			AReEchoEnemyActor* Target = NearbyEnemies[Random.RandRange(0, NearbyEnemies.Num() - 1)];
			FReEchoTimedStatusCommand Stun;
			Stun.StatusId = TEXT("Z_Stun");
			Stun.CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			Stun.DurationSeconds = CardTick.EasterRandomStunDuration;
			Stun.Attack.Source = Player;
			Stun.Attack.Sequence = PulseIndex;
			Target->GetCombatantComponent()->ApplyTimedStatus(Stun);
		}
	}
	if (Rules.bEasterEchoContact)
	{
		TSet<uint64> CurrentContacts;
		for (AReEchoEchoActor* Echo : Echoes)
		{
			if (!Echo || !Echo->IsCombatTargetAlive())
			{
				continue;
			}
			const uint64 EchoKey = static_cast<uint64>(static_cast<uint32>(Echo->GetUniqueID())) << 32;
			const uint64 PlayerPair = EchoKey | 0xffffffffu;
			if (Player->IsCombatTargetAlive() && FVector::DistSquared2D(Echo->GetActorLocation(), Player->GetActorLocation()) <=
			                                         FMath::Square(100.0f))
			{
				CurrentContacts.Add(PlayerPair);
				if (!ActiveEasterEchoContactPairs.Contains(PlayerPair))
				{
					Player->Combatant->ApplyHealing(Rules.EasterEchoContactHealing);
				}
			}
			for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
			{
				AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr;
				if (!Enemy || FVector::DistSquared2D(Echo->GetActorLocation(), Enemy->GetActorLocation()) >
				                  FMath::Square(100.0f))
				{
					continue;
				}
				const uint64 PairKey = EchoKey | static_cast<uint32>(Enemy->GetUniqueID());
				CurrentContacts.Add(PairKey);
				if (!ActiveEasterEchoContactPairs.Contains(PairKey))
				{
					FReEchoHitIntent ContactHit;
					ContactHit.Attack.Source = Echo;
					ContactHit.Attack.Sequence = HashCombine(Echo->GetUniqueID(), Enemy->GetUniqueID());
					ContactHit.Target = Enemy;
					ContactHit.RawDamage = Rules.EasterEchoContactDamage;
					ContactHit.DamageSource = EReEchoDamageSource::Echo;
					ContactHit.SourceLocation = Echo->GetActorLocation();
					ContactHit.HitLocation = Enemy->GetActorLocation();
					ReEchoHitResolver::ResolvePhysicalHit(ContactHit);
				}
			}
		}
		ActiveEasterEchoContactPairs = MoveTemp(CurrentContacts);
	}
	else
	{
		ActiveEasterEchoContactPairs.Reset();
	}
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
	{
		AReEchoEnemyActor* Enemy = Entry.bAlive ? Cast<AReEchoEnemyActor>(Entry.Host.Get()) : nullptr;
		if (!Enemy)
		{
			continue;
		}
		if (Rules.bConnectionLineDamage && Player->IsCombatTargetAlive())
		{
			for (AReEchoEchoActor* Echo : Echoes)
			{
				if (!Echo || !Echo->IsCombatTargetAlive())
				{
					continue;
				}
				const FVector2D Start(Player->GetActorLocation());
				const FVector2D End(Echo->GetActorLocation());
				const FVector2D EnemyPoint(Enemy->GetActorLocation());
				const FVector2D Segment = End - Start;
				const float LengthSquared = Segment.SizeSquared();
				if (LengthSquared <= KINDA_SMALL_NUMBER)
				{
					continue;
				}
				const float Projection = FVector2D::DotProduct(EnemyPoint - Start, Segment) / LengthSquared;
				const float Cross = Segment.X * (EnemyPoint.Y - Start.Y) - Segment.Y * (EnemyPoint.X - Start.X);
				const int8 CurrentSide = Cross > 1.0f ? 1 : (Cross < -1.0f ? -1 : 0);
				const uint64 PairKey = (static_cast<uint64>(static_cast<uint32>(Echo->GetUniqueID())) << 32) |
				                       static_cast<uint32>(Enemy->GetUniqueID());
				int8& PreviousSide = ConnectionLineSideByPair.FindOrAdd(PairKey);
				if (CurrentSide != 0 && PreviousSide != 0 && CurrentSide != PreviousSide && Projection >= 0.0f &&
				    Projection <= 1.0f)
				{
					FReEchoHitIntent ConnectionHit;
					ConnectionHit.Attack.Source = Player;
					ConnectionHit.Attack.Sequence = HashCombine(Echo->GetUniqueID(), Enemy->GetUniqueID());
					ConnectionHit.Target = Enemy;
					ConnectionHit.RawDamage = 0.5f * (RunSubsystem->CurrentBuild.Stats.PhysicalAttack +
					                                  RunSubsystem->CurrentBuild.Stats.ElementalAttack);
					ConnectionHit.DamageSource = EReEchoDamageSource::Path;
					ConnectionHit.SourceLocation = Player->GetActorLocation();
					ConnectionHit.HitLocation = Enemy->GetActorLocation();
					ReEchoHitResolver::ResolvePhysicalHit(ConnectionHit);
				}
				if (CurrentSide != 0)
				{
					PreviousSide = CurrentSide;
				}
			}
		}
		for (const float StunDuration : CardTick.EnemyStunDurations)
		{
			Enemy->ApplyCardStun(StunDuration);
		}
		if (Rules.bEchoBody)
		{
			for (AReEchoEchoActor* Echo : Echoes)
			{
				if (!Echo || !Echo->IsCombatTargetAlive())
				{
					continue;
				}
				const float RadiusCm = Rules.bEchoTrinityComplete ? 400.0f : 100.0f;
				if (FVector::DistSquared2D(Echo->GetActorLocation(), Enemy->GetActorLocation()) <=
				    FMath::Square(RadiusCm))
				{
					Enemy->ApplyCardStun(Rules.bEchoTrinityComplete ? 0.2f : 2.0f);
					break;
				}
			}
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

void AReEchoGameMode::PlayEchoCardAuraPulse(const FReEchoCardRuleSnapshot& Rules)
{
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (Echo)
		{
			Echo->PlayCardAuraPulse(Rules);
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
	ResetEncounterTransitionPresentation();
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
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const int32 OwnedCardCount = RunSubsystem ? RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Num() : 0;
	// Both terminal surfaces show the same owned-card icons; the pause surface intentionally shows none.
	const TArray<FReEchoShopOffer> SettlementCards =
	    RunSubsystem ? RunSubsystem->GetOwnedBuildCardView() : TArray<FReEchoShopOffer>();
	if (bVictoryScreen)
	{
		RestartWidget->SetVictoryScreen(RunSubsystem ? RunSubsystem->TimeShards : 0,
		                                OwnedCardCount,
		                                RunSubsystem ? RunSubsystem->CurrentBuild.CharacterId : NAME_None,
		                                SettlementCards);
	}
	else
	{
		RestartWidget->SetDeathScreen(bDeathScreen,
		                              RunSubsystem ? RunSubsystem->EncounterIndex : 0,
		                              RunSubsystem ? RunSubsystem->TimeShards : 0,
		                              OwnedCardCount,
		                              RunSubsystem ? RunSubsystem->CurrentBuild.CharacterId : NAME_None,
		                              bDeathScreen ? SettlementCards : TArray<FReEchoShopOffer>());
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
	if (InventoryShopWidget)
	{
		bPauseOpenedOverInventoryShop = true;
		ShowRestartScreen(false);
		if (!RestartWidget)
		{
			bPauseOpenedOverInventoryShop = false;
		}
		return;
	}
	bPauseOpenedOverInventoryShop = false;
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
	InventoryShopWidget->OnCardPackRequested.AddUObject(this, &AReEchoGameMode::HandleShopCardPackRequested);
	InventoryShopWidget->OnWeaponEquipRequested.AddUObject(this, &AReEchoGameMode::HandleShopWeaponEquipRequested);
	InventoryShopWidget->OnOwnedPartEquipRequested.AddUObject(
	    this, &AReEchoGameMode::HandleShopOwnedPartEquipRequested);
	InventoryShopWidget->OnRefreshRequested.AddUObject(this, &AReEchoGameMode::HandleShopRefreshRequested);
	if (Mode == EReEchoInventoryShopMode::PostTraitIntermission)
	{
		bPostTraitShopClosing = false;
		InventoryShopWidget->OnEchoStoreRequested.AddUObject(this, &AReEchoGameMode::HandleEchoStoreRequested);
		InventoryShopWidget->OnEchoSkipRequested.AddUObject(this, &AReEchoGameMode::HandleEchoSkipRequested);
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
	bPauseOpenedOverInventoryShop = false;
	CloseShopCardChoice(false);
	PostUiEvent(FReEchoAudioEvents::UiCancel);
	if (bPostTraitIntermission)
	{
		bPostTraitShopClosing = true;
	}
	// 守门式（根因修复）：若商店内刚触发了 Sage 额外特质卡选择，Run 阶段仍为 CardChoice（额外选择待解），
	// 则下一 tick 的 ShowTraitCardChoice 会弹出特质卡屏。此刻若直接推进遭遇，会与特质卡屏的弹出竞态，
	// 把 Phase 推进到 Encounter 而屏仍打开——孤儿屏导致刷新/确认双双失效软锁。
	// 因此当额外选择待解（Phase == CardChoice）时，仅关闭商店、不推进遭遇，先让特质卡屏解完再续流程。
	const bool bTraitChoicePending = (RunSubsystem && RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
	                                 || bReturnToOpenShopAfterTraitChoice;
	const bool bShouldStartNextEncounter = bContinueRunAfterShop && !bTraitChoicePending;
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

void AReEchoGameMode::HandleShopOwnedPartEquipRequested(const FName PartId, const int32 OccurrenceIndex)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	// 背包重装：从符文背包点选已拥有的配件，只切换装备，不扣钱、不走购买事务。
	// 独立通道，避免与"重复购买 I 级符文以合成"的购买意图混淆。
	const bool bWasEquipped = RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
	    [PartId](const FReEchoEquippedPartSnapshot& Part)
	    {
		    return Part.PartId == PartId;
	    });
	FString EquipError;
	// Equip into the exact occurrence the player clicked so the twin slot stays untouched.
	if (!RunSubsystem->TryEquipPurchasedPartAt(PartId, OccurrenceIndex, EquipError))
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[ReEchoShop] Owned part '%s' could not be re-equipped: %s"),
		       *PartId.ToString(),
		       *EquipError);
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	if (!bWasEquipped)
	{
		PostUiEvent(FReEchoAudioEvents::UiEquip);
	}
	RunSubsystem->SaveRun();
	RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
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
	// 例外：I 级武器符文可以重复购买来合成更高级，必须走真正的购买事务（扣碎片、累计份数、触发合成），
	// 不能被这里的免费重装分支吞掉。
	if (!RunSubsystem->IsShopOfferRepeatPurchasable(ItemId) && RunSubsystem->OwnedPartIds.Contains(ItemId))
	{
		const bool bWasEquipped = RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
		    [ItemId](const FReEchoEquippedPartSnapshot& Part)
		    {
			    return Part.PartId == ItemId;
		    });
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
		if (!bWasEquipped)
		{
			PostUiEvent(FReEchoAudioEvents::UiEquip);
		}
		RunSubsystem->SaveRun();
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
		return;
	}

	// 新购统一走结构化事务；成功、拒绝、购买前后状态都由同一接口审计。
	TSet<FName> EquippedPartsBeforePurchase;
	for (const FReEchoEquippedPartSnapshot& Part : RunSubsystem->CurrentBuild.EquippedParts)
	{
		EquippedPartsBeforePurchase.Add(Part.PartId);
	}
	const FReEchoShopPurchaseOutcome PurchaseOutcome = RunSubsystem->PurchaseShopItemDetailed(ItemId);
	if (PurchaseOutcome.IsSuccess())
	{
		PostUiEvent(FReEchoAudioEvents::UiPurchase);
		const bool bEquippedPartsChanged =
		    RunSubsystem->CurrentBuild.EquippedParts.Num() != EquippedPartsBeforePurchase.Num() ||
		    RunSubsystem->CurrentBuild.EquippedParts.ContainsByPredicate(
		        [&EquippedPartsBeforePurchase](const FReEchoEquippedPartSnapshot& Part)
		        {
			        return !EquippedPartsBeforePurchase.Contains(Part.PartId);
		        });
		if (bEquippedPartsChanged)
		{
			PostUiEvent(FReEchoAudioEvents::UiEquip);
		}
		RunSubsystem->SaveRun();
		// The authoritative page is stable for EncounterIndex + ShopRefreshSequence. Rebuilding the complete read-only
		// projection updates ownership, backpack and equipped state without rerolling any remaining offer.
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
	}
	else
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoShop] Purchase request rejected tx=%s item=%s code=%d detail=%s"),
		       *PurchaseOutcome.TransactionId,
		       *ItemId.ToString(),
		       static_cast<int32>(PurchaseOutcome.Result),
		       *PurchaseOutcome.Detail);
		PostUiEvent(FReEchoAudioEvents::UiError);
	}
}

void AReEchoGameMode::HandleShopCardPackRequested(const int32 Tier)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ReEchoUIInteractionAudit::Write(TEXT("CARD_PACK_OPEN_REQUEST"),
	                                FString::Printf(TEXT("tier=%d run=%d player=%d shop=%d existingTraitChoice=%d"),
	                                                Tier,
	                                                RunSubsystem ? 1 : 0,
	                                                PlayerController ? 1 : 0,
	                                                InventoryShopWidget ? 1 : 0,
	                                                TraitCardChoiceWidget ? 1 : 0));
	if (!RunSubsystem || !PlayerController || !InventoryShopWidget || TraitCardChoiceWidget)
	{
		ReEchoUIInteractionAudit::Write(TEXT("CARD_PACK_OPEN_REJECTED"),
		                                FString::Printf(TEXT("tier=%d reason=InvalidRuntimeState"), Tier));
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	FReEchoWeaponPartShopView ShopView = RunSubsystem->GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* Pack = ShopView.CardPackOffers.FindByPredicate(
	    [Tier](const FReEchoShopCardPackOffer& Candidate)
	    {
		    return Candidate.Tier == Tier;
	    });
	if (!Pack || !Pack->CanOpenChoices())
	{
		ReEchoUIInteractionAudit::Write(TEXT("CARD_PACK_OPEN_REJECTED"),
		                                FString::Printf(TEXT("tier=%d reason=%s status=%d candidates=%d"),
		                                                Tier,
		                                                Pack ? TEXT("PackUnavailable") : TEXT("PackMissing"),
		                                                Pack ? static_cast<int32>(Pack->Status) : INDEX_NONE,
		                                                Pack ? Pack->Choices.Num() : 0));
		PostUiEvent(FReEchoAudioEvents::UiError);
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
		return;
	}
	if (Pack->Status == EReEchoShopCardPackStatus::Available)
	{
		const FReEchoShopPurchaseOutcome Payment = RunSubsystem->PurchaseShopCardPackDetailed(Tier);
		if (!Payment.IsSuccess())
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[ReEchoShop] Card-pack payment rejected tx=%s tier=%d code=%d detail=%s"),
			       *Payment.TransactionId,
			       Tier,
			       static_cast<int32>(Payment.Result),
			       *Payment.Detail);
			PostUiEvent(FReEchoAudioEvents::UiError);
			RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
			return;
		}
		PostUiEvent(FReEchoAudioEvents::UiPurchase);
		RunSubsystem->SaveRun();
		ShopView = RunSubsystem->GetWeaponPartShopView();
		Pack = ShopView.CardPackOffers.FindByPredicate(
		    [Tier](const FReEchoShopCardPackOffer& Candidate)
		    {
			    return Candidate.Tier == Tier;
		    });
		if (!Pack || Pack->Status != EReEchoShopCardPackStatus::PaidPendingChoice)
		{
			PostUiEvent(FReEchoAudioEvents::UiError);
			RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
			return;
		}
	}

	TArray<FReEchoShopCardChoiceOffer> EffectiveChoices = Pack->Choices;
	UReEchoUIFlowCoordinatorSubsystem* UIFlow = GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>();
	TraitCardChoiceWidget = UIFlow ? Cast<UReEchoTraitCardChoiceWidget>(
	                                     UIFlow->OpenScreen(PlayerController, EReEchoUIScreen::TraitChoice, true, true))
	                               : nullptr;
	if (!TraitCardChoiceWidget)
	{
		ReEchoUIInteractionAudit::Write(TEXT("CARD_PACK_OPEN_REJECTED"),
		                                FString::Printf(TEXT("tier=%d reason=TraitChoiceScreenCreationFailed"), Tier));
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	ActiveShopCardPackTier = Tier;
	// Cadence abilities can let this pack hand out two cards; set it before the offers are revealed.
	TraitCardChoiceWidget->SetSelectableCount(RunSubsystem->GetPaidShopCardPackSelectableCount());
	TraitCardChoiceWidget->InitializeShopOffers(EffectiveChoices, RunSubsystem->TimeShards, Tier);
	PostUiEvent(FReEchoAudioEvents::UiCardReveal);
	TraitCardChoiceWidget->OnShopCardSelected.AddDynamic(this, &AReEchoGameMode::HandleShopCardSelected);
	TraitCardChoiceWidget->OnShopCardChoicesSelected.AddDynamic(
	    this, &AReEchoGameMode::HandleShopCardChoicesSelected);
	TraitCardChoiceWidget->OnCardSlotRefreshRequested.AddDynamic(this,
	                                                             &AReEchoGameMode::HandleShopCardRefreshRequested);
	TraitCardChoiceWidget->OnShopChoiceCancelled.AddDynamic(this, &AReEchoGameMode::HandleShopCardChoiceCancelled);
	SetPlayerMenuAbilityBlocked(true);
	TArray<FString> CandidateIds;
	for (const FReEchoShopCardChoiceOffer& Choice : EffectiveChoices)
	{
		CandidateIds.Add(Choice.CardId.ToString());
	}
	ReEchoUIInteractionAudit::Write(TEXT("CARD_PACK_OPENED"),
	                                FString::Printf(TEXT("tier=%d widget=%s candidates=%d ids=[%s]"),
	                                                Tier,
	                                                *TraitCardChoiceWidget->GetName(),
	                                                EffectiveChoices.Num(),
	                                                *FString::Join(CandidateIds, TEXT(","))));
}

void AReEchoGameMode::HandleShopCardChoicesSelected(const TArray<FName>& ItemIds)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget || !TraitCardChoiceWidget || ActiveShopCardPackTier <= 0)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	const FReEchoShopPurchaseOutcome Outcome = RunSubsystem->ClaimPaidShopCardChoices(ItemIds);
	if (!Outcome.IsSuccess())
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoShop] Card-pack multi-claim rejected tx=%s tier=%d code=%d detail=%s"),
		       *Outcome.TransactionId,
		       ActiveShopCardPackTier,
		       static_cast<int32>(Outcome.Result),
		       *Outcome.Detail);
		PostUiEvent(FReEchoAudioEvents::UiError);
		TraitCardChoiceWidget->RestoreChoiceFailure(RunSubsystem->TimeShards);
		return;
	}
	RunSubsystem->SaveRun();
	// Every pick was already granted inside the claim, so no bonus follow-up choice can be pending here.
	CloseShopCardChoice(true);
	RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
}

void AReEchoGameMode::HandleShopCardSelected(const FName ItemId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget || !TraitCardChoiceWidget || ActiveShopCardPackTier <= 0)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	const FReEchoShopPurchaseOutcome Outcome = RunSubsystem->ClaimPaidShopCardChoice(ItemId);
	if (!Outcome.IsSuccess())
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoShop] Card-pack purchase rejected tx=%s item=%s tier=%d code=%d detail=%s"),
		       *Outcome.TransactionId,
		       *ItemId.ToString(),
		       ActiveShopCardPackTier,
		       static_cast<int32>(Outcome.Result),
		       *Outcome.Detail);
		PostUiEvent(FReEchoAudioEvents::UiError);
		TraitCardChoiceWidget->RestoreChoiceFailure(RunSubsystem->TimeShards);
		return;
	}

	RunSubsystem->SaveRun();
	const bool bOpenedBonusTraitChoice = RunSubsystem->Phase == EReEchoRunPhase::CardChoice;
	CloseShopCardChoice(true);
	RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
	if (bOpenedBonusTraitChoice)
	{
		bReturnToOpenShopAfterTraitChoice = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
	}
}

void AReEchoGameMode::HandleShopCardRefreshRequested(const int32 SlotIndex)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	ReEchoUIInteractionAudit::Write(TEXT("SHOP_CARD_SLOT_REFRESH_REQUEST"),
	                                FString::Printf(TEXT("tier=%d slot=%d run=%d shop=%d choice=%d"),
	                                                ActiveShopCardPackTier,
	                                                SlotIndex,
	                                                RunSubsystem ? 1 : 0,
	                                                InventoryShopWidget ? 1 : 0,
	                                                TraitCardChoiceWidget ? 1 : 0));
	if (!RunSubsystem || !InventoryShopWidget || !TraitCardChoiceWidget || ActiveShopCardPackTier <= 0)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	FString RefreshError;
	if (!RunSubsystem->TryRefreshShopCardSlot(ActiveShopCardPackTier, SlotIndex, RefreshError))
	{
		ReEchoUIInteractionAudit::Write(
		    TEXT("SHOP_CARD_SLOT_REFRESH_REJECTED"),
		    FString::Printf(TEXT("tier=%d slot=%d reason=%s"), ActiveShopCardPackTier, SlotIndex, *RefreshError));
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoShop] Card slot refresh rejected tier=%d slot=%d reason=%s"),
		       ActiveShopCardPackTier,
		       SlotIndex,
		       *RefreshError);
		PostUiEvent(FReEchoAudioEvents::UiError);
		TraitCardChoiceWidget->RestoreChoiceFailure(RunSubsystem->TimeShards);
		return;
	}

	const FReEchoWeaponPartShopView ShopView = RunSubsystem->GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* Pack = ShopView.CardPackOffers.FindByPredicate(
	    [&](const FReEchoShopCardPackOffer& Candidate)
	    {
		    return Candidate.Tier == ActiveShopCardPackTier;
	    });
	if (!Pack || Pack->Status != EReEchoShopCardPackStatus::PaidPendingChoice)
	{
		ReEchoUIInteractionAudit::Write(TEXT("SHOP_CARD_SLOT_REFRESH_REJECTED"),
		                                FString::Printf(TEXT("tier=%d slot=%d reason=PackUnavailableAfterRefresh"),
		                                                ActiveShopCardPackTier,
		                                                SlotIndex));
		PostUiEvent(FReEchoAudioEvents::UiError);
		CloseShopCardChoice(true);
		RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
		return;
	}
	TArray<FReEchoShopCardChoiceOffer> EffectiveChoices = Pack->Choices;
	RunSubsystem->SaveRun();
	TraitCardChoiceWidget->InitializeShopOffers(
	    EffectiveChoices, RunSubsystem->TimeShards, ActiveShopCardPackTier, SlotIndex);
	PostUiEvent(FReEchoAudioEvents::UiCardReveal);
	ReEchoUIInteractionAudit::Write(TEXT("SHOP_CARD_SLOT_REFRESH_SUCCEEDED"),
	                                FString::Printf(TEXT("tier=%d slot=%d candidates=%d shards=%d"),
	                                                ActiveShopCardPackTier,
	                                                SlotIndex,
	                                                EffectiveChoices.Num(),
	                                                RunSubsystem->TimeShards));
	PostUiEvent(FReEchoAudioEvents::UiPurchase);
}

void AReEchoGameMode::HandleShopCardChoiceCancelled()
{
	PostUiEvent(FReEchoAudioEvents::UiCancel);
	CloseShopCardChoice(true);
}

void AReEchoGameMode::CloseShopCardChoice(const bool bRestoreShopFocus)
{
	if (!TraitCardChoiceWidget || ActiveShopCardPackTier <= 0)
	{
		return;
	}
	if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
	{
		UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		if (bRestoreShopFocus && InventoryShopWidget)
		{
			if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
			{
				UIFlow->FocusScreen(PlayerController, EReEchoUIScreen::InventoryShop, true);
			}
		}
	}
	TraitCardChoiceWidget = nullptr;
	ActiveShopCardPackTier = 0;
}

void AReEchoGameMode::HandleShopWeaponEquipRequested(const FName WeaponId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !InventoryShopWidget)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	const FName PreviousWeaponId = RunSubsystem->CurrentBuild.WeaponId;
	FString EquipError;
	if (!RunSubsystem->TryEquipOwnedWeapon(WeaponId, EquipError))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoShop] Owned weapon '%s' could not be equipped: %s"),
		       *WeaponId.ToString(),
		       *EquipError);
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	if (PreviousWeaponId != RunSubsystem->CurrentBuild.WeaponId)
	{
		PostUiEvent(FReEchoAudioEvents::UiEquip);
	}
	RunSubsystem->SaveRun();
	RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
}

void AReEchoGameMode::HandleShopRefreshRequested()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	FString RefreshError;
	if (!RunSubsystem || !InventoryShopWidget || !RunSubsystem->TryRefreshWeaponRuneShop(RefreshError))
	{
		UE_LOG(LogReEcho, Warning, TEXT("[ReEchoShop] Weapon/rune refresh rejected: %s"), *RefreshError);
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
		case EReEchoEchoStorageResult::InvalidRecordingId:
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
	if (!RunSubsystem->CurrentBuild.CardState.OwnedCardIds.Contains(FName(ReEchoTimeAnchor::CardId)))
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		InventoryShopWidget->ShowEchoStatus(
		    NSLOCTEXT("ReEcho", "EchoStorageCardRequired", "需要先获得“时空锚点”才能存储回响。"));
		return;
	}
	const EReEchoEchoStorageResult Result = RunSubsystem->StorePendingRecordingAsTimeAnchor();
	if (Result == EReEchoEchoStorageResult::Success)
	{
		const bool bSaved = RunSubsystem->SaveRun();
		InventoryShopWidget->SetEchoSummary(RunSubsystem->GetEchoStorageSummary());
		InventoryShopWidget->ShowEchoStatus(
		    bSaved ? NSLOCTEXT("ReEcho", "EchoStored", "本场回响已设为时间锚点。")
		           : NSLOCTEXT("ReEcho", "EchoStoreSaveFailed", "本场回响已设为时间锚点，但存档失败。"));
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
	const bool bReturnToInventoryShop = bPauseOpenedOverInventoryShop && InventoryShopWidget;
	bPauseOpenedOverInventoryShop = false;
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
	if (bReturnToInventoryShop)
	{
		UGameplayStatics::SetGamePaused(this, true);
		SetPlayerMenuAbilityBlocked(true);
		InventoryShopWidget->SetKeyboardFocus();
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
			        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
			{
				UIFlow->FocusScreen(PlayerController, EReEchoUIScreen::InventoryShop, false);
			}
		}
		return;
	}
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
			const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
			RestartWidget->SetQuitConfirmation(true, false, RunSubsystem ? RunSubsystem->EncounterIndex : 0);
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
	if (bRestartScreenIsTerminal)
	{
		bExitToMainMenuAfterConfirmation = true;
		CompletePauseExit();
		return;
	}
	if (bQuitConfirmationVisible)
	{
		return;
	}
	bQuitConfirmationVisible = true;
	bExitToMainMenuAfterConfirmation = true;
	if (RestartWidget)
	{
		const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
		RestartWidget->SetQuitConfirmation(true, true, RunSubsystem ? RunSubsystem->EncounterIndex : 0);
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
	PostAudioEvent(FReEchoAudioEvents::Revive, FVector::ZeroVector);
	ShowLoadoutSelection();
}

void AReEchoGameMode::HandleEncounterEnded()
{
	if (bEncounterTransitioning || !Player)
	{
		return;
	}
	bEncounterTransitioning = true;
	ClearTimeShardPickups();
	// #9 修正：倒计时必须显示到 0 才结束本局。先把 HUD 强制刷成剩余 0 秒（让玩家看到"0"，
	// 而非冻结在上一个"1"帧），再用一个短暂 settle 节拍让"0"可见，最后收起 HUD 并弹出选卡/结算。
	UReEchoRunSubsystem* RunSubsystemForHud = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (EncounterHudWidget)
	{
		EncounterHudWidget->SetEncounterStatus(RunSubsystemForHud ? RunSubsystemForHud->EncounterIndex : 0,
		                                       GetTotalEncounterCount(),
		                                       0.0f,
		                                       Director ? Director->GetEncounterDuration()
		                                                : GetDefault<UReEchoBalanceSettings>()->EncounterDuration,
		                                       IsBossEncounter());
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
	RunSubsystem->CompleteEncounter(
	    Recording, bPlayerSurvived, bBossKilled, Player->Combatant ? Player->Combatant->CurrentHealth : -1.0f);
	if (bPlayerSurvived && Player->Combatant && RunSubsystem->CurrentBuild.CardState.Runtime.bCurseBankDefaulted)
	{
		FReEchoTimedStatusCommand Curse;
		Curse.StatusId = TEXT("Z_Cursed");
		Curse.CurrentTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		Curse.DurationSeconds = 3600.0f;
		Curse.Attack.Source = Player;
		Curse.DamageSource = EReEchoDamageSource::Player;
		Player->Combatant->ApplyTimedStatus(Curse);
	}
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
		ResetEncounterTransitionPresentation();
		// 死亡：保留 HUD 可见（显示"剩余 0 秒"+"关卡 X/Y"），不收起。
		return;
	}
	if (ShouldPlayStage01To02Cg(RunSubsystem->EncounterIndex))
	{
		if (!RunSubsystem->SkipPostEncounterCardChoiceForStageTransitionCg())
		{
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("[Stage01To02CG] Run phase could not skip Encounter 1 rewards; continuing fail-open."));
		}
		RunSubsystem->SaveRun();
		PrepareEncounterIntermission();
		bEncounterIntermissionPreparedForTransition = true;
		if (!BeginStage01To02CameraSequence())
		{
			if (!BeginStage01To02Cg())
			{
				CompleteStage01To02Cg(true);
			}
		}
		return;
	}
	const bool bShouldPlayOrdinaryTransition = !IsBossEncounter();
	if (bShouldPlayOrdinaryTransition)
	{
		PrepareEncounterIntermission();
		bEncounterIntermissionPreparedForTransition = true;
		if (!BeginEncounterEndSequence())
		{
			CompleteEncounterEndSequence();
		}
		else
		{
			UE_LOG(LogReEcho,
			       Display,
			       TEXT("[EncounterTransition] authoritative zero reached; sequence started from frame zero."));
		}
		return;
	}
	ResetEncounterTransitionPresentation();

	const float EncounterEndSettleSeconds = 0.5f;
	FTimerDelegate SettleDelegate = FTimerDelegate::CreateUObject(this, &AReEchoGameMode::ProceedToPostEncounterUI);
	GetWorldTimerManager().SetTimer(EncounterEndSettleTimerHandle, SettleDelegate, EncounterEndSettleSeconds, false);
}

bool AReEchoGameMode::ShouldStartEncounterTransition(const float RemainingTime,
                                                     const bool bBossEncounter,
                                                     const bool bTransitioning)
{
	return RemainingTime > 0.0f && RemainingTime <= 3.0f && !bBossEncounter && !bTransitioning;
}

bool AReEchoGameMode::ShouldCompleteEncounterTransition(const bool bTransitioning,
                                                        const bool bMediaFailed,
                                                        const bool bMediaFinished,
                                                        const float ElapsedSeconds)
{
	(void)ElapsedSeconds;
	return bTransitioning && (bMediaFailed || bMediaFinished);
}

bool AReEchoGameMode::ShouldPlayStage01To02Cg(const int32 CompletedEncounterIndex)
{
	return CompletedEncounterIndex == 1;
}

float AReEchoGameMode::GetStage01To02EchoRevealDelaySeconds()
{
	return 0.4f;
}

float AReEchoGameMode::GetStage01To02EchoRevealTimeoutSeconds()
{
	return 1.2f;
}

bool AReEchoGameMode::ShouldOfferStage01To02CgSkip(const bool bHasViewedCg, const bool bPlayingStageCg)
{
	return bHasViewedCg && bPlayingStageCg;
}

bool AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(const int32 CompletedEncounterIndex,
                                                           const EReEchoRunPhase Phase,
                                                           const bool bApplied,
                                                           const bool bReturningToOpenShop)
{
	return bApplied && !bReturningToOpenShop && CompletedEncounterIndex >= 2 && CompletedEncounterIndex <= 7 &&
	       Phase == EReEchoRunPhase::Planning;
}

UReEchoEncounterTransitionWidget* AReEchoGameMode::EnsureEncounterTransitionWidget()
{
	if (EncounterTransitionWidget)
	{
		return EncounterTransitionWidget;
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>() : nullptr;
	EncounterTransitionWidget = UIFlow && PlayerController
	                                ? Cast<UReEchoEncounterTransitionWidget>(UIFlow->OpenScreen(
	                                      PlayerController, EReEchoUIScreen::EncounterTransition, false, false))
	                                : nullptr;
	return EncounterTransitionWidget;
}

bool AReEchoGameMode::BeginEncounterEndSequence()
{
	UReEchoEncounterTransitionWidget* TransitionWidget = EnsureEncounterTransitionWidget();
	if (!TransitionWidget || !TransitionWidget->StartSequence())
	{
		if (ArenaCameraActor)
		{
			ArenaCameraActor->ResetEncounterCountdownPostProcess();
		}
		UE_LOG(LogReEcho, Error, TEXT("Encounter transition sequence could not start; falling back to cards."));
		return false;
	}
	if (ArenaCameraActor)
	{
		ArenaCameraActor->SetEncounterCountdownPostProcessIntensity(1.0f);
	}
	SetEncounterTransitionWorldPaused(true);
	EncounterSequenceElapsedSeconds = 0.0f;
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::PlayingSequence;
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[EncounterTransition] sequence requested at remaining=%.3f"),
	       Director ? Director->GetRemainingTime() : -1.0f);
	return true;
}

void AReEchoGameMode::CompleteEncounterEndSequence()
{
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Completed)
	{
		return;
	}
	SetEncounterTransitionWorldPaused(false);
	ProceedToPostEncounterUI();
	if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
	{
		UIFlow->CloseScreen(EReEchoUIScreen::EncounterTransition);
	}
	EncounterTransitionWidget = nullptr;
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
}

void AReEchoGameMode::UpdateEncounterTransitionPresentation(const float DeltaSeconds)
{
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::PlayingCardChoiceToShop)
	{
		const bool bFailed = !EncounterTransitionWidget || EncounterTransitionWidget->HasSequenceFailed();
		const bool bFinished = EncounterTransitionWidget && EncounterTransitionWidget->IsSequenceFinished();
		UpdateCardChoiceToShopBackgroundBlend();
		UpdateCardChoiceToShopCollapseTarget();
		if (bFailed || bFinished)
		{
			CompleteCardChoiceToShopTransition(bFailed);
		}
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::FadingToShop)
	{
		if (!EncounterTransitionWidget || EncounterTransitionWidget->IsFadeOutFinished())
		{
			FinishCardChoiceToShopFade();
		}
		return;
	}
	if (!Director)
	{
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::PlayingSequence)
	{
		EncounterSequenceElapsedSeconds += DeltaSeconds;
		const bool bFailed = !EncounterTransitionWidget || EncounterTransitionWidget->HasSequenceFailed();
		const bool bFinished = EncounterTransitionWidget && EncounterTransitionWidget->IsSequenceFinished();
		if (ShouldCompleteEncounterTransition(
		        bEncounterTransitioning, bFailed, bFinished, EncounterSequenceElapsedSeconds))
		{
			CompleteEncounterEndSequence();
		}
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusPlayer ||
	    EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02HoldPlayer ||
	    EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02RevealEcho ||
	    EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusEcho ||
	    EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02MoveToPlayer)
	{
		AdvanceStage01To02CameraSequence(DeltaSeconds);
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::PlayingStage01To02Cg)
	{
		const bool bFailed = !EncounterTransitionWidget || EncounterTransitionWidget->HasSequenceFailed();
		const bool bFinished = EncounterTransitionWidget && EncounterTransitionWidget->IsSequenceFinished();
		if (bFailed || bFinished)
		{
			CompleteStage01To02Cg(bFailed);
		}
		return;
	}
	if (bEncounterTransitioning)
	{
		return;
	}

	const float RemainingTime = Director->GetRemainingTime();
	if (ShouldStartEncounterTransition(RemainingTime, IsBossEncounter(), bEncounterTransitioning))
	{
		if (ArenaCameraActor)
		{
			ArenaCameraActor->SetEncounterCountdownPostProcessIntensity(
			    AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(RemainingTime));
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::CountdownPostProcess;
		}
	}
	else if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::CountdownPostProcess &&
	         RemainingTime > 0.0f && RemainingTime <= 3.0f)
	{
		if (ArenaCameraActor)
		{
			ArenaCameraActor->SetEncounterCountdownPostProcessIntensity(
			    AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(RemainingTime));
		}
	}
	else if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::CountdownPostProcess)
	{
		ResetEncounterTransitionPresentation();
	}
}

bool AReEchoGameMode::BeginCardChoiceToShopTransition()
{
	UReEchoEncounterTransitionWidget* TransitionWidget = EnsureEncounterTransitionWidget();
	if (!TransitionWidget || !TransitionWidget->StartCardChoiceToShopSequence())
	{
		UE_LOG(LogReEcho, Error, TEXT("[CardChoiceToShop] media could not start; failing open to shop."));
		return false;
	}
	if (TraitCardChoiceWidget)
	{
		TraitCardChoiceWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		TraitCardChoiceWidget->SetRenderOpacity(1.0f);
	}
	// TraitChoice pauses the world. Wmf/HAP can expose its first decoded surface while paused, but its media clock
	// does not advance reliably in PIE. Keep menu abilities blocked and let the transition own the visible frame.
	ResumeWorldForMenuTransition();
	bCardChoiceToShopBackgroundPrepared = false;
	PrepareCardChoiceToShopBackground();
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::PlayingCardChoiceToShop;
	UE_LOG(LogReEcho, Display, TEXT("[CardChoiceToShop] final free-card choice committed; transition started."));
	return true;
}

void AReEchoGameMode::PrepareCardChoiceToShopBackground()
{
	if (bCardChoiceToShopBackgroundPrepared)
	{
		return;
	}
	bCardChoiceToShopBackgroundPrepared = true;
	if (TraitCardChoiceWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		}
		TraitCardChoiceWidget = nullptr;
	}
	ShowPostTraitShop();
	if (InventoryShopWidget)
	{
		InventoryShopWidget->SetIsEnabled(true);
		InventoryShopWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		InventoryShopWidget->SetRenderOpacity(0.0f);
		// Opening the shop reapplies menu pause; the still-playing media must remain on a live world clock.
		ResumeWorldForMenuTransition();
	}
	UE_LOG(LogReEcho, Display, TEXT("[CardChoiceToShop] transparent shop created after TraitChoice closed."));
}

void AReEchoGameMode::UpdateCardChoiceToShopBackgroundBlend()
{
	if (!bCardChoiceToShopBackgroundPrepared || !InventoryShopWidget || !EncounterTransitionWidget)
	{
		return;
	}
	InventoryShopWidget->SetRenderOpacity(EncounterTransitionWidget->GetCardChoiceToShopBackgroundBlendAlpha());
}

void AReEchoGameMode::UpdateCardChoiceToShopCollapseTarget()
{
	if (!bCardChoiceToShopBackgroundPrepared || !InventoryShopWidget || !EncounterTransitionWidget)
	{
		return;
	}

	FVector2D TargetAbsoluteCenter;
	if (InventoryShopWidget->GetCardChoiceToShopCollapseTargetAbsolute(TargetAbsoluteCenter))
	{
		EncounterTransitionWidget->SetCardChoiceToShopCollapseTargetAbsolute(TargetAbsoluteCenter);
	}
}

void AReEchoGameMode::CompleteCardChoiceToShopTransition(const bool bFailed)
{
	if (EncounterTransitionPresentationState != EEncounterTransitionPresentationState::PlayingCardChoiceToShop)
	{
		return;
	}
	PrepareCardChoiceToShopBackground();
	if (InventoryShopWidget)
	{
		InventoryShopWidget->SetRenderOpacity(1.0f);
	}
	if (!bFailed && EncounterTransitionWidget)
	{
		EncounterTransitionWidget->BeginSequenceFadeOut(0.4f);
		EncounterTransitionPresentationState = EEncounterTransitionPresentationState::FadingToShop;
		return;
	}
	FinishCardChoiceToShopFade();
}

void AReEchoGameMode::FinishCardChoiceToShopFade()
{
	if (GetGameInstance())
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::EncounterTransition);
			if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
			{
				UIFlow->FocusScreen(PlayerController, EReEchoUIScreen::InventoryShop, true);
			}
		}
	}
	EncounterTransitionWidget = nullptr;
	bCardChoiceToShopBackgroundPrepared = false;
	if (InventoryShopWidget)
	{
		InventoryShopWidget->SetIsEnabled(true);
		InventoryShopWidget->SetRenderOpacity(1.0f);
		InventoryShopWidget->SetVisibility(ESlateVisibility::Visible);
		UGameplayStatics::SetGamePaused(this, true);
	}
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
	UE_LOG(LogReEcho, Display, TEXT("[CardChoiceToShop] shop revealed and interaction enabled."));
}

void AReEchoGameMode::ResetEncounterTransitionPresentation()
{
	SetEncounterTransitionWorldPaused(false);
	if (ArenaCameraActor)
	{
		ArenaCameraActor->ResetEncounterCountdownPostProcess();
		ArenaCameraActor->CancelStage01To02CameraSequence();
	}
	if (EncounterTransitionWidget)
	{
		EncounterTransitionWidget->ResetPresentation();
	}
	if (EncounterTransitionMediaSound)
	{
		EncounterTransitionMediaSound->SetMediaPlayer(nullptr);
	}
	if (GetGameInstance())
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::EncounterTransition);
		}
	}
	EncounterTransitionWidget = nullptr;
	bCardChoiceToShopBackgroundPrepared = false;
	EncounterSequenceElapsedSeconds = 0.0f;
	Stage01To02EchoRevealElapsedSeconds = 0.0f;
	CompleteStage01To02EchoReveal();
	bEncounterIntermissionPreparedForTransition = false;
	bPreparedEncounterAwaitingActivation = false;
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::None;
}

void AReEchoGameMode::SetEncounterTransitionWorldPaused(const bool bPaused)
{
	if (bPaused)
	{
		SetEncounterTransitionCameraRefreshWhilePaused(true);
		if (!UGameplayStatics::IsGamePaused(this))
		{
			bEncounterTransitionPausedWorld = UGameplayStatics::SetGamePaused(this, true);
			UE_LOG(LogReEcho,
			       Display,
			       TEXT("[EncounterTransition] final frame frozen paused=%s"),
			       bEncounterTransitionPausedWorld ? TEXT("true") : TEXT("false"));
		}
		return;
	}
	if (bEncounterTransitionPausedWorld)
	{
		const bool bResumed = UGameplayStatics::SetGamePaused(this, false);
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[EncounterTransition] final frame released resumed=%s"),
		       bResumed ? TEXT("true") : TEXT("false"));
		bEncounterTransitionPausedWorld = false;
	}
	SetEncounterTransitionCameraRefreshWhilePaused(false);
}

void AReEchoGameMode::SetEncounterTransitionCameraRefreshWhilePaused(const bool bEnabled)
{
	AReEchoPlayerController* PlayerController =
	    Cast<AReEchoPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (bEnabled)
	{
		if (!PlayerController || bEncounterTransitionControllerPauseTickOverridden)
		{
			return;
		}
		bEncounterTransitionPreviousControllerFullTickWhenPaused = PlayerController->ShouldRefreshCameraWhilePaused();
		bEncounterTransitionControllerPauseTickOverridden = true;
		PlayerController->SetRefreshCameraWhilePaused(true);
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[EncounterTransition] paused camera refresh enabled controller=%s previous=%s"),
		       *GetNameSafe(PlayerController),
		       bEncounterTransitionPreviousControllerFullTickWhenPaused ? TEXT("true") : TEXT("false"));
		return;
	}
	if (!bEncounterTransitionControllerPauseTickOverridden)
	{
		return;
	}
	if (PlayerController)
	{
		PlayerController->SetRefreshCameraWhilePaused(bEncounterTransitionPreviousControllerFullTickWhenPaused);
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[EncounterTransition] paused camera refresh restored controller=%s value=%s"),
		       *GetNameSafe(PlayerController),
		       PlayerController->ShouldRefreshCameraWhilePaused() ? TEXT("true") : TEXT("false"));
	}
	bEncounterTransitionControllerPauseTickOverridden = false;
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
		PostAudioEvent(FReEchoAudioEvents::FlowVictory, FVector::ZeroVector);
		if (!bEncounterIntermissionPreparedForTransition)
		{
			PrepareEncounterIntermission();
		}
		bEncounterIntermissionPreparedForTransition = false;
		if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
		{
			ShowTraitCardChoice();
		}
		else
		{
			bContinueRunAfterShop = true;
			ShowPostTraitShop();
		}
	}
}

bool AReEchoGameMode::BeginStage01To02CameraSequence()
{
	if (!ArenaCameraActor || !Player)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[Stage01To02Camera] pre-CG focus skipped camera=%s player=%s."),
		       *GetNameSafe(ArenaCameraActor),
		       *GetNameSafe(Player));
		return false;
	}
	SetActorTickEnabled(true);
	SetTickableWhenPaused(true);
	ArenaCameraActor->ResetEncounterCountdownPostProcess();
	ArenaCameraActor->BeginStage01To02CameraSequence();
	if (!ArenaCameraActor->FocusStage01To02Target(Player,
	                                              ArenaCameraActor->GetStage01To02PlayerFocusRatio(),
	                                              ArenaCameraActor->GetStage01To02PlayerFocusDuration()))
	{
		ArenaCameraActor->CancelStage01To02CameraSequence();
		return false;
	}
	SetEncounterTransitionWorldPaused(true);
	SetPlayerMenuAbilityBlocked(true);
	if (UReEchoAudioService* AudioService = GetGameInstance()->GetSubsystem<UReEchoAudioService>())
	{
		AudioService->StopMusicState();
	}
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02FocusPlayer;
	UE_LOG(LogReEcho, Display, TEXT("[Stage01To02Camera] pre-CG player focus started."));
	return true;
}

bool AReEchoGameMode::BeginStage01To02Cg()
{
	SetEncounterTransitionWorldPaused(false);
	if (ArenaCameraActor)
	{
		ArenaCameraActor->ResetEncounterCountdownPostProcess();
	}
	UReEchoEncounterTransitionWidget* TransitionWidget = EnsureEncounterTransitionWidget();
	if (!TransitionWidget)
	{
		UE_LOG(LogReEcho, Error, TEXT("[Stage01To02CG] transition screen could not be opened; fail-open."));
		return false;
	}
	TransitionWidget->ResetPresentation();
	if (!TransitionWidget->StartStage01To02Sequence())
	{
		UE_LOG(LogReEcho, Error, TEXT("[Stage01To02CG] media source could not start; fail-open."));
		return false;
	}
	TransitionWidget->OnStageCgSkipRequested.RemoveAll(this);
	TransitionWidget->OnStageCgSkipRequested.AddDynamic(this, &AReEchoGameMode::HandleStage01To02CgSkipRequested);
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	TransitionWidget->SetStage01To02SkipAvailable(
	    ShouldOfferStage01To02CgSkip(RunSubsystem && RunSubsystem->HasViewedStage01To02Cg(), true));
	SetPlayerMenuAbilityBlocked(true);
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::PlayingStage01To02Cg;
	if (UReEchoAudioService* AudioService = GetGameInstance()->GetSubsystem<UReEchoAudioService>())
	{
		AudioService->StopMusicState();
	}
	UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] playback requested directly after Encounter 1."));
	return true;
}

void AReEchoGameMode::HandleStage01To02CgSkipRequested()
{
	if (EncounterTransitionPresentationState != EEncounterTransitionPresentationState::PlayingStage01To02Cg)
	{
		return;
	}
	UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] previously viewed CG skipped by player."));
	if (EncounterTransitionWidget)
	{
		EncounterTransitionWidget->ResetPresentation();
	}
	CompleteStage01To02Cg(false, true);
}

void AReEchoGameMode::CompleteStage01To02Cg(const bool bFailed, const bool bSkipped)
{
	if (EncounterTransitionPresentationState != EEncounterTransitionPresentationState::PlayingStage01To02Cg)
	{
		return;
	}
	if (bFailed)
	{
		UE_LOG(LogReEcho, Error, TEXT("[Stage01To02CG] playback failed; continuing to the post-CG camera gate."));
	}
	else if (bSkipped)
	{
		UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] skip accepted; preparing Encounter 2 presentation."));
	}
	else
	{
		UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] playback completed; preparing Encounter 2 presentation."));
		if (UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
		{
			RunSubsystem->MarkStage01To02CgViewed();
		}
	}
	if (EncounterTransitionMediaSound)
	{
		EncounterTransitionMediaSound->SetMediaPlayer(nullptr);
	}
	if (!PrepareNextEncounter(true))
	{
		UE_LOG(LogReEcho, Error, TEXT("[Stage01To02Camera] Encounter 2 preparation failed; retrying normal entry."));
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::EncounterTransition);
		}
		EncounterTransitionWidget = nullptr;
		if (ArenaCameraActor)
		{
			ArenaCameraActor->CancelStage01To02CameraSequence();
		}
		EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
		return;
	}
	AReEchoEchoActor* EchoTarget = FindStage01To02CameraEcho();
	const bool bEchoPrepositioned =
	    ArenaCameraActor && EchoTarget &&
	    ArenaCameraActor->FocusStage01To02Target(EchoTarget, ArenaCameraActor->GetStage01To02EchoFocusRatio(), 0.0f);
	if (bEchoPrepositioned)
	{
		UE_LOG(LogReEcho,
		       Display,
		       TEXT("[Stage01To02Camera] Echo prepositioned behind the final CG frame target=%s."),
		       *GetNameSafe(EchoTarget));
	}
	// Encounter 2 remains prepared but inactive, with input and enemy simulation still gated. Leave the world
	// unpaused so Niagara's system manager can actually simulate the birth effect before the Echo is revealed.
	SetEncounterTransitionWorldPaused(false);
	// Start the birth circle while the final opaque CG frame still covers the world. Closing the transition screen
	// afterwards guarantees the first returned gameplay frame already contains the running effect.
	const bool bEchoRevealStarted = bEchoPrepositioned && BeginStage01To02EchoReveal();
	if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
	        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
	{
		UIFlow->CloseScreen(EReEchoUIScreen::EncounterTransition);
	}
	EncounterTransitionWidget = nullptr;
	if (bEchoPrepositioned)
	{
		if (bEchoRevealStarted)
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02RevealEcho;
			return;
		}
		CompleteStage01To02EchoReveal();
		if (ArenaCameraActor->FocusStage01To02TargetAtStandardWidth(
		        EchoTarget, ArenaCameraActor->GetStage01To02EchoZoomOutDuration()))
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02FocusEcho;
			return;
		}
	}
	BeginStage01To02PostCgCameraSequence();
}

void AReEchoGameMode::BeginStage01To02PostCgCameraSequence()
{
	CompleteStage01To02EchoReveal();
	AReEchoEchoActor* EchoTarget = FindStage01To02CameraEcho();
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[Stage01To02Camera] hidden Echo preposition skipped camera=%s echo=%s; restoring via player."),
	       *GetNameSafe(ArenaCameraActor),
	       *GetNameSafe(EchoTarget));
	if (ArenaCameraActor && Player &&
	    ArenaCameraActor->FocusStage01To02TargetAtStandardWidth(Player,
	                                                            ArenaCameraActor->GetStage01To02MoveToPlayerDuration()))
	{
		EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02MoveToPlayer;
		return;
	}
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
	if (ArenaCameraActor)
	{
		ArenaCameraActor->EndStage01To02CameraSequence();
	}
	ActivatePreparedEncounter();
}

void AReEchoGameMode::AdvanceStage01To02CameraSequence(const float DeltaSeconds)
{
	if (!ArenaCameraActor)
	{
		if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusPlayer)
		{
			if (!BeginStage01To02Cg())
			{
				EncounterTransitionPresentationState = EEncounterTransitionPresentationState::PlayingStage01To02Cg;
				CompleteStage01To02Cg(true);
			}
		}
		else
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
			ActivatePreparedEncounter();
		}
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02RevealEcho)
	{
		Stage01To02EchoRevealElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
		bool bAnyBornEffectActive = false;
		for (const AReEchoEchoActor* Echo : Echoes)
		{
			bAnyBornEffectActive |= IsValid(Echo) && Echo->IsBornVfxPlaying();
		}
		const bool bRevealDelayElapsed = Stage01To02EchoRevealElapsedSeconds >= GetStage01To02EchoRevealDelaySeconds();
		if (!bAnyBornEffectActive || bRevealDelayElapsed)
		{
			CompleteStage01To02EchoReveal();
			AReEchoEchoActor* EchoTarget = FindStage01To02CameraEcho();
			if (EchoTarget && ArenaCameraActor->FocusStage01To02TargetAtStandardWidth(
			                      EchoTarget, ArenaCameraActor->GetStage01To02EchoZoomOutDuration()))
			{
				EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02FocusEcho;
			}
			else
			{
				BeginStage01To02PostCgCameraSequence();
			}
			return;
		}
		return;
	}
	if (!ArenaCameraActor->IsStage01To02CameraMoveComplete())
	{
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusPlayer)
	{
		if (Player && ArenaCameraActor->FocusStage01To02Target(Player,
		                                                       ArenaCameraActor->GetStage01To02PlayerFocusRatio(),
		                                                       ArenaCameraActor->GetStage01To02PlayerHoldDuration()))
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02HoldPlayer;
			return;
		}
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02HoldPlayer ||
	    EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusPlayer)
	{
		if (!BeginStage01To02Cg())
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::PlayingStage01To02Cg;
			CompleteStage01To02Cg(true);
		}
		return;
	}
	if (EncounterTransitionPresentationState == EEncounterTransitionPresentationState::Stage01To02FocusEcho)
	{
		if (Player && ArenaCameraActor->FocusStage01To02TargetAtStandardWidth(
		                  Player, ArenaCameraActor->GetStage01To02MoveToPlayerDuration()))
		{
			EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Stage01To02MoveToPlayer;
			return;
		}
	}
	ArenaCameraActor->EndStage01To02CameraSequence();
	EncounterTransitionPresentationState = EEncounterTransitionPresentationState::Completed;
	ActivatePreparedEncounter();
}

bool AReEchoGameMode::BeginStage01To02EchoReveal()
{
	Stage01To02EchoRevealElapsedSeconds = 0.0f;
	bool bPlayedAnyBornVfx = false;
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (IsValid(Echo))
		{
			bPlayedAnyBornVfx |= Echo->BeginDeferredBornReveal();
		}
	}
	return bPlayedAnyBornVfx;
}

void AReEchoGameMode::CompleteStage01To02EchoReveal()
{
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (IsValid(Echo))
		{
			Echo->CompleteDeferredBornReveal();
		}
	}
}

AReEchoEchoActor* AReEchoGameMode::FindStage01To02CameraEcho() const
{
	for (AReEchoEchoActor* Echo : Echoes)
	{
		if (IsValid(Echo))
		{
			return Echo;
		}
	}
	return nullptr;
}

void AReEchoGameMode::CloseTraitCardChoiceScreen()
{
	if (TraitCardChoiceWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		}
		TraitCardChoiceWidget = nullptr;
	}
	if (UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->ResetPendingTraitCardChoice();
	}
	SetPlayerMenuAbilityBlocked(false);
}

void AReEchoGameMode::ShowTraitCardChoice()
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!RunSubsystem || !PlayerController || TraitCardChoiceWidget)
	{
		return;
	}

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
	if (Offers.Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Expected three trait card offers, received %d"), Offers.Num());
		if (RunSubsystem->Phase == EReEchoRunPhase::Planning)
		{
			bContinueRunAfterShop = true;
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowPostTraitShop);
		}
		else
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
		}
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

	// Cadence abilities can let this pack hand out two cards; set it before the offers are revealed.
	TraitCardChoiceWidget->SetSelectableCount(RunSubsystem->GetPendingTraitCardSelectableCount());
	TraitCardChoiceWidget->InitializeOffers(Offers, RunSubsystem->TimeShards);
	PostUiEvent(FReEchoAudioEvents::UiCardReveal);
	TraitCardChoiceWidget->OnCardSelected.AddDynamic(this, &AReEchoGameMode::HandleTraitCardSelected);
	TraitCardChoiceWidget->OnCardChoicesSelected.AddDynamic(this, &AReEchoGameMode::HandleTraitCardsSelected);
	TraitCardChoiceWidget->OnCardSlotRefreshRequested.AddDynamic(this,
	                                                             &AReEchoGameMode::HandleTraitCardRefreshRequested);
	SetPlayerMenuAbilityBlocked(true);
}

void AReEchoGameMode::HandleTraitCardRefreshRequested(const int32 SlotIndex)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	ReEchoUIInteractionAudit::Write(TEXT("FREE_CARD_SLOT_REFRESH_REQUEST"),
	                                FString::Printf(TEXT("encounter=%d slot=%d run=%d choice=%d"),
	                                                RunSubsystem ? RunSubsystem->EncounterIndex : INDEX_NONE,
	                                                SlotIndex,
	                                                RunSubsystem ? 1 : 0,
	                                                TraitCardChoiceWidget ? 1 : 0));
	if (!RunSubsystem || !TraitCardChoiceWidget)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	FString RefreshError;
	if (!RunSubsystem->TryRefreshTraitCardSlot(SlotIndex, RefreshError))
	{
		// 兜底诊断：孤儿屏（Phase 已离开 CardChoice）下刷新被拒时，收敛孤儿屏并续上流程，避免软锁。
		if (TraitCardChoiceWidget && RunSubsystem->Phase != EReEchoRunPhase::CardChoice)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[TraitChoice] Refresh rejected on orphaned trait choice screen (phase=%d); closing and resuming flow."),
			       static_cast<int32>(RunSubsystem->Phase));
			CloseTraitCardChoiceScreen();
			bContinueRunAfterShop = true;
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowPostTraitShop);
			return;
		}
		ReEchoUIInteractionAudit::Write(
		    TEXT("FREE_CARD_SLOT_REFRESH_REJECTED"),
		    FString::Printf(
		        TEXT("encounter=%d slot=%d reason=%s"), RunSubsystem->EncounterIndex, SlotIndex, *RefreshError));
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ReEchoFreeCard] Slot refresh rejected encounter=%d slot=%d reason=%s"),
		       RunSubsystem->EncounterIndex,
		       SlotIndex,
		       *RefreshError);
		TraitCardChoiceWidget->RestoreChoiceFailure(RunSubsystem->TimeShards);
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}

	const TArray<FReEchoTraitCardOffer> Offers = RunSubsystem->GenerateTraitCardOffers(3);
	if (Offers.Num() != 3)
	{
		ReEchoUIInteractionAudit::Write(TEXT("FREE_CARD_SLOT_REFRESH_REJECTED"),
		                                FString::Printf(TEXT("encounter=%d slot=%d reason=ProjectionFailed"),
		                                                RunSubsystem->EncounterIndex,
		                                                SlotIndex));
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	RunSubsystem->SaveRun();
	TraitCardChoiceWidget->InitializeOffers(Offers, RunSubsystem->TimeShards, SlotIndex);
	PostUiEvent(FReEchoAudioEvents::UiCardReveal);
	RefreshPlayerHudTimeShards(RunSubsystem);
	ReEchoUIInteractionAudit::Write(TEXT("FREE_CARD_SLOT_REFRESH_SUCCEEDED"),
	                                FString::Printf(TEXT("encounter=%d slot=%d candidates=%d shards=%d"),
	                                                RunSubsystem->EncounterIndex,
	                                                SlotIndex,
	                                                Offers.Num(),
	                                                RunSubsystem->TimeShards));
	PostUiEvent(FReEchoAudioEvents::UiPurchase);
}

void AReEchoGameMode::HandleTraitCardsSelected(const TArray<FName>& CardIds)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || CardIds.Num() <= 0)
	{
		return;
	}
	// Cadence ability: every picked card is granted in one transaction, so the pack closes at once and the
	// phase returns to planning. No deferred follow-up choice is ever opened.
	const bool bApplied = RunSubsystem->ApplyTraitCards(CardIds);
	if (!bApplied)
	{
		PostUiEvent(FReEchoAudioEvents::UiError);
		if (TraitCardChoiceWidget)
		{
			TraitCardChoiceWidget->RestoreChoiceFailure(RunSubsystem->TimeShards);
		}
		return;
	}
	RunSubsystem->SaveRun();

	const bool bPlayCardChoiceToShop = ShouldPlayCardChoiceToShopTransition(
	    RunSubsystem->EncounterIndex, RunSubsystem->Phase, bApplied, bReturnToOpenShopAfterTraitChoice);
	if (bPlayCardChoiceToShop)
	{
		bContinueRunAfterShop = true;
		if (BeginCardChoiceToShopTransition())
		{
			return;
		}
	}

	if (TraitCardChoiceWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		}
		TraitCardChoiceWidget = nullptr;
	}
	if (bReturnToOpenShopAfterTraitChoice)
	{
		if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
			return;
		}
		bReturnToOpenShopAfterTraitChoice = false;
		if (ArenaCameraActor)
		{
			ArenaCameraActor->ResetEncounterCountdownPostProcess();
		}
		if (InventoryShopWidget)
		{
			RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
			if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
			        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
			{
				if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
				{
					UIFlow->FocusScreen(PlayerController, EReEchoUIScreen::InventoryShop, true);
				}
			}
			SetPlayerMenuAbilityBlocked(true);
			return;
		}
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

void AReEchoGameMode::HandleTraitCardSelected(const FName CardId)
{
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem)
	{
		return;
	}
	const bool bApplied = RunSubsystem->ApplyTraitCard(CardId);
	if (!bApplied)
	{
		// 兜底诊断：特质卡屏已沦为孤儿（Phase 已离开 CardChoice）时，确认无法应用。
		// 此时不应静默 UiError 导致软锁，而应收敛孤儿屏并续上流程（回到下一遭遇前的商店/结算）。
		if (TraitCardChoiceWidget && RunSubsystem->Phase != EReEchoRunPhase::CardChoice)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[TraitChoice] Confirm ignored on orphaned trait choice screen (phase=%d); closing and resuming flow."),
			       static_cast<int32>(RunSubsystem->Phase));
			CloseTraitCardChoiceScreen();
			bContinueRunAfterShop = true;
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowPostTraitShop);
			return;
		}
		PostUiEvent(FReEchoAudioEvents::UiError);
		return;
	}
	RunSubsystem->SaveRun();

	const bool bPlayCardChoiceToShop = ShouldPlayCardChoiceToShopTransition(
	    RunSubsystem->EncounterIndex, RunSubsystem->Phase, bApplied, bReturnToOpenShopAfterTraitChoice);
	if (bPlayCardChoiceToShop)
	{
		bContinueRunAfterShop = true;
		if (BeginCardChoiceToShopTransition())
		{
			return;
		}
	}

	if (TraitCardChoiceWidget)
	{
		if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
		        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
		{
			UIFlow->CloseScreen(EReEchoUIScreen::TraitChoice);
		}
		TraitCardChoiceWidget = nullptr;
	}
	if (bReturnToOpenShopAfterTraitChoice)
	{
		if (RunSubsystem->Phase == EReEchoRunPhase::CardChoice)
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::ShowTraitCardChoice);
			return;
		}
		bReturnToOpenShopAfterTraitChoice = false;
		if (ArenaCameraActor)
		{
			ArenaCameraActor->ResetEncounterCountdownPostProcess();
		}
		if (InventoryShopWidget)
		{
			RefreshShopPresentation(RunSubsystem, InventoryShopWidget->GetMode());
			if (UReEchoUIFlowCoordinatorSubsystem* UIFlow =
			        GetGameInstance()->GetSubsystem<UReEchoUIFlowCoordinatorSubsystem>())
			{
				if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
				{
					UIFlow->FocusScreen(PlayerController, EReEchoUIScreen::InventoryShop, true);
				}
			}
			SetPlayerMenuAbilityBlocked(true);
			return;
		}
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

void AReEchoGameMode::HandleCardGrantCommitted(const FReEchoStatBlock& Stats,
                                               const EReEchoHealthAdjustment HealthAdjustment)
{
	if (HealthAdjustment != EReEchoHealthAdjustment::None && Player && Player->Combatant)
	{
		Player->Combatant->ApplyHealthAdjustment(Stats.HpMax, HealthAdjustment);
		if (HealthAdjustment == EReEchoHealthAdjustment::SetToStatPoint)
		{
			Player->Combatant->RestoreCurrentHealth(Stats.HpPoint);
		}
	}
}

void AReEchoGameMode::ShowPostTraitShop()
{
	if (ArenaCameraActor)
	{
		ArenaCameraActor->ResetEncounterCountdownPostProcess();
	}
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
	OutView.PlayerIcon = Player->GetMinimapIconTexture();

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
		Entry.Icon = Echo->GetMinimapIconTexture();
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

void AReEchoGameMode::RefreshPlayerHudTimeShards(const UReEchoRunSubsystem* RunSubsystem)
{
	if (PlayerHudWidget)
	{
		PlayerHudWidget->SetTimeShards(RunSubsystem ? RunSubsystem->GetDisplayedTimeShardBalance() : 0);
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
		int32 BossEntryCount = 0;
		int32 LivingBossCount = 0;
		for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
		{
			if (Entry.Archetype == EReEchoEnemyArchetype::Boss)
			{
				++BossEntryCount;
				LivingBossCount += Entry.bAlive ? 1 : 0;
			}
		}
		if (ShouldCompleteBossEncounter(bBossSuccessfullySpawnedThisEncounter, LivingBossCount))
		{
			int32 PendingBossBatchCount = 0;
			int32 PendingBossLocationCount = 0;
			for (const FReEchoPendingSpawnBatchState& Pending : PendingSpawnBatches)
			{
				if (Pending.EnemyRole == TEXT("Boss"))
				{
					++PendingBossBatchCount;
					PendingBossLocationCount += Pending.Locations.Num();
				}
			}
			const UReEchoRunSubsystem* TraceRunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("[BossVictoryTrace][VictoryCandidate] encounter=%d encounterTime=%.3f roster=%d bossSpawned=%s "
			            "bossEntries=%d livingBosses=%d pendingBossBatches=%d pendingBossLocations=%d "
			            "scheduler=%d/%d clearedBefore=%s"),
			       TraceRunSubsystem ? TraceRunSubsystem->EncounterIndex : -1,
			       Director ? Director->EncounterTime : -1.0f,
			       EnemyRoster ? EnemyRoster->GetEntries().Num() : -1,
			       bBossSuccessfullySpawnedThisEncounter ? TEXT("true") : TEXT("false"),
			       BossEntryCount,
			       LivingBossCount,
			       PendingBossBatchCount,
			       PendingBossLocationCount,
			       EncounterWaveScheduler.GetNextEventIndex(),
			       EncounterWaveScheduler.GetEventCount(),
			       bEncounterClearedByDefeat ? TEXT("true") : TEXT("false"));
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
	RefreshPlayerHudTimeShards(RunSubsystem);
	if (EncounterHudWidget)
	{
		const bool bCurrentEncounterIsBoss = IsBossEncounter();
		float BossCurrentHealth = 0.0f;
		float BossMaximumHealth = 0.0f;
		if (bCurrentEncounterIsBoss)
		{
			for (const FReEchoEnemyRosterEntrySnapshot& Entry : EnemyRoster->GetEntries())
			{
				if (Entry.Archetype != EReEchoEnemyArchetype::Boss || !Entry.bAlive)
				{
					continue;
				}
				if (const AReEchoEnemyActor* Boss = Cast<AReEchoEnemyActor>(Entry.Host.Get()))
				{
					if (const UReEchoCombatantComponent* BossCombatant = Boss->GetCombatantComponent())
					{
						BossCurrentHealth = BossCombatant->CurrentHealth;
						BossMaximumHealth = BossCombatant->Stats.HpMax;
					}
				}
				break;
			}
		}
		EncounterHudWidget->SetEncounterStatus(RunSubsystem ? RunSubsystem->EncounterIndex : 0,
		                                       GetTotalEncounterCount(),
		                                       Director->GetRemainingTime(),
		                                       Director->GetEncounterDuration(),
		                                       bCurrentEncounterIsBoss,
		                                       BossCurrentHealth,
		                                       BossMaximumHealth);
	}
	UpdateEncounterTransitionPresentation(DeltaSeconds);
}
