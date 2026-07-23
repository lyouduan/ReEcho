#include "ReEchoGameMode.h"

#include "Combat/ReEchoCombatantComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Encounter/ReEchoEncounterDirector.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoRestartWidget.h"
#include "UI/ReEchoTraitCardChoiceWidget.h"

AReEchoGameMode::AReEchoGameMode()
{
	DefaultPawnClass = AReEchoPlayerPawn::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void AReEchoGameMode::StartPlay()
{
	Super::StartPlay();
	CreateArena();
	Player = Cast<AReEchoPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	Director = GetWorld()->SpawnActor<AReEchoEncounterDirector>();
	Director->OnFixedStep.AddDynamic(this, &AReEchoGameMode::HandleFixedStep);
	Director->OnEncounterEnded.AddDynamic(this, &AReEchoGameMode::HandleEncounterEnded);
	if (Player)
	{
		Player->OnActiveSkill.AddDynamic(this, &AReEchoGameMode::HandlePlayerSkill);
		Player->OnWeaponChanged.AddDynamic(this, &AReEchoGameMode::HandlePlayerWeaponChanged);
		Player->Combatant->OnDeath.AddDynamic(this, &AReEchoGameMode::HandlePlayerDeath);
	}
	if (UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>())
	{
		RunSubsystem->StartRun(TEXT("J01"), TEXT("W_J_02"));
	}
	BeginNextEncounter();
}

static void ApplyShapeMaterial(UStaticMeshComponent* Mesh, const FLinearColor& Color)
{
	if (UMaterialInterface* Base =
	        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Base, Mesh);
		MaterialInstance->SetVectorParameterValue(TEXT("Color"), Color);
		Mesh->SetMaterial(0, MaterialInstance);
	}
}

void AReEchoGameMode::CreateArena()
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/CubeMesh.CubeMesh"));
	auto SpawnBlock = [&](FVector Location, FVector Scale, FLinearColor Color, const TCHAR* Name)
	{
		AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
		StaticMeshActor->SetActorLabel(Name);
		StaticMeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		StaticMeshActor->SetActorScale3D(Scale);
		ApplyShapeMaterial(StaticMeshActor->GetStaticMeshComponent(), Color);
	};
	SpawnBlock(FVector(0, 0, -55), FVector(14, 14, 0.5f), FLinearColor(0.04f, 0.05f, 0.07f), TEXT("ArenaFloor"));
	SpawnBlock(FVector(0, 1400, 100), FVector(14, 0.25f, 2.f), FLinearColor(0.1f, 0.12f, 0.18f), TEXT("WallNorth"));
	SpawnBlock(FVector(0, -1400, 100), FVector(14, 0.25f, 2.f), FLinearColor(0.1f, 0.12f, 0.18f), TEXT("WallSouth"));
	SpawnBlock(FVector(1400, 0, 100), FVector(0.25f, 14, 2.f), FLinearColor(0.1f, 0.12f, 0.18f), TEXT("WallEast"));
	SpawnBlock(FVector(-1400, 0, 100), FVector(0.25f, 14, 2.f), FLinearColor(0.1f, 0.12f, 0.18f), TEXT("WallWest"));
	ADirectionalLight* Light =
	    GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55.f, -35.f, 0.f));
	Light->GetLightComponent()->SetIntensity(6.f);
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
}

void AReEchoGameMode::BeginNextEncounter()
{
	ClearCombatants();
	UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || RunSubsystem->EncounterIndex >= 6)
	{
		return;
	}
	bEncounterTransitioning = false;
	RunSubsystem->BeginEncounter();
	if (Player)
	{
		Player->SetActorLocation(FVector(0, 0, 40));
		const FReEchoStatBlock& Stats = RunSubsystem->CurrentBuild.Stats;
		Player->Combatant->InitializeFromStats(Stats, true);
		Player->Movement->MaxSpeed = 420.0f * Stats.MovementSpeed;
		Player->Recorder->BeginRecording(RunSubsystem->EncounterIndex,
		                                 TEXT("GrayboxArena"),
		                                 1337 + RunSubsystem->EncounterIndex,
		                                 RunSubsystem->CurrentBuild);
	}
	const TArray<FReEchoRecording> Recordings = RunSubsystem->GetEchoRecordings(1);
	if (!Recordings.IsEmpty())
	{
		AReEchoEchoActor* Echo = GetWorld()->SpawnActor<AReEchoEchoActor>();
		Echo->InitializeEcho(
			Recordings[0],
			RunSubsystem->CurrentBuild.Stats.EchoEfficiency);
		Echoes.Add(Echo);
	}
	SpawnEnemies(RunSubsystem->EncounterIndex);
	Director->StartEncounter();
}

void AReEchoGameMode::SpawnEnemies(int32 EncounterIndex)
{
	const int32 GruntCount = FMath::Min(3 + EncounterIndex, 8);
	const int32 BomberCount = EncounterIndex >= 2 ? FMath::Min(EncounterIndex, 4) : 0;
	int32 SpawnIndex = 0;
	auto SpawnEnemy = [&](EReEchoEnemyKind Kind, float Radius)
	{
		const float Angle = SpawnIndex++ * 2.399963f;
		FVector SpawnLocation(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 50.f);
		AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>(SpawnLocation, FRotator::ZeroRotator);
		Enemy->Configure(Kind, SpawnIndex);
	};
	if (EncounterIndex == 6)
	{
		SpawnEnemy(EReEchoEnemyKind::Boss, 900.f);
		return;
	}
	for (int32 EnemyIndex = 0; EnemyIndex < GruntCount; ++EnemyIndex)
	{
		SpawnEnemy(EReEchoEnemyKind::Grunt, 700.f + EnemyIndex * 45.f);
	}

	for (int32 EnemyIndex = 0; EnemyIndex < BomberCount; ++EnemyIndex)
	{
		SpawnEnemy(EReEchoEnemyKind::Bomber, 1050.f);
	}
}

void AReEchoGameMode::HandleFixedStep(float FixedDeltaSeconds)
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

void AReEchoGameMode::HandlePlayerWeaponChanged(const FName WeaponId)
{
	if (Player && Director)
	{
		Player->Recorder->RecordWeaponChange(
			Director->EncounterTime,
			WeaponId);
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

void AReEchoGameMode::ShowRestartScreen()
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

	RestartWidget = CreateWidget<UReEchoRestartWidget>(
		PlayerController,
		UReEchoRestartWidget::StaticClass());
	if (!RestartWidget)
	{
		return;
	}

	RestartWidget->OnRestartRequested.AddDynamic(this, &AReEchoGameMode::HandleRestartRequested);
	RestartWidget->AddToViewport(100);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(RestartWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleRestartRequested()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (RestartWidget)
	{
		RestartWidget->RemoveFromParent();
		RestartWidget = nullptr;
	}

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
	const FReEchoRecording Recording = Player->Recorder->FinishRecording(Director ? Director->EncounterTime : 30.f);
	const bool bPlayerSurvived = Player->Combatant->IsAlive();
	RunSubsystem->CompleteEncounter(Recording, bPlayerSurvived, false);
	if (!bPlayerSurvived)
	{
		return;
	}

	if (RunSubsystem->EncounterIndex < 6)
	{
		ShowTraitCardChoice();
	}
}

void AReEchoGameMode::ShowTraitCardChoice()
{
	ClearCombatants();

	UReEchoRunSubsystem* RunSubsystem =
		GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!RunSubsystem || !PlayerController || TraitCardChoiceWidget)
	{
		return;
	}

	const TArray<FReEchoTraitCardOffer> Offers =
		RunSubsystem->GenerateTraitCardOffers(3);
	if (Offers.Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Expected three trait card offers, received %d"), Offers.Num());
		GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
		return;
	}

	TraitCardChoiceWidget = CreateWidget<UReEchoTraitCardChoiceWidget>(
		PlayerController,
		UReEchoTraitCardChoiceWidget::StaticClass());
	if (!TraitCardChoiceWidget)
	{
		return;
	}

	TraitCardChoiceWidget->InitializeOffers(Offers);
	TraitCardChoiceWidget->OnCardSelected.AddDynamic(
		this,
		&AReEchoGameMode::HandleTraitCardSelected);
	TraitCardChoiceWidget->AddToViewport(90);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TraitCardChoiceWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AReEchoGameMode::HandleTraitCardSelected(const FName CardId)
{
	UReEchoRunSubsystem* RunSubsystem =
		GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	if (!RunSubsystem || !RunSubsystem->ApplyTraitCard(CardId))
	{
		return;
	}

	if (TraitCardChoiceWidget)
	{
		TraitCardChoiceWidget->RemoveFromParent();
		TraitCardChoiceWidget = nullptr;
	}

	RestoreGameInput();
	GetWorldTimerManager().SetTimerForNextTick(this, &AReEchoGameMode::BeginNextEncounter);
}

void AReEchoGameMode::RestoreGameInput()
{
	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PlayerController =
			UGameplayStatics::GetPlayerController(this, 0))
	{
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
	if (!GEngine || !Director || !Player)
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
			Director->EndEncounter();
		}
	}
	const UReEchoRunSubsystem* RunSubsystem = GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>();
	const FString Text = FString::Printf(
	    TEXT("RE-ECHO  |  Encounter %d/6  |  Time %0.1f  |  HP %0.0f  |  Weapon %s  |  1/2/3 Switch  LMB/J Attack"),
	    RunSubsystem ? RunSubsystem->EncounterIndex : 0,
	    Director->GetRemainingTime(),
	    Player->Combatant->CurrentHealth,
	    *Player->GetEquippedWeaponLabel());
	GEngine->AddOnScreenDebugMessage(7, 0.f, FColor::White, Text);
}


