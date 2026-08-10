#include "Data/ReEchoCsvDataRegistry.h"

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FString GetCsvFixtureDirectory(const TCHAR* FixtureName)
{
	return FPaths::Combine(
	    FPaths::ProjectContentDir(), TEXT("Data"), TEXT("TestFixtures"), TEXT("CsvRuntime"), FixtureName);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCsvDefaultDataLoadsTest,
                                 "ReEcho.Data.CsvDefaultDataLoads",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCsvDefaultDataLoadsTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Result = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), Result.bSuccess))
	{
		AddError(Result.FormatIssues());
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Default load publishes a snapshot"), Snapshot.IsValid()))
	{
		return false;
	}

	const FReEchoRuntimeSmokeRow* Row = Snapshot->FindRuntimeSmokeRow(FName(TEXT("RUNTIME_SMOKE")));
	if (!TestTrue(TEXT("Runtime smoke row exists"), Row != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("CSV scalar comes from production data"), Row->TestScalar, 42.5f);
	TestEqual(TEXT("Percent is stored as decimal"), Row->TestPercent, 0.20f);
	TestEqual(TEXT("Distance unit is centimeters"), Row->DistanceCm, 250.0f);
	TestEqual(TEXT("Duration unit is seconds"), Row->DurationSeconds, 1.5f);
	TestEqual(TEXT("One-to-many child effects attach by foreign key"), Row->Effects.Num(), 1);

	const FReEchoCsvCharacterRow* Sage = Snapshot->FindCharacter(TEXT("J_01"));
	if (!TestTrue(TEXT("Workbook character alias resolves to canonical runtime id"), Sage != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Sage canonical id is preserved"), Sage->Id, FName(TEXT("J_SPADE")));
	TestEqual(TEXT("Sage base health comes from CSV"), Sage->BaseStats.HpMax, 15.0f);

	const TArray<FReEchoCsvCardRow> TraitCards = Snapshot->GetOfferableCards(TEXT("Trait"));
	TestEqual(TEXT("Current trait draw pool contains six cards"), TraitCards.Num(), 6);
	const FReEchoCsvCardRow* HealthCard = Snapshot->FindCard(TEXT("G_1_02"));
	if (!TestTrue(TEXT("Health card exists"), HealthCard != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Health card has max-health and recovery effects"), HealthCard->Effects.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCsvFixtureValueReloadsTest,
                                 "ReEcho.Data.CsvFixtureValueReloadsWithoutCompile",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCsvFixtureValueReloadsTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Result =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(GetCsvFixtureDirectory(TEXT("ValidAlt")));
	if (!TestTrue(TEXT("Alternate CSV fixture loads"), Result.bSuccess))
	{
		AddError(Result.FormatIssues());
		return false;
	}

	const FReEchoRuntimeSmokeRow* Row =
	    Result.Snapshot.IsValid() ? Result.Snapshot->FindRuntimeSmokeRow(FName(TEXT("RUNTIME_SMOKE"))) : nullptr;
	if (!TestTrue(TEXT("Alternate fixture row exists"), Row != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Changed CSV scalar is reflected by reload"), Row->TestScalar, 77.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCsvInvalidDataDoesNotPublishTest,
                                 "ReEcho.Data.CsvInvalidDataDoesNotPublishPartialSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCsvInvalidDataDoesNotPublishTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult ValidResult =
	    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(GetCsvFixtureDirectory(TEXT("ValidAlt")));
	if (!TestTrue(TEXT("Valid fixture publishes"), ValidResult.bSuccess))
	{
		AddError(ValidResult.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Before = FReEchoCsvDataRegistry::GetSnapshot();

	const FReEchoCsvLoadResult InvalidResult =
	    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(GetCsvFixtureDirectory(TEXT("UnknownReference")));
	TestFalse(TEXT("Invalid fixture is rejected"), InvalidResult.bSuccess);
	TestTrue(TEXT("Invalid fixture reports foreign-key field"),
	         InvalidResult.FormatIssues().Contains(TEXT("RuntimeRowId")));
	TestTrue(TEXT("Published snapshot remains the prior successful snapshot"),
	         FReEchoCsvDataRegistry::GetSnapshot() == Before);

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCsvInvalidFixturesFailClearlyTest,
                                 "ReEcho.Data.CsvInvalidFixturesFailClearly",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCsvInvalidFixturesFailClearlyTest::RunTest(const FString& Parameters)
{
	struct FExpectedFailure
	{
		const TCHAR* FixtureName = TEXT("");
		const TCHAR* ExpectedToken = TEXT("");
	};

	const TArray<FExpectedFailure> ExpectedFailures = {
	    {TEXT("DuplicateId"), TEXT("Duplicate id")},
	    {TEXT("MissingRequired"), TEXT("TestScalar")},
	    {TEXT("UnknownReference"), TEXT("RuntimeRowId")},
	    {TEXT("UnknownBehavior"), TEXT("BehaviorId")},
	    {TEXT("UnknownEffectKind"), TEXT("EffectKind")},
	    {TEXT("IllegalRange"), TEXT("TestPercent")},
	    {TEXT("DuplicateCardEffectOrder"), TEXT("CardId/Order")},
	    {TEXT("UnsupportedCardEffectTrigger"), TEXT("Trigger")},
	    {TEXT("UnknownCardEffectTarget"), TEXT("Target")},
	    {TEXT("InvalidCardEffectBehaviorPair"), TEXT("EffectKind/BehaviorId")},
	    {TEXT("UnsupportedVersion"), TEXT("SchemaVersion")}};

	for (const FExpectedFailure& ExpectedFailure : ExpectedFailures)
	{
		const FReEchoCsvLoadResult Result =
		    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(GetCsvFixtureDirectory(ExpectedFailure.FixtureName));
		TestFalse(FString::Printf(TEXT("%s fixture fails"), ExpectedFailure.FixtureName), Result.bSuccess);
		TestTrue(
		    FString::Printf(TEXT("%s fixture reports %s"), ExpectedFailure.FixtureName, ExpectedFailure.ExpectedToken),
		    Result.FormatIssues().Contains(ExpectedFailure.ExpectedToken));
	}
	return true;
}

#endif
