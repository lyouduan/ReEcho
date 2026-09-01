#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDifficultyPackageTest,
                                 "ReEcho.Difficulty.PackagesAndCache",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDifficultyPackageTest::RunTest(const FString& Parameters)
{
	if (!FReEchoCsvDataRegistry::GetSnapshot().IsValid())
	{
		TestTrue(TEXT("Default CSV snapshot loads"), FReEchoCsvDataRegistry::LoadAndPublishDefault().bSuccess);
	}
	const FReEchoCsvLoadResult Party =
	    FReEchoCsvDataRegistry::LoadDifficultySnapshot(EReEchoRunDifficulty::Party);
	const FReEchoCsvLoadResult Standard =
	    FReEchoCsvDataRegistry::LoadDifficultySnapshot(EReEchoRunDifficulty::Standard);
	const FReEchoCsvLoadResult Nightmare =
	    FReEchoCsvDataRegistry::LoadDifficultySnapshot(EReEchoRunDifficulty::Nightmare);
	TestTrue(TEXT("Party package loads"), Party.bSuccess);
	TestTrue(TEXT("Standard package loads"), Standard.bSuccess);
	TestTrue(TEXT("Nightmare package loads"), Nightmare.bSuccess);
	if (!Party.Snapshot.IsValid() || !Standard.Snapshot.IsValid() || !Nightmare.Snapshot.IsValid())
	{
		return false;
	}
	TestEqual(TEXT("Initial enemy counts match"), Party.Snapshot->Enemies.Num(), Standard.Snapshot->Enemies.Num());
	TestEqual(TEXT("Initial encounter counts match"), Nightmare.Snapshot->Encounters.Num(), Standard.Snapshot->Encounters.Num());
	TestEqual(TEXT("Initial wave counts match"), Party.Snapshot->EncounterWaves.Num(), Nightmare.Snapshot->EncounterWaves.Num());
	TestTrue(TEXT("Difficulty snapshots are isolated"), Party.Snapshot.Get() != Nightmare.Snapshot.Get());
	const FReEchoCsvLoadResult CachedParty =
	    FReEchoCsvDataRegistry::LoadDifficultySnapshot(EReEchoRunDifficulty::Party);
	TestTrue(TEXT("Repeated difficulty request reuses cache"), CachedParty.Snapshot.Get() == Party.Snapshot.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDifficultyRunLockTest,
                                 "ReEcho.Difficulty.RunLockAndSaveMigration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDifficultyRunLockTest::RunTest(const FString& Parameters)
{
	if (!FReEchoCsvDataRegistry::GetSnapshot().IsValid())
	{
		TestTrue(TEXT("Default CSV snapshot loads"), FReEchoCsvDataRegistry::LoadAndPublishDefault().bSuccess);
	}
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>();
	TestTrue(TEXT("Fresh run can edit next-run difficulty"), Source->CanEditDifficultyPreference());
	TestTrue(TEXT("Standard run starts"), Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01")));
	TestFalse(TEXT("Started run locks difficulty preference"), Source->CanEditDifficultyPreference());
	TestEqual(TEXT("Default locked difficulty is Standard"),
	          Source->GetRunDifficulty(),
	          EReEchoRunDifficulty::Standard);
	Source->PrepareForStartMenu();
	TestTrue(TEXT("Returning to the start menu releases only the in-memory difficulty lock"),
	         Source->CanEditDifficultyPreference());
	TestEqual(TEXT("Start menu returns to character selection"), Source->Phase, EReEchoRunPhase::CharacterSelect);

	UReEchoRunSaveGame* PartySave = Source->CreateSaveSnapshot();
	PartySave->Difficulty = EReEchoRunDifficulty::Party;
	UReEchoRunSubsystem* PartyRestore = NewObject<UReEchoRunSubsystem>();
	TestTrue(TEXT("Party save restores"), PartyRestore->RestoreSaveSnapshot(*PartySave));
	TestEqual(TEXT("Continue locks saved Party difficulty"),
	          PartyRestore->GetRunDifficulty(),
	          EReEchoRunDifficulty::Party);
	TestEqual(TEXT("Restored snapshot is Party"),
	          PartyRestore->GetRunDataSnapshot()->Difficulty,
	          EReEchoRunDifficulty::Party);

	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 26;
	LegacySave->Difficulty = EReEchoRunDifficulty::Nightmare;
	UReEchoRunSubsystem* LegacyRestore = NewObject<UReEchoRunSubsystem>();
	TestTrue(TEXT("v26 save migrates"), LegacyRestore->RestoreSaveSnapshot(*LegacySave));
	TestEqual(TEXT("v26 save deterministically migrates to Standard"),
	          LegacyRestore->GetRunDifficulty(),
	          EReEchoRunDifficulty::Standard);

	UReEchoRunSaveGame* InvalidSave = Source->CreateSaveSnapshot();
	InvalidSave->Difficulty = static_cast<EReEchoRunDifficulty>(255);
	UReEchoRunSubsystem* InvalidRestore = NewObject<UReEchoRunSubsystem>();
	TestFalse(TEXT("Unknown saved difficulty is rejected"), InvalidRestore->RestoreSaveSnapshot(*InvalidSave));
	return true;
}

#endif
