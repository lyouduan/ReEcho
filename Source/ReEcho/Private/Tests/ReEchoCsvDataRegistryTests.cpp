#include "Data/ReEchoCsvDataRegistry.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FString GetCsvFixtureDirectory(const TCHAR* FixtureName)
{
	const FString SourceDataDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
	const FString FixtureDirectory =
	    FPaths::Combine(SourceDataDirectory, TEXT("TestFixtures"), TEXT("CsvRuntime"), FixtureName);
	const FString AssembledDirectory =
	    FPaths::Combine(FPaths::ProjectSavedDir(),
	                    TEXT("Automation"),
	                    TEXT("CsvRuntime"),
	                    FString::Printf(TEXT("%s_%s"), FixtureName, *FGuid::NewGuid().ToString(EGuidFormats::Digits)));

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*AssembledDirectory, true);

	TArray<FString> ProductionCsvFiles;
	FileManager.FindFiles(ProductionCsvFiles, *FPaths::Combine(SourceDataDirectory, TEXT("*.csv")), true, false);
	for (const FString& FileName : ProductionCsvFiles)
	{
		FileManager.Copy(*FPaths::Combine(AssembledDirectory, FileName),
		                 *FPaths::Combine(SourceDataDirectory, FileName));
	}

	TArray<FString> OverrideCsvFiles;
	FileManager.FindFiles(OverrideCsvFiles, *FPaths::Combine(FixtureDirectory, TEXT("*.csv")), true, false);
	for (const FString& FileName : OverrideCsvFiles)
	{
		FileManager.Copy(*FPaths::Combine(AssembledDirectory, FileName), *FPaths::Combine(FixtureDirectory, FileName));
	}
	return AssembledDirectory;
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
	TestEqual(TEXT("Current staged trait draw pool contains 64 cards"), TraitCards.Num(), 64);
	const FReEchoCsvCardRow* HealthCard = Snapshot->FindCard(TEXT("G_1_02"));
	if (!TestTrue(TEXT("Health card exists"), HealthCard != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Health card has max-health and recovery effects"), HealthCard->Effects.Num(), 2);

	const FReEchoCsvElementRow* Lightning = Snapshot->FindElement(EReEchoElement::Lightning);
	if (!TestTrue(TEXT("Lightning element is registered from CSV"), Lightning != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Lightning is a trigger element"), Lightning->Role, EReEchoElementRole::Trigger);
	TestEqual(TEXT("Element table contains four rows"), Snapshot->Elements.Num(), 4);

	const FReEchoCsvStatusRow* Burn = Snapshot->FindStatus(TEXT("Z_Burn"));
	if (!TestTrue(TEXT("Burn status exists"), Burn != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Burn duration comes from status CSV"), Burn->DurationSeconds, 3.0f);
	TestEqual(TEXT("Status table keeps workbook status rows"), Snapshot->Statuses.Num(), 8);

	const FReEchoCsvReactionRow* Conduct = Snapshot->FindReaction(TEXT("Lightning"), TEXT("Water"));
	if (!TestTrue(TEXT("Ordered Lightning over Water reaction exists"), Conduct != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Conduct damage multiplier no longer carries the +2 formula term"), Conduct->DamageMultiplier, 1.0f);
	TestEqual(TEXT("Conduct damage increase comes from CSV"), Conduct->DamageIncrease, 2.0f);
	TestEqual(TEXT("Six workbook reactions are enabled"), Snapshot->Reactions.Num(), 6);

	// WS4 (Plan 68): the authoritative boss is M_SHEEP (blood-bar-depleted two-form).
	const FReEchoCsvEnemyRow* Boss = Snapshot->FindEnabledEnemy(TEXT("M_SHEEP"));
	if (!TestTrue(TEXT("Stable boss EnemyId resolves from CSV"), Boss != nullptr))
	{
		return false;
	}
	TestEqual(TEXT("Seven enemy definitions include compatibility and Kepler rows"), Snapshot->Enemies.Num(), 7);
	TestEqual(TEXT("Boss one-form health comes from enemy CSV"), Boss->MaxHealth, 1300.0f);
	TestEqual(TEXT("Boss owns five behaviors (melee + volley + spread + blink + beam)"), Boss->Abilities.Num(), 5);
	TestEqual(TEXT("Boss active rotation begins with the melee basic attack"),
	          Boss->Abilities[0].BehaviorId,
	          FName(TEXT("Boss.MeleeSweep")));
	TestEqual(TEXT("Boss ships two phases (one-form + blood-depleted two-form)"), Boss->BossPhases.Num(), 2);
	TestEqual(TEXT("Phase-two maximum health is the black-form ceiling"), Boss->BossPhases[1].PhaseMaxHealth, 650.0f);
	TestEqual(TEXT("Phase-two refill policy is RefillToMaximum"),
	          Boss->BossPhases[1].RefillHealthPolicy,
	          TEXT("RefillToMaximum"));
	TestNotNull(TEXT("Kepler slime resolves by stable id"), Snapshot->FindEnabledEnemy(TEXT("M_SLIME")));
	TestNotNull(TEXT("Kepler rabbit resolves by stable id"), Snapshot->FindEnabledEnemy(TEXT("M_RABBIT")));
	TestNotNull(TEXT("Kepler fox resolves by stable id"), Snapshot->FindEnabledEnemy(TEXT("M_FOX")));
	TestEqual(TEXT("Eight table-driven encounters load"), Snapshot->Encounters.Num(), 8);
	TestEqual(TEXT("Encounter one owns three waves"), Snapshot->GetEncounterWaves(TEXT("Encounter.1")).Num(), 3);
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
	    {TEXT("UnknownWeaponReference"), TEXT("DefaultWeaponId")},
	    {TEXT("UnknownBehavior"), TEXT("BehaviorId")},
	    {TEXT("UnknownEffectKind"), TEXT("EffectKind")},
	    {TEXT("IllegalRange"), TEXT("TestPercent")},
	    {TEXT("DuplicateCardEffectOrder"), TEXT("CardId/Order")},
	    {TEXT("UnsupportedCardEffectTrigger"), TEXT("Trigger")},
	    {TEXT("UnknownCardEffectTarget"), TEXT("Target")},
	    {TEXT("InvalidCardEffectBehaviorPair"), TEXT("EffectKind/BehaviorId")},
	    {TEXT("UnknownFormulaId"), TEXT("FormulaId")},
	    {TEXT("DuplicateReactionPair"), TEXT("ordered pair")},
	    {TEXT("UnsupportedCanCrit"), TEXT("CanCrit")},
	    {TEXT("DuplicateWeaponInputSlot"), TEXT("InputSlot")},
	    {TEXT("InvalidWeaponPartEffectBehaviorPair"), TEXT("EffectKind/BehaviorId")},
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
