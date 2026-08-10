#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoElementReaction.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
FString AssembleElementCsvFixture(const TCHAR* FixtureName)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionTest,
                                 "ReEcho.Combat.ElementReactions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads for element reactions"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	TestTrue(TEXT("Lightning is now a combat element"),
	         ReEchoElementReaction::IsCombatElement(EReEchoElement::Lightning));

	FReEchoElementState State;
	FReEchoElementHitResult Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f);
	TestFalse(TEXT("Trigger-only flame hit does not attach by itself"), Result.bTriggeredReaction);
	TestEqual(TEXT("Trigger element does not become attachment"), State.Attached, EReEchoElement::None);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestFalse(TEXT("First grass hit only attaches"), Result.bTriggeredReaction);
	TestEqual(TEXT("Grass becomes attached"), State.Attached, EReEchoElement::Grass);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f, 1.0f, 10.0f);
	TestTrue(TEXT("Flame over grass triggers burn"), Result.bTriggeredReaction);
	TestEqual(TEXT("Burn reaction id comes from CSV"), Result.ReactionId, FName(TEXT("Y_ER_F_G")));
	TestEqual(TEXT("Burn uses table damage scale"), Result.Damage, 10.0f);
	TestEqual(TEXT("Burn status is applied"), Result.AppliedStatusId, FName(TEXT("Z_Burn")));
	TestEqual(TEXT("Reaction consumes attachment"), State.Attached, EReEchoElement::None);
	TestTrue(TEXT("Reaction starts elemental immunity"), State.ImmunityUntil > 10.0f);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 8.0f, 1.0f, 11.0f);
	TestTrue(TEXT("Elemental immunity blocks immediate attachment"), Result.bBlockedByImmunity);
	TestEqual(TEXT("Blocked hit keeps attachment clear"), State.Attached, EReEchoElement::None);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 40.0f);
	TestEqual(TEXT("Vaporize reaction id is ordered flame over water"), Result.ReactionId, FName(TEXT("Y_ER_F_W")));
	TestEqual(TEXT("Vaporize applies typed damage increase"), Result.Damage, 50.0f);
	TestEqual(TEXT("Vaporize radius comes from CSV"), Result.RadiusCm, 150.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 9.0f);
	TestEqual(TEXT("Growth reaction id is lightning over grass"), Result.ReactionId, FName(TEXT("Y_ER_L_G")));
	TestEqual(TEXT("Growth radius comes from CSV"), Result.RadiusCm, 200.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 12.0f, 1.05f);
	TestEqual(TEXT("Conduct reaction id is lightning over water"), Result.ReactionId, FName(TEXT("Y_ER_L_W")));
	TestEqual(TEXT("Conduct scales by ReactionEfficiency"), Result.Damage, 25.2f);
	TestEqual(TEXT("Conduct radius comes from CSV"), Result.RadiusCm, 100.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestEqual(TEXT("Grass over water has its own ordered reaction id"), Result.ReactionId, FName(TEXT("Y_ER_G_W")));
	TestTrue(TEXT("Enhance primes next reaction"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Enhancement multiplier comes from CSV"), State.EnhancementMultiplier, 2.0f);

	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestTrue(TEXT("Second enhancement reaction does not stack beyond table multiplier"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Enhancement remains non-stacked"), State.EnhancementMultiplier, 2.0f);

	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 10.0f);
	TestEqual(TEXT("Water over grass has a distinct ordered reaction id"), Result.ReactionId, FName(TEXT("Y_ER_W_G")));

	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f);
	TestTrue(TEXT("Primed enhancement affects next damaging reaction"), Result.bAppliedEnhancement);
	TestEqual(TEXT("Enhanced burn consumes enhancement multiplier"), Result.Damage, 20.0f);
	TestFalse(TEXT("Enhancement clears after damaging reaction"), State.bEnhancedNextReaction);

	const FReEchoCsvLoadResult ChangedValueLoad =
	    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(AssembleElementCsvFixture(TEXT("ReactionValueChanged")));
	if (!TestTrue(TEXT("Reaction value changed fixture loads"), ChangedValueLoad.bSuccess))
	{
		AddError(ChangedValueLoad.FormatIssues());
		return false;
	}
	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 10.0f);
	TestEqual(TEXT("Reaction coefficient change is visible without recompiling C++"), Result.Damage, 30.0f);

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

#endif
