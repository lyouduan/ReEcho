#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

namespace
{
UReEchoRunSubsystem* CreateReplayStartedRun(UGameInstance* GameInstance)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	return Run;
}

FReEchoRecording MakeCompletedRecording(UReEchoRunSubsystem* Run, const FName MapId, const int32 EncounterIndex)
{
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = EncounterIndex;
	Recording.Duration = 10.0f + static_cast<float>(EncounterIndex);
	Recording.MapId = MapId;
	Recording.BuildSnapshot = Run->CurrentBuild;
	return Recording;
}

void CompleteWithoutAnchor(UReEchoRunSubsystem* Run, const FReEchoRecording& Recording)
{
	Run->StagePendingRecording(Recording);
	Run->SkipPendingRecordingStorage();
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverLatestFallback,
                                 "ReEcho.Run.EchoReplayResolver.LatestFallback",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverLatestFallback::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = CreateReplayStartedRun(GameInstance);
	const FReEchoRecording First = MakeCompletedRecording(Run, TEXT("Map_A"), 1);
	const FReEchoRecording Latest = MakeCompletedRecording(Run, TEXT("Map_B"), 2);
	CompleteWithoutAnchor(Run, First);
	CompleteWithoutAnchor(Run, Latest);

	const TArray<FReEchoRecording> Resolved = Run->ResolveReplayRecordings(ReEchoTimeAnchor::MaximumResolvedEchoes);
	TestEqual(TEXT("Automatic replay respects the default one-echo rule"), Resolved.Num(), 1);
	if (Resolved.Num() == 1)
	{
		TestEqual(TEXT("The rolling latest recording resolves"), Resolved[0].Id, Latest.Id);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverTimeAnchor,
                                 "ReEcho.Run.EchoReplayResolver.TimeAnchor",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverTimeAnchor::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = CreateReplayStartedRun(GameInstance);
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
	const FReEchoRecording Anchor = MakeCompletedRecording(Run, TEXT("Map_A"), 1);
	const FReEchoRecording Latest = MakeCompletedRecording(Run, TEXT("Map_B"), 2);
	Run->StagePendingRecording(Anchor);
	TestEqual(TEXT("The encounter becomes the single time anchor"),
	          Run->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	CompleteWithoutAnchor(Run, Latest);
	const FReEchoEchoStorageSummary Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("Summary exposes a valid time anchor"), Summary.bHasTimeAnchor);
	TestEqual(TEXT("Summary exposes the anchor id"), Summary.TimeAnchorRecording.RecordingId, Anchor.Id);

	const TArray<FReEchoRecording> Resolved = Run->ResolveReplayRecordings(ReEchoTimeAnchor::MaximumResolvedEchoes);
	TestEqual(TEXT("A time anchor resolves exactly one echo"), Resolved.Num(), 1);
	if (Resolved.Num() == 1)
	{
		TestEqual(TEXT("The anchored echo wins over rolling latest"), Resolved[0].Id, Anchor.Id);
	}

	const FReEchoRecording Replacement = MakeCompletedRecording(Run, TEXT("Map_C"), 3);
	Run->StagePendingRecording(Replacement);
	TestEqual(TEXT("Replacing the time anchor succeeds"),
	          Run->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("The new encounter is now the only anchor"),
	          Run->GetEchoStorageSummary().TimeAnchorRecording.RecordingId,
	          Replacement.Id);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverAnchorSaveResume,
                                 "ReEcho.Run.EchoReplayResolver.AnchorSaveResume",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverAnchorSaveResume::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = CreateReplayStartedRun(GameInstance);
	Source->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
	const FReEchoRecording Anchor = MakeCompletedRecording(Source, TEXT("Map_A"), 1);
	Source->StagePendingRecording(Anchor);
	TestEqual(TEXT("Anchor is set before saving"),
	          Source->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);

	UReEchoRunSaveGame* Snapshot = Source->CreateSaveSnapshot();
	TestNotNull(TEXT("Save snapshot is created"), Snapshot);
	TestEqual(TEXT("Deprecated selection ids are written empty"), Snapshot->SelectedReplayIds.Num(), 0);
	TestEqual(TEXT("Deprecated storage capacity is written as zero"), Snapshot->StorageCapacity, 0);
	TestEqual(TEXT("Deprecated replay limit is written as zero"), Snapshot->SpecificReplayLimit, 0);

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = CreateReplayStartedRun(RestoredGameInstance);
	TestTrue(TEXT("Save restores"), Restored->RestoreSaveSnapshot(*Snapshot));
	const FReEchoEchoStorageSummary Summary = Restored->GetEchoStorageSummary();
	TestTrue(TEXT("Single time anchor survives the round trip"), Summary.bHasTimeAnchor);
	TestEqual(TEXT("Restored time anchor id matches"), Summary.TimeAnchorRecording.RecordingId, Anchor.Id);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
