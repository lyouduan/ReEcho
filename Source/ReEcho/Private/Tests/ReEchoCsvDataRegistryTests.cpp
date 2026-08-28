#include "Data/ReEchoCsvDataRegistry.h"

#include "Data/ReEchoDataAssets.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
constexpr const TCHAR* DefaultCatalogPath =
    TEXT("/Game/ReEcho/DataAsset/Gameplay/DA_ReEchoGameDataCatalog.DA_ReEchoGameDataCatalog");

const UReEchoGameDataCatalog* LoadDefaultCatalog()
{
	return LoadObject<UReEchoGameDataCatalog>(nullptr, DefaultCatalogPath);
}

UReEchoGameDataCatalog* DuplicateCatalog(const UReEchoGameDataCatalog& Source)
{
	UReEchoGameDataCatalog* Copy = DuplicateObject<UReEchoGameDataCatalog>(&Source, GetTransientPackage());
	Copy->Core = DuplicateObject<UReEchoCoreDataAsset>(Source.Core, Copy);
	Copy->Cards = DuplicateObject<UReEchoCardDataAsset>(Source.Cards, Copy);
	Copy->Elements = DuplicateObject<UReEchoElementDataAsset>(Source.Elements, Copy);
	Copy->Weapons = DuplicateObject<UReEchoWeaponDataAsset>(Source.Weapons, Copy);
	Copy->Enemies = DuplicateObject<UReEchoEnemyDataAsset>(Source.Enemies, Copy);
	Copy->Encounters = DuplicateObject<UReEchoEncounterDataAsset>(Source.Encounters, Copy);
	Copy->Shop = DuplicateObject<UReEchoShopDataAsset>(Source.Shop, Copy);
	return Copy;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDataAssetDefaultDataLoadsTest,
                                 "ReEcho.Data.Asset.DefaultDataLoads",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDataAssetDefaultDataLoadsTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Result = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default Blueprint data loads"), Result.bSuccess))
	{
		AddError(Result.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Default load publishes a snapshot"), Snapshot.IsValid()))
	{
		return false;
	}

	const FReEchoRuntimeSmokeRow* Smoke = Snapshot->FindRuntimeSmokeRow(TEXT("RUNTIME_SMOKE"));
	TestNotNull(TEXT("Runtime smoke row exists"), Smoke);
	if (Smoke)
	{
		TestEqual(TEXT("Scalar is preserved"), Smoke->TestScalar, 42.5f);
		TestEqual(TEXT("Percent is stored as decimal"), Smoke->TestPercent, 0.20f);
		TestEqual(TEXT("Child effects remain attached"), Smoke->Effects.Num(), 1);
	}
	const FReEchoCsvCharacterRow* Sage = Snapshot->FindCharacter(TEXT("J_01"));
	TestNotNull(TEXT("Character alias resolves"), Sage);
	if (Sage)
	{
		TestEqual(TEXT("Canonical character id is preserved"), Sage->Id, FName(TEXT("J_SPADE")));
		TestEqual(TEXT("Character health is preserved"), Sage->BaseStats.HpMax, 15.0f);
	}
	TestEqual(TEXT("Current trait draw pool contains 64 cards"), Snapshot->GetOfferableCards(TEXT("Trait")).Num(), 64);
	TestEqual(TEXT("Four elements load"), Snapshot->Elements.Num(), 4);
	TestEqual(TEXT("Nine statuses load"), Snapshot->Statuses.Num(), 9);
	TestEqual(TEXT("Six reactions load"), Snapshot->Reactions.Num(), 6);
	TestEqual(TEXT("Seven enemies load"), Snapshot->Enemies.Num(), 7);
	TestEqual(TEXT("Eight encounters load"), Snapshot->Encounters.Num(), 8);

	const FReEchoCsvReactionRow* Conduct = Snapshot->FindReaction(TEXT("Lightning"), TEXT("Water"));
	TestNotNull(TEXT("Conduct reaction exists"), Conduct);
	if (Conduct)
	{
		TestEqual(TEXT("Conduct multiplier is current"), Conduct->DamageMultiplier, 1.0f);
		TestEqual(TEXT("Conduct additive increase is current"), Conduct->DamageIncrease, 0.0f);
	}
	const FReEchoCsvEnemyRow* Boss = Snapshot->FindEnabledEnemy(TEXT("M_SHEEP"));
	TestNotNull(TEXT("Boss resolves by stable id"), Boss);
	if (Boss)
	{
		TestEqual(TEXT("Boss current first-form health"), Boss->MaxHealth, 1000.0f);
		TestEqual(TEXT("Boss has two phases"), Boss->BossPhases.Num(), 2);
		if (Boss->BossPhases.Num() > 1)
		{
			TestEqual(TEXT("Boss current second-form health"), Boss->BossPhases[1].PhaseMaxHealth, 500.0f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDataAssetValueReloadsTest,
                                 "ReEcho.Data.Asset.ValueReloadsWithoutCompile",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDataAssetValueReloadsTest::RunTest(const FString& Parameters)
{
	const UReEchoGameDataCatalog* Default = LoadDefaultCatalog();
	if (!TestNotNull(TEXT("Default catalog asset exists"), Default))
	{
		return false;
	}
	UReEchoGameDataCatalog* Copy = DuplicateCatalog(*Default);
	Copy->Core->RuntimeSmokeRows[0].TestScalar = 77.25f;
	const FReEchoCsvLoadResult Result = FReEchoCsvDataRegistry::LoadSnapshotFromCatalog(*Copy);
	if (!TestTrue(TEXT("Edited Blueprint asset compiles"), Result.bSuccess))
	{
		AddError(Result.FormatIssues());
		return false;
	}
	const FReEchoRuntimeSmokeRow* Row = Result.Snapshot->FindRuntimeSmokeRow(TEXT("RUNTIME_SMOKE"));
	TestNotNull(TEXT("Edited row exists"), Row);
	TestEqual(TEXT("Value edit is visible without a C++ rebuild"), Row ? Row->TestScalar : 0.0f, 77.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDataAssetInvalidDataDoesNotPublishTest,
                                 "ReEcho.Data.Asset.InvalidDataDoesNotPublish",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDataAssetInvalidDataDoesNotPublishTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Valid = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default asset publishes"), Valid.bSuccess))
	{
		AddError(Valid.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Before = FReEchoCsvDataRegistry::GetSnapshot();
	const UReEchoGameDataCatalog* Default = LoadDefaultCatalog();
	if (!TestNotNull(TEXT("Default catalog asset exists"), Default))
	{
		return false;
	}

	UReEchoGameDataCatalog* Copy = DuplicateCatalog(*Default);
	Copy->Core->Characters[0].DefaultWeaponId = TEXT("Missing.Weapon");
	const FReEchoCsvLoadResult Invalid = FReEchoCsvDataRegistry::LoadAndPublishFromCatalog(*Copy);
	TestFalse(TEXT("Unknown asset reference is rejected"), Invalid.bSuccess);
	TestTrue(TEXT("Failure identifies the edited field"), Invalid.FormatIssues().Contains(TEXT("DefaultWeaponId")));
	TestTrue(TEXT("Failed reload preserves the last good snapshot"), FReEchoCsvDataRegistry::GetSnapshot() == Before);

	Copy = DuplicateCatalog(*Default);
	const FReEchoCsvCardRow DuplicateCard = Copy->Cards->Cards[0];
	Copy->Cards->Cards.Add(DuplicateCard);
	const FReEchoCsvLoadResult Duplicate = FReEchoCsvDataRegistry::LoadSnapshotFromCatalog(*Copy);
	TestFalse(TEXT("Duplicate stable id is rejected"), Duplicate.bSuccess);
	TestTrue(TEXT("Duplicate failure is clear"), Duplicate.FormatIssues().Contains(TEXT("Duplicate id")));
	return true;
}

#endif
