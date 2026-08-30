#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

namespace
{
UReEchoRunSubsystem* CreateStartedRun(UGameInstance* GameInstance)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	return Run;
}

FReEchoRecording MakeCompletedRecording(const UReEchoRunSubsystem& Run, const int32 EncounterIndex)
{
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = EncounterIndex;
	Recording.Duration = 20.0f + static_cast<float>(EncounterIndex);
	Recording.MapId = TEXT("TestArena");
	Recording.BuildSnapshot = Run.CurrentBuild;
	return Recording;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStoragePendingLifecycleTest,
                                 "ReEcho.Run.EchoStoragePendingLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStoragePendingLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
	FReEchoEchoStorageSummary Summary = Run->GetEchoStorageSummary();
	TestFalse(TEXT("A fresh run has no pending echo"), Summary.bHasPendingRecording);
	TestFalse(TEXT("A fresh run has no time anchor"), Summary.bHasTimeAnchor);

	const FReEchoRecording First = MakeCompletedRecording(*Run, 1);
	TestEqual(TEXT("A valid encounter stages"), Run->StagePendingRecording(First), EReEchoEchoStorageResult::Success);
	Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("The staged encounter is pending"), Summary.bHasPendingRecording);
	TestEqual(TEXT("Pending identity is preserved"), Summary.PendingRecording.RecordingId, First.Id);
	TestTrue(TEXT("The rolling latest echo is independent"), Summary.bHasLatestCompletedRecording);

	TestEqual(TEXT("Skipping succeeds"), Run->SkipPendingRecordingStorage(), EReEchoEchoStorageResult::Success);
	Summary = Run->GetEchoStorageSummary();
	TestFalse(TEXT("Skipping clears only the pending decision"), Summary.bHasPendingRecording);
	TestTrue(TEXT("Skipping keeps automatic latest replay"), Summary.bHasLatestCompletedRecording);
	TestEqual(TEXT("Skipping twice reports no pending recording"),
	          Run->SkipPendingRecordingStorage(),
	          EReEchoEchoStorageResult::NoPendingRecording);
	TestEqual(TEXT("An invalid recording id is rejected"),
	          Run->StagePendingRecording(FReEchoRecording{}),
	          EReEchoEchoStorageResult::InvalidRecordingId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSingleTimeAnchorTest,
                                 "ReEcho.Run.SingleTimeAnchor",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSingleTimeAnchorTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
	const FReEchoRecording First = MakeCompletedRecording(*Run, 1);
	Run->StagePendingRecording(First);
	TestEqual(TEXT("The pending encounter becomes the anchor"),
	          Run->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	FReEchoEchoStorageSummary Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("One time anchor is exposed"), Summary.bHasTimeAnchor);
	TestEqual(TEXT("The first anchor identity is preserved"), Summary.TimeAnchorRecording.RecordingId, First.Id);

	const FReEchoRecording Second = MakeCompletedRecording(*Run, 2);
	Run->StagePendingRecording(Second);
	TestEqual(TEXT("A new encounter replaces the only anchor directly"),
	          Run->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("The replacement remains the only anchor"), Summary.bHasTimeAnchor);
	TestEqual(
	    TEXT("The second encounter replaced the first anchor"), Summary.TimeAnchorRecording.RecordingId, Second.Id);
	TestFalse(TEXT("Committing the anchor clears the pending decision"), Summary.bHasPendingRecording);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageSaveMigrationTest,
                                 "ReEcho.Run.EchoStorageSaveMigration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageSaveMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = CreateStartedRun(GameInstance);
	const FReEchoRecording Older = MakeCompletedRecording(*Source, 1);
	const FReEchoRecording Newer = MakeCompletedRecording(*Source, 2);

	auto MakeLegacy = [&](const FGuid AnchorId)
	{
		UReEchoRunSaveGame* Legacy = NewObject<UReEchoRunSaveGame>();
		Legacy->SaveVersion = 4;
		Legacy->SavedPhase = EReEchoRunPhase::Planning;
		Legacy->CurrentBuild = Source->CurrentBuild;
		Legacy->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
		Legacy->RecordingHistory = {Newer, Older};
		Legacy->AnchorId = AnchorId;
		return Legacy;
	};

	UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
	TestTrue(TEXT("A valid v4 anchor migrates"), Migrated->RestoreSaveSnapshot(*MakeLegacy(Older.Id)));
	FReEchoEchoStorageSummary Summary = Migrated->GetEchoStorageSummary();
	TestTrue(TEXT("The migrated archive has one anchor"), Summary.bHasTimeAnchor);
	TestEqual(TEXT("The migrated anchor identity is preserved"), Summary.TimeAnchorRecording.RecordingId, Older.Id);

	UReEchoRunSubsystem* Dangling = NewObject<UReEchoRunSubsystem>(GameInstance);
	TestTrue(TEXT("A dangling v4 anchor still restores"), Dangling->RestoreSaveSnapshot(*MakeLegacy(FGuid::NewGuid())));
	Summary = Dangling->GetEchoStorageSummary();
	TestFalse(TEXT("A dangling v4 anchor is discarded"), Summary.bHasTimeAnchor);
	TestTrue(TEXT("A dangling anchor keeps automatic latest replay"), Summary.bHasLatestCompletedRecording);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageRecordingPayloadTest,
                                 "ReEcho.Run.EchoStorageRecordingPayloadUnchanged",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageRecordingPayloadTest::RunTest(const FString& Parameters)
{
	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>();
	FReEchoBuildSnapshot InitialBuild;
	InitialBuild.CharacterId = TEXT("J_SPADE");
	InitialBuild.WeaponId = TEXT("W_J_01");
	Recorder->BeginRecording(1, TEXT("TestArena"), 4242, InitialBuild);
	Recorder->AdvanceRecording(0.05f, FVector(10.0f, 0.0f, 0.0f));
	Recorder->RecordSkill(0.10f, FVector(20.0f, 0.0f, 0.0f), TEXT("ActiveAOE"));
	FReEchoRecording Recording = Recorder->FinishRecording(0.15f);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
	Recording.BuildSnapshot = Run->CurrentBuild;
	Run->StagePendingRecording(Recording);
	Run->StorePendingRecordingAsTimeAnchor();
	const TArray<FReEchoRecording> Resolved = Run->ResolveReplayRecordings(1);
	TestEqual(TEXT("The time anchor resolves one recording"), Resolved.Num(), 1);
	if (Resolved.Num() == 1)
	{
		TestEqual(TEXT("Every position sample survives"), Resolved[0].Positions.Num(), Recording.Positions.Num());
		TestEqual(TEXT("Every skill event survives"), Resolved[0].Skills.Num(), Recording.Skills.Num());
		TestEqual(TEXT("Duration survives"), Resolved[0].Duration, Recording.Duration);
		TestEqual(TEXT("Random seed survives"), Resolved[0].RandomSeed, Recording.RandomSeed);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
