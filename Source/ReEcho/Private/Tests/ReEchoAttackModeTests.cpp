// ReEcho Plan28: deterministic automation tests for player automatic/manual basic attack.
// Covers only cheap, deterministic logic (no PIE, no world): default mode, mode transitions,
// held-input release on mode switch, deterministic nearest-target selection, stable tie-break,
// no-target result, and that auto-attack / mode switching produce no Echo recording events.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	using FReEchoCandidate = FReEchoAttackTargetCandidate;

	void AddCandidate(TArray<FReEchoCandidate>& Out, const FVector& Location, const bool bAlive, const int32 StableId)
	{
		FReEchoCandidate Candidate;
		Candidate.Location = Location;
		Candidate.bAlive = bAlive;
		Candidate.StableId = StableId;
		Out.Add(Candidate);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackModeTargetingTest, "ReEcho.AttackMode.Targeting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FReEchoAttackModeTargetingTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const float RangeCm = 150.0f;

	// Case A: nearest in range; dead and out-of-range ignored.
	{
		TArray<FReEchoCandidate> Candidates;
		AddCandidate(Candidates, FVector(50.0f, 0.0f, 0.0f), true, 5);
		AddCandidate(Candidates, FVector(100.0f, 0.0f, 0.0f), true, 9);
		AddCandidate(Candidates, FVector(10.0f, 0.0f, 0.0f), false, 1);
		AddCandidate(Candidates, FVector(200.0f, 0.0f, 0.0f), true, 3);
		TestEqual(TEXT("A: nearest in range selected"),
		          AReEchoPlayerPawn::SelectNearestEnemyInRange(Origin, RangeCm, Candidates), 0);
	}

	// Case B: equal distance -> stable id ascending wins.
	{
		TArray<FReEchoCandidate> Candidates;
		AddCandidate(Candidates, FVector(50.0f, 0.0f, 0.0f), true, 7);
		AddCandidate(Candidates, FVector(0.0f, 50.0f, 0.0f), true, 3);
		TestEqual(TEXT("B: lower stable id wins on tie"),
		          AReEchoPlayerPawn::SelectNearestEnemyInRange(Origin, RangeCm, Candidates), 1);
	}

	// Case C: nothing within range -> INDEX_NONE.
	{
		TArray<FReEchoCandidate> Candidates;
		AddCandidate(Candidates, FVector(200.0f, 0.0f, 0.0f), true, 1);
		TestEqual(TEXT("C: no target in range"),
		          AReEchoPlayerPawn::SelectNearestEnemyInRange(Origin, RangeCm, Candidates), INDEX_NONE);
	}

	// Case D: all dead -> INDEX_NONE.
	{
		TArray<FReEchoCandidate> Candidates;
		AddCandidate(Candidates, FVector(50.0f, 0.0f, 0.0f), false, 1);
		AddCandidate(Candidates, FVector(10.0f, 0.0f, 0.0f), false, 2);
		TestEqual(TEXT("D: all dead -> no target"),
		          AReEchoPlayerPawn::SelectNearestEnemyInRange(Origin, RangeCm, Candidates), INDEX_NONE);
	}

	// Case E: exactly at range boundary is included.
	{
		TArray<FReEchoCandidate> Candidates;
		AddCandidate(Candidates, FVector(150.0f, 0.0f, 0.0f), true, 1);
		TestEqual(TEXT("E: boundary included"),
		          AReEchoPlayerPawn::SelectNearestEnemyInRange(Origin, RangeCm, Candidates), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackModeStateTest, "ReEcho.AttackMode.StateAndHeldInput",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FReEchoAttackModeStateTest::RunTest(const FString& Parameters)
{
	AReEchoPlayerPawn* Pawn = NewObject<AReEchoPlayerPawn>();
	TestNotNull(TEXT("pawn created"), Pawn);

	// Default is automatic attack.
	TestTrue(TEXT("default mode is automatic"), Pawn->IsAutoAttackMode());

	// Pressing simulated held input is tracked.
	Pawn->PressAutoAttackInput();
	TestTrue(TEXT("held input tracked after press"), Pawn->IsAutoAttackInputHeld());
	Pawn->PressAutoAttackInput();
	TestTrue(TEXT("repeated automatic press is idempotent"), Pawn->IsAutoAttackInputHeld());

	// Switching to manual releases the simulated held input.
	Pawn->SetAutoAttackMode(false);
	TestFalse(TEXT("mode switched to manual"), Pawn->IsAutoAttackMode());
	TestFalse(TEXT("held input released on switch to manual"), Pawn->IsAutoAttackInputHeld());

	// Switching back to automatic.
	Pawn->SetAutoAttackMode(true);
	TestTrue(TEXT("mode switched back to automatic"), Pawn->IsAutoAttackMode());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackModeNoRecordingTest, "ReEcho.AttackMode.NoRecording",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FReEchoAttackModeNoRecordingTest::RunTest(const FString& Parameters)
{
	AReEchoPlayerPawn* Pawn = NewObject<AReEchoPlayerPawn>();
	TestNotNull(TEXT("pawn created"), Pawn);
	TestNotNull(TEXT("recorder present"), Pawn->GetRecorder());

	Pawn->GetRecorder()->BeginRecording(0, TEXT("AttackModeTest"), 1, FReEchoBuildSnapshot{});

	// Mode switches and auto-attack held input must not produce Echo recording events.
	Pawn->SetAutoAttackMode(false);
	Pawn->SetAutoAttackMode(true);
	Pawn->PressAutoAttackInput();
	Pawn->ReleaseAutoAttackInput();

	TestEqual(TEXT("no skill recording events from mode switch / auto attack"),
	          Pawn->GetRecorder()->GetRecording().Skills.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackModeSaveTest, "ReEcho.AttackMode.SaveAndMigration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FReEchoAttackModeSaveTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(GameInstance);
	Source->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	TestTrue(TEXT("new run defaults to automatic"), Source->IsAutomaticAttackMode());

	Source->SetAutomaticAttackMode(false);
	UReEchoRunSaveGame* CurrentSave = Source->CreateSaveSnapshot();
	TestEqual(TEXT("attack-mode save uses v6"), CurrentSave->SaveVersion, 6);
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(GameInstance);
	TestTrue(TEXT("v6 save restores"), Restored->RestoreSaveSnapshot(*CurrentSave));
	TestFalse(TEXT("continue restores manual mode"), Restored->IsAutomaticAttackMode());

	CurrentSave->SaveVersion = 5;
	CurrentSave->bAutomaticAttackMode = false;
	UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
	TestTrue(TEXT("v5 save migrates"), Migrated->RestoreSaveSnapshot(*CurrentSave));
	TestTrue(TEXT("v5 migration defaults to automatic"), Migrated->IsAutomaticAttackMode());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
