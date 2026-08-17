#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

namespace
{
UReEchoRunSubsystem* CreateStartedRun(UGameInstance* GameInstance)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	return Run;
}

FReEchoRecording MakeCompletedRecording(const UReEchoRunSubsystem& Run, const int32 EncounterIndex)
{
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = EncounterIndex;
	Recording.Duration = 20.0f + static_cast<float>(EncounterIndex);
	Recording.BuildSnapshot = Run.CurrentBuild;
	return Recording;
}

/** Stores three echoes so full-storage and selection rules can be exercised. */
TArray<FGuid> FillStorage(UReEchoRunSubsystem& Run)
{
	TArray<FGuid> StoredIds;
	for (int32 Index = 0; Index < ReEchoEchoStorage::MaxStorageCapacity; ++Index)
	{
		const FReEchoRecording Recording = MakeCompletedRecording(Run, Index + 1);
		Run.StagePendingRecording(Recording);
		Run.StorePendingRecording();
		StoredIds.Add(Recording.Id);
	}
	return StoredIds;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStoragePendingLifecycleTest,
                                 "ReEcho.Run.EchoStoragePendingLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStoragePendingLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);

	FReEchoEchoStorageSummary Summary = Run->GetEchoStorageSummary();
	TestFalse(TEXT("A fresh run has no pending echo"), Summary.bHasPendingRecording);
	TestFalse(TEXT("A fresh run has no rolling latest echo"), Summary.bHasLatestCompletedRecording);
	TestEqual(TEXT("A fresh run stores nothing"), Summary.StoredEchoes.Num(), 0);
	TestEqual(TEXT("A fresh run has prototype storage capacity"), Summary.StorageCapacity, 3);
	TestEqual(TEXT("A fresh run has not unlocked specific replay"), Summary.SpecificReplayLimit, 0);
	TestEqual(TEXT("A fresh run resolves no replay echo"), Run->GetEchoRecordings(1).Num(), 0);

	const FReEchoRecording First = MakeCompletedRecording(*Run, 1);
	Run->BeginEncounter();
	Run->CompleteEncounter(First, true, false);
	Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("A successful encounter stages a pending echo"), Summary.bHasPendingRecording);
	TestEqual(TEXT("The pending echo keeps its stable id"), Summary.PendingRecording.RecordingId, First.Id);
	TestTrue(TEXT("A successful encounter produces the rolling latest echo"), Summary.bHasLatestCompletedRecording);
	TestEqual(
	    TEXT("The rolling latest echo keeps its stable id"), Summary.LatestCompletedRecording.RecordingId, First.Id);
	TestEqual(TEXT("Staging alone occupies no storage slot"), Summary.StoredEchoes.Num(), 0);

	TestEqual(TEXT("Skipping the pending decision succeeds"),
	          Run->SkipPendingRecordingStorage(),
	          EReEchoEchoStorageResult::Success);
	Summary = Run->GetEchoStorageSummary();
	TestFalse(TEXT("Skipping clears the pending echo"), Summary.bHasPendingRecording);
	TestTrue(TEXT("Skipping keeps the rolling latest echo"), Summary.bHasLatestCompletedRecording);
	TestEqual(TEXT("Skipping keeps the same rolling latest identity"),
	          Summary.LatestCompletedRecording.RecordingId,
	          First.Id);
	TestEqual(TEXT("Skipping stores nothing"), Summary.StoredEchoes.Num(), 0);
	TestEqual(TEXT("Skipping twice reports no pending decision"),
	          Run->SkipPendingRecordingStorage(),
	          EReEchoEchoStorageResult::NoPendingRecording);
	TestEqual(TEXT("Storing without a pending decision is refused"),
	          Run->StorePendingRecording(),
	          EReEchoEchoStorageResult::NoPendingRecording);

	// The legacy facade must resolve from the new rolling latest state, not from an old history array.
	const TArray<FReEchoRecording> LegacyRecordings = Run->GetEchoRecordings(1);
	TestEqual(TEXT("The legacy facade resolves exactly one echo"), LegacyRecordings.Num(), 1);
	if (LegacyRecordings.Num() == 1)
	{
		TestEqual(TEXT("The legacy facade resolves the rolling latest echo"), LegacyRecordings[0].Id, First.Id);
	}

	// A second successful encounter overwrites the rolling latest echo without storing anything.
	const FReEchoRecording Second = MakeCompletedRecording(*Run, 2);
	Run->BeginEncounter();
	Run->CompleteEncounter(Second, true, false);
	Summary = Run->GetEchoStorageSummary();
	TestEqual(TEXT("A new successful encounter overwrites the rolling latest echo"),
	          Summary.LatestCompletedRecording.RecordingId,
	          Second.Id);
	TestEqual(TEXT("Overwriting the rolling latest echo stores nothing"), Summary.StoredEchoes.Num(), 0);

	UReEchoRunSubsystem* FailedRun = CreateStartedRun(GameInstance);
	const FReEchoRecording FailedRecording = MakeCompletedRecording(*FailedRun, 1);
	FailedRun->BeginEncounter();
	FailedRun->CompleteEncounter(FailedRecording, false, false);
	Summary = FailedRun->GetEchoStorageSummary();
	TestEqual(TEXT("A failed encounter fails the run"), FailedRun->Phase, EReEchoRunPhase::Failed);
	TestFalse(TEXT("A failed encounter stages no pending echo"), Summary.bHasPendingRecording);
	TestFalse(TEXT("A failed encounter produces no rolling latest echo"), Summary.bHasLatestCompletedRecording);
	TestEqual(TEXT("A failed encounter resolves no replay echo"), FailedRun->GetEchoRecordings(1).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageSlotsTest,
                                 "ReEcho.Run.EchoStorageSlots",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageSlotsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);

	TArray<FGuid> StoredIds;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FReEchoRecording Recording = MakeCompletedRecording(*Run, Index + 1);
		TestEqual(TEXT("Each successful encounter stages a pending echo"),
		          Run->StagePendingRecording(Recording),
		          EReEchoEchoStorageResult::Success);
		TestEqual(TEXT("A pending echo stores into a free slot"),
		          Run->StorePendingRecording(),
		          EReEchoEchoStorageResult::Success);
		StoredIds.Add(Recording.Id);
	}
	FReEchoEchoStorageSummary Summary = Run->GetEchoStorageSummary();
	TestEqual(TEXT("Three explicit stores fill prototype storage"), Summary.StoredEchoes.Num(), 3);
	TestFalse(TEXT("Storing clears the pending decision"), Run->HasPendingRecording());

	const FReEchoRecording Fourth = MakeCompletedRecording(*Run, 4);
	TestEqual(TEXT("A fourth successful encounter still stages"),
	          Run->StagePendingRecording(Fourth),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Full storage refuses to auto-evict an older echo"),
	          Run->StorePendingRecording(),
	          EReEchoEchoStorageResult::StorageFull);
	Summary = Run->GetEchoStorageSummary();
	TestEqual(TEXT("A refused store keeps every stored echo"), Summary.StoredEchoes.Num(), 3);
	TestTrue(TEXT("A refused store keeps the pending decision"), Summary.bHasPendingRecording);
	for (int32 Index = 0; Index < StoredIds.Num(); ++Index)
	{
		TestEqual(TEXT("A refused store preserves slot order and identity"),
		          Summary.StoredEchoes[Index].RecordingId,
		          StoredIds[Index]);
	}

	TestEqual(TEXT("An unknown replacement target is refused"),
	          Run->StorePendingRecordingReplacing(FGuid::NewGuid()),
	          EReEchoEchoStorageResult::InvalidReplacementTarget);
	TestEqual(TEXT("An invalid replacement target is refused"),
	          Run->StorePendingRecordingReplacing(FGuid()),
	          EReEchoEchoStorageResult::InvalidReplacementTarget);
	Summary = Run->GetEchoStorageSummary();
	TestEqual(TEXT("A refused replacement keeps every stored echo"), Summary.StoredEchoes.Num(), 3);
	TestTrue(TEXT("A refused replacement keeps the pending decision"), Summary.bHasPendingRecording);

	TestEqual(TEXT("An explicit replacement target succeeds"),
	          Run->StorePendingRecordingReplacing(StoredIds[1]),
	          EReEchoEchoStorageResult::Success);
	Summary = Run->GetEchoStorageSummary();
	TestEqual(TEXT("Replacement keeps storage at capacity"), Summary.StoredEchoes.Num(), 3);
	TestEqual(TEXT("Replacement reuses the named slot"), Summary.StoredEchoes[1].RecordingId, Fourth.Id);
	TestEqual(TEXT("Replacement leaves the first slot alone"), Summary.StoredEchoes[0].RecordingId, StoredIds[0]);
	TestEqual(TEXT("Replacement leaves the third slot alone"), Summary.StoredEchoes[2].RecordingId, StoredIds[2]);
	TestFalse(TEXT("Replacement clears the pending decision"), Summary.bHasPendingRecording);

	const FReEchoRecording Fifth = MakeCompletedRecording(*Run, 5);
	TestEqual(TEXT("A fifth successful encounter stages"),
	          Run->StagePendingRecording(Fifth),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("A replaced id is a stale replacement target"),
	          Run->StorePendingRecordingReplacing(StoredIds[1]),
	          EReEchoEchoStorageResult::InvalidReplacementTarget);

	FReEchoRecording Duplicate = MakeCompletedRecording(*Run, 6);
	Duplicate.Id = StoredIds[0];
	TestEqual(TEXT("Staging an already stored id is refused"),
	          Run->StagePendingRecording(Duplicate),
	          EReEchoEchoStorageResult::DuplicateRecordingId);
	Summary = Run->GetEchoStorageSummary();
	TestTrue(TEXT("A refused stage keeps the previous pending echo"), Summary.bHasPendingRecording);
	TestEqual(
	    TEXT("A refused stage keeps the previous pending identity"), Summary.PendingRecording.RecordingId, Fifth.Id);
	TestEqual(TEXT("A refused stage does not overwrite the rolling latest echo"),
	          Summary.LatestCompletedRecording.RecordingId,
	          Fifth.Id);

	TestEqual(TEXT("Staging an invalid recording id is refused"),
	          Run->StagePendingRecording(FReEchoRecording()),
	          EReEchoEchoStorageResult::InvalidRecordingId);
	TestEqual(TEXT("A refused stage keeps storage untouched"), Run->GetEchoStorageSummary().StoredEchoes.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageCapabilitiesTest,
                                 "ReEcho.Run.EchoStorageCapabilities",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageCapabilitiesTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
	const TArray<FGuid> StoredIds = FillStorage(*Run);
	TestEqual(TEXT("Storage is full before capability checks"), Run->GetEchoStorageSummary().StoredEchoes.Num(), 3);

	// Storage capacity and specific replay limit are two independent values.
	TestEqual(TEXT("Storage capacity starts at the prototype maximum"), Run->GetStorageCapacity(), 3);
	TestEqual(TEXT("Specific replay starts unavailable"), Run->GetSpecificReplayLimit(), 0);
	TestEqual(TEXT("Unlocking specific multi replay succeeds"),
	          Run->SetSpecificReplayLimit(2),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Changing the replay limit leaves storage capacity untouched"), Run->GetStorageCapacity(), 3);
	TestEqual(
	    TEXT("Re-affirming storage capacity succeeds"), Run->SetStorageCapacity(3), EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Changing storage capacity leaves the replay limit untouched"), Run->GetSpecificReplayLimit(), 2);

	TestEqual(TEXT("Storage capacity above the prototype maximum is refused"),
	          Run->SetStorageCapacity(4),
	          EReEchoEchoStorageResult::InvalidStorageCapacity);
	TestEqual(TEXT("Negative storage capacity is refused"),
	          Run->SetStorageCapacity(-1),
	          EReEchoEchoStorageResult::InvalidStorageCapacity);
	TestEqual(TEXT("Storage capacity cannot shrink below the stored echoes"),
	          Run->SetStorageCapacity(2),
	          EReEchoEchoStorageResult::InvalidStorageCapacity);
	TestEqual(TEXT("A refused capacity change keeps the old capacity"), Run->GetStorageCapacity(), 3);
	TestEqual(TEXT("A replay limit above the prototype maximum is refused"),
	          Run->SetSpecificReplayLimit(4),
	          EReEchoEchoStorageResult::InvalidReplayLimit);
	TestEqual(TEXT("A negative replay limit is refused"),
	          Run->SetSpecificReplayLimit(-1),
	          EReEchoEchoStorageResult::InvalidReplayLimit);
	TestEqual(TEXT("A refused replay limit change keeps the old limit"), Run->GetSpecificReplayLimit(), 2);

	TestEqual(TEXT("Selecting more echoes than the replay limit is refused"),
	          Run->SetSelectedReplayIds({StoredIds[0], StoredIds[1], StoredIds[2]}),
	          EReEchoEchoStorageResult::ReplayLimitExceeded);
	TestEqual(TEXT("A refused selection changes nothing"), Run->GetSelectedReplayIds().Num(), 0);
	TestEqual(TEXT("A duplicated selection id is refused"),
	          Run->SetSelectedReplayIds({StoredIds[0], StoredIds[0]}),
	          EReEchoEchoStorageResult::DuplicateRecordingId);
	TestEqual(TEXT("Selecting an echo that is not stored is refused"),
	          Run->SetSelectedReplayIds({FGuid::NewGuid()}),
	          EReEchoEchoStorageResult::InvalidRecordingId);
	TestEqual(TEXT("Selecting an invalid guid is refused"),
	          Run->SetSelectedReplayIds({FGuid()}),
	          EReEchoEchoStorageResult::InvalidRecordingId);
	TestEqual(TEXT("Every refused selection leaves the selection empty"), Run->GetSelectedReplayIds().Num(), 0);

	TestEqual(TEXT("A legal selection succeeds"),
	          Run->SetSelectedReplayIds({StoredIds[2], StoredIds[0]}),
	          EReEchoEchoStorageResult::Success);
	const TArray<FReEchoRecording> Resolved = Run->ResolveReplayRecordings(3);
	TestEqual(TEXT("Specific multi replay resolves the selected echoes"), Resolved.Num(), 2);
	if (Resolved.Num() == 2)
	{
		TestEqual(TEXT("Selection order drives resolution, not slot order"), Resolved[0].Id, StoredIds[2]);
		TestEqual(TEXT("Selection order drives resolution for the second echo"), Resolved[1].Id, StoredIds[0]);
	}

	TestEqual(
	    TEXT("Lowering the replay limit succeeds"), Run->SetSpecificReplayLimit(1), EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Lowering the replay limit truncates the selection deterministically"),
	          Run->GetSelectedReplayIds().Num(),
	          1);
	if (Run->GetSelectedReplayIds().Num() == 1)
	{
		TestEqual(TEXT("Truncation keeps the earliest selected echo"), Run->GetSelectedReplayIds()[0], StoredIds[2]);
	}

	TestEqual(
	    TEXT("Losing specific replay succeeds"), Run->SetSpecificReplayLimit(0), EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Losing specific replay clears the selection"), Run->GetSelectedReplayIds().Num(), 0);
	const TArray<FReEchoRecording> Fallback = Run->GetEchoRecordings(1);
	TestEqual(TEXT("Without specific replay exactly one echo resolves"), Fallback.Num(), 1);
	if (Fallback.Num() == 1)
	{
		TestEqual(TEXT("Without specific replay the rolling latest echo resolves"), Fallback[0].Id, StoredIds[2]);
	}

	// Replacing a stored echo must drop the now stale selection entry.
	TestEqual(TEXT("Re-unlocking specific multi replay succeeds"),
	          Run->SetSpecificReplayLimit(2),
	          EReEchoEchoStorageResult::Success);
	const TArray<FReEchoRecording> UnselectedDefault = Run->ResolveReplayRecordings(3);
	TestEqual(TEXT("Unlocked but unselected falls back to one echo"), UnselectedDefault.Num(), 1);
	if (UnselectedDefault.Num() == 1)
	{
		TestEqual(TEXT("The unselected default replay is the rolling previous encounter"),
		          UnselectedDefault[0].Id,
		          StoredIds[2]);
	}
	TestEqual(TEXT("Selecting two stored echoes succeeds"),
	          Run->SetSelectedReplayIds({StoredIds[0], StoredIds[1]}),
	          EReEchoEchoStorageResult::Success);
	const FReEchoRecording Replacement = MakeCompletedRecording(*Run, 4);
	TestEqual(TEXT("A replacement candidate stages"),
	          Run->StagePendingRecording(Replacement),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Replacing a selected echo succeeds"),
	          Run->StorePendingRecordingReplacing(StoredIds[0]),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Replacing a stored echo drops its stale selection"), Run->GetSelectedReplayIds().Num(), 1);
	if (Run->GetSelectedReplayIds().Num() == 1)
	{
		TestEqual(TEXT("Surviving selections keep their order"), Run->GetSelectedReplayIds()[0], StoredIds[1]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageSaveMigrationTest,
                                 "ReEcho.Run.EchoStorageSaveMigration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageSaveMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Source = CreateStartedRun(GameInstance);
	const FReEchoRecording Older = MakeCompletedRecording(*Source, 1);
	const FReEchoRecording Newer = MakeCompletedRecording(*Source, 2);

	const auto MakeLegacyV4Save = [Source, &Older, &Newer](const FGuid& AnchorId, const bool bIncludeHistory)
	{
		UReEchoRunSaveGame* Legacy = NewObject<UReEchoRunSaveGame>(GetTransientPackage());
		Legacy->SaveVersion = 4;
		Legacy->SavedPhase = EReEchoRunPhase::Planning;
		// Keep the migration fixture before encounter progression. Plan48 explicitly rejects
		// progressed legacy six-encounter saves rather than reinterpreting their history.
		Legacy->EncounterIndex = 0;
		Legacy->TimeShards = 30;
		Legacy->CurrentBuild = Source->CurrentBuild;
		if (bIncludeHistory)
		{
			// v4 stored the newest encounter first.
			Legacy->RecordingHistory = {Newer, Older};
		}
		Legacy->AnchorId = AnchorId;
		return Legacy;
	};

	// v4 without an anchor: the newest history entry becomes the rolling latest echo only.
	{
		UReEchoRunSaveGame* Legacy = MakeLegacyV4Save(FGuid(), true);
		UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
		TestTrue(TEXT("A v4 archive without an anchor still resumes"), Migrated->RestoreSaveSnapshot(*Legacy));
		const FReEchoEchoStorageSummary Summary = Migrated->GetEchoStorageSummary();
		TestTrue(TEXT("v4 migration keeps the newest encounter as the rolling latest echo"),
		         Summary.bHasLatestCompletedRecording);
		TestEqual(TEXT("v4 migration picks the newest history entry"),
		          Summary.LatestCompletedRecording.RecordingId,
		          Newer.Id);
		TestEqual(TEXT("v4 migration without an anchor stores nothing"), Summary.StoredEchoes.Num(), 0);
		TestEqual(TEXT("v4 migration without an anchor selects nothing"), Summary.SelectedReplayIds.Num(), 0);
		TestEqual(TEXT("v4 migration without an anchor leaves specific replay locked"), Summary.SpecificReplayLimit, 0);
		TestFalse(TEXT("v4 migration produces no pending decision"), Summary.bHasPendingRecording);
		const TArray<FReEchoRecording> Resolved = Migrated->GetEchoRecordings(1);
		TestEqual(TEXT("v4 migration without an anchor resolves the rolling latest echo"), Resolved.Num(), 1);
		if (Resolved.Num() == 1)
		{
			TestEqual(TEXT("v4 default replay is the previous encounter"), Resolved[0].Id, Newer.Id);
		}
	}

	// v4 with a resolvable anchor migrates to specific single replay.
	{
		UReEchoRunSaveGame* Legacy = MakeLegacyV4Save(Older.Id, true);
		UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
		TestTrue(TEXT("A v4 archive with a valid anchor resumes"), Migrated->RestoreSaveSnapshot(*Legacy));
		const FReEchoEchoStorageSummary Summary = Migrated->GetEchoStorageSummary();
		TestEqual(TEXT("A migrated anchor occupies exactly one storage slot"), Summary.StoredEchoes.Num(), 1);
		if (Summary.StoredEchoes.Num() == 1)
		{
			TestEqual(TEXT("The migrated slot holds the anchored echo"), Summary.StoredEchoes[0].RecordingId, Older.Id);
			TestTrue(TEXT("The migrated anchor is selected"), Summary.StoredEchoes[0].bSelectedForNextEncounter);
		}
		TestEqual(TEXT("A migrated anchor becomes specific single replay"), Summary.SpecificReplayLimit, 1);
		TestEqual(TEXT("A migrated anchor selects exactly one echo"), Summary.SelectedReplayIds.Num(), 1);
		TestTrue(TEXT("A migrated anchor still keeps the rolling latest echo"), Summary.bHasLatestCompletedRecording);
		TestEqual(TEXT("The rolling latest echo stays the newest encounter"),
		          Summary.LatestCompletedRecording.RecordingId,
		          Newer.Id);
		const TArray<FReEchoRecording> Resolved = Migrated->GetEchoRecordings(1);
		TestEqual(TEXT("A migrated anchor resolves one echo"), Resolved.Num(), 1);
		if (Resolved.Num() == 1)
		{
			TestEqual(
			    TEXT("A migrated anchor resolves the anchored echo, not the newest one"), Resolved[0].Id, Older.Id);
		}
	}

	// v4 with an anchor whose recording is missing normalizes without substituting another encounter.
	{
		const FGuid MissingAnchorId = FGuid::NewGuid();
		UReEchoRunSaveGame* Legacy = MakeLegacyV4Save(MissingAnchorId, true);
		UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
		TestTrue(TEXT("A v4 archive with a dangling anchor still resumes"), Migrated->RestoreSaveSnapshot(*Legacy));
		const FReEchoEchoStorageSummary Summary = Migrated->GetEchoStorageSummary();
		TestEqual(TEXT("A dangling anchor stores nothing"), Summary.StoredEchoes.Num(), 0);
		TestEqual(TEXT("A dangling anchor selects nothing"), Summary.SelectedReplayIds.Num(), 0);
		TestEqual(TEXT("A dangling anchor leaves specific replay locked"), Summary.SpecificReplayLimit, 0);
		TestTrue(TEXT("A dangling anchor still keeps the rolling latest echo"), Summary.bHasLatestCompletedRecording);
		TestEqual(TEXT("A dangling anchor never substitutes another encounter into a slot"),
		          Summary.LatestCompletedRecording.RecordingId,
		          Newer.Id);
	}

	// v4 with an anchor but no history at all also normalizes instead of crashing.
	{
		UReEchoRunSaveGame* Legacy = MakeLegacyV4Save(Older.Id, false);
		UReEchoRunSubsystem* Migrated = NewObject<UReEchoRunSubsystem>(GameInstance);
		TestTrue(TEXT("A v4 archive with an empty history resumes"), Migrated->RestoreSaveSnapshot(*Legacy));
		const FReEchoEchoStorageSummary Summary = Migrated->GetEchoStorageSummary();
		TestFalse(TEXT("An empty v4 history produces no rolling latest echo"), Summary.bHasLatestCompletedRecording);
		TestEqual(TEXT("An empty v4 history stores nothing"), Summary.StoredEchoes.Num(), 0);
		TestEqual(TEXT("An empty v4 history leaves specific replay locked"), Summary.SpecificReplayLimit, 0);
		TestEqual(TEXT("An empty v4 history resolves no replay echo"), Migrated->GetEchoRecordings(1).Num(), 0);
	}

	// v5 round trip: every locked field survives save and restore, including the pending decision.
	{
		UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
		const TArray<FGuid> StoredIds = FillStorage(*Run);
		TestEqual(TEXT("Round trip source unlocks specific multi replay"),
		          Run->SetSpecificReplayLimit(2),
		          EReEchoEchoStorageResult::Success);
		TestEqual(TEXT("Round trip source selects two stored echoes"),
		          Run->SetSelectedReplayIds({StoredIds[2], StoredIds[0]}),
		          EReEchoEchoStorageResult::Success);
		const FReEchoRecording Undecided = MakeCompletedRecording(*Run, 4);
		TestEqual(TEXT("Round trip source keeps an unresolved pending decision"),
		          Run->StagePendingRecording(Undecided),
		          EReEchoEchoStorageResult::Success);

		UReEchoRunSaveGame* Snapshot = Run->CreateSaveSnapshot();
		TestNotNull(TEXT("A v5 snapshot is created"), Snapshot);
		TestEqual(TEXT("The snapshot writes the current save version"),
		          Snapshot->SaveVersion,
		          UReEchoRunSaveGame::CurrentSaveVersion);
		TestEqual(TEXT("v5 never writes the legacy history array"), Snapshot->RecordingHistory.Num(), 0);
		TestFalse(TEXT("v5 never writes the legacy anchor"), Snapshot->AnchorId.IsValid());

		TArray<uint8> SerializedSave;
		TestTrue(TEXT("A v5 snapshot serializes"), UGameplayStatics::SaveGameToMemory(Snapshot, SerializedSave));
		UReEchoRunSaveGame* Deserialized =
		    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedSave));
		TestNotNull(TEXT("A v5 snapshot deserializes"), Deserialized);
		if (Deserialized)
		{
			UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(GameInstance);
			TestTrue(TEXT("A v5 snapshot restores"), Restored->RestoreSaveSnapshot(*Deserialized));
			const FReEchoEchoStorageSummary Summary = Restored->GetEchoStorageSummary();
			TestEqual(TEXT("Every stored echo survives the round trip"), Summary.StoredEchoes.Num(), 3);
			for (int32 Index = 0; Index < Summary.StoredEchoes.Num(); ++Index)
			{
				TestEqual(TEXT("Stored echo identity and slot order survive the round trip"),
				          Summary.StoredEchoes[Index].RecordingId,
				          StoredIds[Index]);
			}
			TestEqual(TEXT("The replay limit survives the round trip"), Summary.SpecificReplayLimit, 2);
			TestEqual(TEXT("Storage capacity survives the round trip"), Summary.StorageCapacity, 3);
			TestEqual(TEXT("The selection survives the round trip"), Summary.SelectedReplayIds.Num(), 2);
			if (Summary.SelectedReplayIds.Num() == 2)
			{
				TestEqual(TEXT("Selection order survives the round trip"), Summary.SelectedReplayIds[0], StoredIds[2]);
				TestEqual(
				    TEXT("The second selection survives the round trip"), Summary.SelectedReplayIds[1], StoredIds[0]);
			}
			TestTrue(TEXT("The unresolved pending decision survives the round trip"), Summary.bHasPendingRecording);
			TestEqual(TEXT("The pending echo keeps its identity across the round trip"),
			          Summary.PendingRecording.RecordingId,
			          Undecided.Id);
			TestTrue(TEXT("The rolling latest echo survives the round trip"), Summary.bHasLatestCompletedRecording);
			TestEqual(TEXT("The rolling latest echo keeps its identity across the round trip"),
			          Summary.LatestCompletedRecording.RecordingId,
			          Undecided.Id);
			FReEchoRecording RestoredPending;
			TestTrue(TEXT("The full pending recording payload restores"),
			         Restored->TryGetPendingRecording(RestoredPending));
			TestEqual(TEXT("The pending recording duration restores"), RestoredPending.Duration, Undecided.Duration);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoStorageRecordingPayloadTest,
                                 "ReEcho.Run.EchoStorageRecordingPayloadUnchanged",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoStorageRecordingPayloadTest::RunTest(const FString& Parameters)
{
	// Plan30 must not touch recording semantics: automatic basic attacks stay out of the event list and
	// only explicit active skill calls append events.
	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>(GetTransientPackage());
	FReEchoBuildSnapshot InitialBuild;
	InitialBuild.CharacterId = TEXT("J_CAT");
	InitialBuild.WeaponId = TEXT("W_J_02");
	Recorder->BeginRecording(1, TEXT("TestArena"), 4242, InitialBuild);
	Recorder->AdvanceRecording(0.05f, FVector(10.0f, 0.0f, 0.0f));
	Recorder->AdvanceRecording(0.10f, FVector(20.0f, 0.0f, 0.0f));
	Recorder->RecordSkill(0.10f, FVector(20.0f, 0.0f, 0.0f), TEXT("ActiveAOE"));
	Recorder->AdvanceRecording(0.15f, FVector(30.0f, 0.0f, 0.0f));
	FReEchoRecording Recording = Recorder->FinishRecording(0.15f);

	TestTrue(TEXT("Advancing the recorder still samples positions"), Recording.Positions.Num() > 0);
	TestEqual(TEXT("Automatic basic attacks are still not recorded as skill events"), Recording.Skills.Num(), 1);
	if (Recording.Skills.Num() == 1)
	{
		TestEqual(
		    TEXT("Active skill events keep their skill id"), Recording.Skills[0].SkillId, FName(TEXT("ActiveAOE")));
	}
	TestEqual(TEXT("Recording keeps its random seed"), Recording.RandomSeed, 4242);
	TestEqual(TEXT("Recording keeps its map id"), Recording.MapId, FName(TEXT("TestArena")));
	TestTrue(TEXT("Recording keeps a stable identity"), Recording.Id.IsValid());

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = CreateStartedRun(GameInstance);
	Recording.BuildSnapshot = Run->CurrentBuild;
	TestEqual(TEXT("A recorded echo stages"), Run->StagePendingRecording(Recording), EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("A recorded echo stores"), Run->StorePendingRecording(), EReEchoEchoStorageResult::Success);

	FReEchoRecording Stored;
	TestTrue(TEXT("A stored echo resolves by its stable id"), Run->TryGetStoredEcho(Recording.Id, Stored));
	TestEqual(TEXT("Storage preserves every position sample"), Stored.Positions.Num(), Recording.Positions.Num());
	TestEqual(TEXT("Storage preserves every skill event"), Stored.Skills.Num(), Recording.Skills.Num());
	TestEqual(TEXT("Storage preserves the recording duration"), Stored.Duration, Recording.Duration);
	TestEqual(TEXT("Storage preserves the random seed"), Stored.RandomSeed, Recording.RandomSeed);
	TestEqual(
	    TEXT("Storage preserves the locked weapon"), Stored.BuildSnapshot.WeaponId, Recording.BuildSnapshot.WeaponId);
	return true;
}

#endif
