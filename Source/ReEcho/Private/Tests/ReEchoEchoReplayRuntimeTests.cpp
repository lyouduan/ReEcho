#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

namespace
{
UReEchoRunSubsystem* CreateReplayStartedRun(UGameInstance* GameInstance)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	return Run;
}

FReEchoRecording MakeCompletedRecording(UReEchoRunSubsystem* Sub, const FName MapId, const int32 EncounterIndex)
{
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = EncounterIndex;
	Recording.Duration = 10.0f + static_cast<float>(EncounterIndex);
	Recording.MapId = MapId;
	Recording.BuildSnapshot = Sub->CurrentBuild;
	return Recording;
}
}

// Limit zero (specific replay not yet unlocked) only ever returns the rolling previous encounter.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverLatestFallback,
                                 "ReEcho.Run.EchoReplayResolver.LatestFallback",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverLatestFallback::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Sub = CreateReplayStartedRun(GameInstance);

	const FReEchoRecording RecA = MakeCompletedRecording(Sub, FName(TEXT("Map_A")), 1);
	Sub->StagePendingRecording(RecA);
	Sub->StorePendingRecording();

	TestEqual(TEXT("Default SpecificReplayLimit is zero (ability not unlocked)"), Sub->GetSpecificReplayLimit(), 0);

	const TArray<FReEchoRecording> Latest = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Limit zero resolves exactly the rolling latest"), Latest.Num(), 1);
	if (Latest.Num() == 1)
	{
		TestTrue(TEXT("Resolved echo is the latest completed recording"), Latest[0].Id == RecA.Id);
	}

	// Resolver must be read-only: it never mutates storage or selection.
	TestEqual(TEXT("Resolver did not mutate StoredEchoes"), Sub->GetEchoStorageSummary().StoredEchoes.Num(), 1);
	TestEqual(TEXT("Resolver did not mutate selection"), Sub->GetSelectedReplayIds().Num(), 0);

	// Even a stored echo plus a residual selection must not override limit-zero latest behavior.
	(void)Sub->SetSpecificReplayLimit(1);
	(void)Sub->SetSelectedReplayIds({RecA.Id});
	(void)Sub->SetSpecificReplayLimit(0);
	const TArray<FReEchoRecording> StillLatest = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Residual selection cannot override limit-zero latest"), StillLatest.Num(), 1);
	if (StillLatest.Num() == 1)
	{
		TestTrue(TEXT("Limit zero still returns the latest, not the selected stored echo"),
		         StillLatest[0].Id == RecA.Id);
	}

	GameInstance->MarkAsGarbage();
	Sub->MarkAsGarbage();
	return true;
}

// With the ability unlocked, only the selected stored echo replays; the rolling latest is never used.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverSpecificSingle,
                                 "ReEcho.Run.EchoReplayResolver.SpecificSingle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverSpecificSingle::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Sub = CreateReplayStartedRun(GameInstance);

	const FReEchoRecording RecA = MakeCompletedRecording(Sub, FName(TEXT("Map_A")), 1);
	const FReEchoRecording RecB = MakeCompletedRecording(Sub, FName(TEXT("Map_B")), 2);
	Sub->StagePendingRecording(RecA);
	Sub->StorePendingRecording();
	Sub->StagePendingRecording(RecB);
	Sub->StorePendingRecording();

	// Latest is now RecB; selecting RecA proves the resolver returns the selection, not the latest.
	FReEchoRecording Latest;
	TestTrue(TEXT("Latest completed recording is RecB"),
	         Sub->TryGetLatestCompletedRecording(Latest) && Latest.Id == RecB.Id);

	(void)Sub->SetSpecificReplayLimit(1);
	(void)Sub->SetSelectedReplayIds({RecA.Id});

	const TArray<FReEchoRecording> Resolved = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Limit one resolves exactly one echo"), Resolved.Num(), 1);
	if (Resolved.Num() == 1)
	{
		TestTrue(TEXT("Resolved echo is the selected RecA, not the rolling latest RecB"), Resolved[0].Id == RecA.Id);
	}

	GameInstance->MarkAsGarbage();
	Sub->MarkAsGarbage();
	return true;
}

// A larger limit resolves every valid selected GUID once, in stable selection order, up to the limit.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverSpecificMulti,
                                 "ReEcho.Run.EchoReplayResolver.SpecificMulti",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverSpecificMulti::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Sub = CreateReplayStartedRun(GameInstance);

	const FReEchoRecording RecA = MakeCompletedRecording(Sub, FName(TEXT("Map_A")), 1);
	const FReEchoRecording RecB = MakeCompletedRecording(Sub, FName(TEXT("Map_B")), 2);
	const FReEchoRecording RecC = MakeCompletedRecording(Sub, FName(TEXT("Map_C")), 3);
	Sub->StagePendingRecording(RecA);
	Sub->StorePendingRecording();
	Sub->StagePendingRecording(RecB);
	Sub->StorePendingRecording();
	Sub->StagePendingRecording(RecC);
	Sub->StorePendingRecording();

	// Limit two with exactly two selected echoes -> two echoes, in stable selection order.
	(void)Sub->SetSpecificReplayLimit(2);
	(void)Sub->SetSelectedReplayIds({RecC.Id, RecB.Id});
	TArray<FReEchoRecording> Resolved = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Limit two resolves two selected echoes"), Resolved.Num(), 2);
	if (Resolved.Num() == 2)
	{
		TestTrue(TEXT("First resolved echo follows selection order (RecC)"), Resolved[0].Id == RecC.Id);
		TestTrue(TEXT("Second resolved echo follows selection order (RecB)"), Resolved[1].Id == RecB.Id);
	}

	// Full limit returns every selected GUID once, in selection order, not storage order.
	(void)Sub->SetSpecificReplayLimit(3);
	(void)Sub->SetSelectedReplayIds({RecC.Id, RecB.Id, RecA.Id});
	Resolved = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Limit three resolves all three selected echoes"), Resolved.Num(), 3);
	if (Resolved.Num() == 3)
	{
		TestTrue(TEXT("Stable order preserved (RecC)"), Resolved[0].Id == RecC.Id);
		TestTrue(TEXT("Stable order preserved (RecB)"), Resolved[1].Id == RecB.Id);
		TestTrue(TEXT("Stable order preserved (RecA)"), Resolved[2].Id == RecA.Id);
	}

	// RequestedCount clamps the result without touching storage or selection.
	const int32 BeforeStored = Sub->GetEchoStorageSummary().StoredEchoes.Num();
	const int32 BeforeSelected = Sub->GetSelectedReplayIds().Num();
	const TArray<FReEchoRecording> Clipped = Sub->ResolveReplayRecordings(1);
	TestEqual(TEXT("RequestedCount clamps to one"), Clipped.Num(), 1);
	TestEqual(
	    TEXT("Resolver did not mutate StoredEchoes"), Sub->GetEchoStorageSummary().StoredEchoes.Num(), BeforeStored);
	TestEqual(TEXT("Resolver did not mutate selection"), Sub->GetSelectedReplayIds().Num(), BeforeSelected);

	// Reducing the limit clamps the existing selection to the new limit.
	(void)Sub->SetSpecificReplayLimit(2);
	Resolved = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Limit reduction clamps selection to two echoes"), Resolved.Num(), 2);
	if (Resolved.Num() == 2)
	{
		TestTrue(TEXT("Clamped first echo follows selection order (RecC)"), Resolved[0].Id == RecC.Id);
		TestTrue(TEXT("Clamped second echo follows selection order (RecB)"), Resolved[1].Id == RecB.Id);
	}

	// Requesting more than the limit is rejected atomically; the valid selection is unchanged.
	(void)Sub->SetSpecificReplayLimit(1);
	const EReEchoEchoStorageResult Overflow = Sub->SetSelectedReplayIds({RecA.Id, RecB.Id});
	TestEqual(
	    TEXT("Selecting more than the limit is rejected"), Overflow, EReEchoEchoStorageResult::ReplayLimitExceeded);

	GameInstance->MarkAsGarbage();
	Sub->MarkAsGarbage();
	return true;
}

// An unlocked ability with an empty or stale selection resolves to zero echoes; it never falls back to latest.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverEmptyStale,
                                 "ReEcho.Run.EchoReplayResolver.EmptyStale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverEmptyStale::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Sub = CreateReplayStartedRun(GameInstance);

	const FReEchoRecording RecA = MakeCompletedRecording(Sub, FName(TEXT("Map_A")), 1);
	const FReEchoRecording RecB = MakeCompletedRecording(Sub, FName(TEXT("Map_B")), 2);
	Sub->StagePendingRecording(RecA);
	Sub->StorePendingRecording();
	Sub->StagePendingRecording(RecB);
	Sub->StorePendingRecording();

	// Empty selection -> zero echoes (no silent latest fallback). This is the Plan31 bug fix.
	(void)Sub->SetSpecificReplayLimit(1);
	(void)Sub->SetSelectedReplayIds({});
	const TArray<FReEchoRecording> EmptyResolved = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Empty selection resolves to zero echoes (no latest fallback)"), EmptyResolved.Num(), 0);

	// An invalid/stale GUID is rejected atomically; the previously valid selection is preserved.
	(void)Sub->SetSpecificReplayLimit(2);
	(void)Sub->SetSelectedReplayIds({RecA.Id});
	const FGuid StaleId = FGuid::NewGuid();
	const EReEchoEchoStorageResult StaleResult = Sub->SetSelectedReplayIds({RecA.Id, StaleId});
	TestEqual(TEXT("Selection containing a stale GUID is rejected"),
	          StaleResult,
	          EReEchoEchoStorageResult::InvalidRecordingId);
	const TArray<FReEchoRecording> StillOne = Sub->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Valid selection preserved after a rejected stale request"), StillOne.Num(), 1);
	if (StillOne.Num() == 1)
	{
		TestTrue(TEXT("Preserved echo is RecA"), StillOne[0].Id == RecA.Id);
	}

	GameInstance->MarkAsGarbage();
	Sub->MarkAsGarbage();
	return true;
}

// The selected replay set must survive a save/resume round trip and still resolve identically.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoReplayResolverSaveResume,
                                 "ReEcho.Run.EchoReplayResolver.SaveResume",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoReplayResolverSaveResume::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Sub = CreateReplayStartedRun(GameInstance);

	const FReEchoRecording RecA = MakeCompletedRecording(Sub, FName(TEXT("Map_A")), 1);
	const FReEchoRecording RecB = MakeCompletedRecording(Sub, FName(TEXT("Map_B")), 2);
	Sub->StagePendingRecording(RecA);
	Sub->StorePendingRecording();
	Sub->StagePendingRecording(RecB);
	Sub->StorePendingRecording();

	(void)Sub->SetSpecificReplayLimit(2);
	(void)Sub->SetSelectedReplayIds({RecB.Id, RecA.Id});

	UReEchoRunSaveGame* Snapshot = Sub->CreateSaveSnapshot();
	TestNotNull(TEXT("Save snapshot created"), Snapshot);

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = CreateReplayStartedRun(RestoredGameInstance);
	TestTrue(TEXT("Save restores into a fresh subsystem"), Restored->RestoreSaveSnapshot(*Snapshot));

	const TArray<FReEchoRecording> Resolved = Restored->ResolveReplayRecordings(ReEchoEchoStorage::MaxStorageCapacity);
	TestEqual(TEXT("Restored selection resolves two echoes"), Resolved.Num(), 2);
	if (Resolved.Num() == 2)
	{
		TestTrue(TEXT("Restored first echo follows saved selection (RecB)"), Resolved[0].Id == RecB.Id);
		TestTrue(TEXT("Restored second echo follows saved selection (RecA)"), Resolved[1].Id == RecA.Id);
	}

	GameInstance->MarkAsGarbage();
	Sub->MarkAsGarbage();
	RestoredGameInstance->MarkAsGarbage();
	Restored->MarkAsGarbage();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
