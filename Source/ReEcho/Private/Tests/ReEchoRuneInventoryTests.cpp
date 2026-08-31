#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Run/ReEchoRuneInventory.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

namespace
{
const FName RuneI(TEXT("P_GUN_EXPLOSIVE_MUZZLE_I"));
const FName RuneII(TEXT("P_GUN_EXPLOSIVE_MUZZLE_II"));
const FName RuneIII(TEXT("P_GUN_EXPLOSIVE_MUZZLE_III"));
const FName Core(TEXT("P_CORE_FLAME"));

UReEchoRunSubsystem* NewRuneRun()
{
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(NewObject<UGameInstance>());
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_09"));
	Run->TimeShards = 10000;
	return Run;
}

bool HasEquipped(const UReEchoRunSubsystem* Run, const FName Id)
{
	return Run->CurrentBuild.EquippedParts.ContainsByPredicate(
	    [Id](const FReEchoEquippedPartSnapshot& Part)
	    {
		    return Part.PartId == Id;
	    });
}

// Use the public save/restore boundary to build exact pages. Production random streams stay untouched.
bool SetRunePage(UReEchoRunSubsystem* Run, const TArray<FName>& Ids)
{
	Run->GetWeaponPartShopView();
	UReEchoRunSaveGame* Save = Run->CreateSaveSnapshot();
	Save->WeaponPartShopOfferIds = Ids;
	Save->WeaponPartShopOfferIds.SetNum(3);
	Save->PurchasedWeaponPartOfferIds.Reset();
	return Run->RestoreSaveSnapshot(*Save);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneArithmeticTest,
                                 "ReEcho.Shop.Rune.SynthesisArithmetic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneArithmeticTest::RunTest(const FString&)
{
	const auto Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Production rune data is loaded"), Snapshot.IsValid()))
	{
		return false;
	}
	using namespace ReEchoRuneInventory;
	FString Error;
	int32 Families = 0;
	for (const auto& Pair : Snapshot->RuneUpgrades)
	{
		if (!Pair.Key.ToString().EndsWith(TEXT("_I")))
		{
			continue;
		}
		++Families;
		const FName I = Pair.Key;
		const FName II = Pair.Value.ToPartId;
		const auto* Next = Snapshot->RuneUpgrades.Find(II);
		if (!TestNotNull(TEXT("Each I has II->III recipe"), Next))
		{
			return false;
		}
		const FName III = Next->ToPartId;
		FState Input, Output;
		Input.Counts.Add(I, 1);
		TestTrue(TEXT("One purchased I settles"), TrySettle(*Snapshot, Input, I, Output, Error));
		TestEqual(TEXT("One I stays in backpack"), Output.Counts.FindRef(I), 1);
		TestTrue(TEXT("Empty equipment stays empty"), Output.EquippedIds.IsEmpty());
		Input.Counts[I] = 2;
		TestTrue(TEXT("Two backpack I settle"), TrySettle(*Snapshot, Input, I, Output, Error));
		TestEqual(TEXT("Two I become one II"), Output.Counts.FindRef(II), 1);
		TestFalse(TEXT("Consumed I removed"), Output.Counts.Contains(I));
		Input.Counts[I] = 4;
		TestTrue(TEXT("Four I settle through both recipes"), TrySettle(*Snapshot, Input, I, Output, Error));
		TestEqual(TEXT("Four I become III in backpack"), Output.Counts.FindRef(III), 1);
		TestTrue(TEXT("Backpack III is not automatically equipped"), Output.EquippedIds.IsEmpty());
		TestTrue(TEXT("Terminal tier gates I"), OwnsTerminalTier(*Snapshot, I, {III}));
		TestFalse(TEXT("II does not gate I"), OwnsTerminalTier(*Snapshot, I, {II}));
		Input.Counts.Reset();
		Input.Counts.Add(I, 2);
		Input.Counts.Add(Core, 1);
		Input.EquippedIds = {Core, I};
		TestTrue(TEXT("Direct equipped fusion settles"), TrySettle(*Snapshot, Input, I, Output, Error));
		TestTrue(TEXT("Original ordered slot retained"), Output.EquippedIds == TArray<FName>({Core, II}));
		Input.Counts.Remove(I);
		Input.Counts.Add(II, 1);
		Input.Counts.Add(I, 2);
		Input.EquippedIds = {Core, II};
		TestTrue(TEXT("Backpack II cascades into equipped II"), TrySettle(*Snapshot, Input, I, Output, Error));
		TestTrue(TEXT("Confirmed user example equips III"), Output.EquippedIds == TArray<FName>({Core, III}));
		TestEqual(TEXT("Only core and III survive"), Output.Counts.Num(), 2);
		FState Repeated;
		TestTrue(TEXT("Already settled state can settle again"),
		         TrySettle(*Snapshot, Output, NAME_None, Repeated, Error));
		TestTrue(TEXT("Settlement is idempotent"),
		         Repeated.Counts.OrderIndependentCompareEqual(Output.Counts) &&
		             Repeated.EquippedIds == Output.EquippedIds);
		Input.Counts = {{I, 1}};
		Input.EquippedIds = {I};
		TestTrue(TEXT("Equipped copy is not a second backpack copy"),
		         TrySettle(*Snapshot, Input, NAME_None, Output, Error));
		TestEqual(TEXT("Single equipped copy stays tier I"), Output.EquippedIds[0], I);
	}
	TestEqual(TEXT("All active tiered families are covered"), Families, 31);
	FState Gems, Result;
	Gems.Counts.Add(Core, 1);
	TestTrue(TEXT("Core settles without recipe"), TrySettle(*Snapshot, Gems, Core, Result, Error));
	TestTrue(TEXT("Core remains in backpack"), Result.EquippedIds.IsEmpty());
	FReEchoCsvDataSnapshot Bad = *Snapshot;
	Bad.RuneUpgrades.FindChecked(RuneI).NeedCount = 1;
	Result = Gems;
	TestFalse(TEXT("Malformed recipe rejected"), TrySettle(Bad, Gems, NAME_None, Result, Error));
	TestTrue(TEXT("Rejected candidate leaves output untouched"),
	         Result.Counts.OrderIndependentCompareEqual(Gems.Counts));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRunePurchaseTest,
                                 "ReEcho.Shop.Rune.PurchaseAndEquippedCascade",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRunePurchaseTest::RunTest(const FString&)
{
	UReEchoRunSubsystem* Run = NewRuneRun();
	FString Error;
	TestTrue(TEXT("Exact purchase page restores"), SetRunePage(Run, {Core, RuneI}));
	TestTrue(TEXT("First core purchase succeeds"), Run->PurchaseShopItem(Core));
	TestFalse(TEXT("Purchased core does not auto-equip"), HasEquipped(Run, Core));
	TestTrue(TEXT("First rune purchase succeeds"), Run->PurchaseShopItem(RuneI));
	TestFalse(TEXT("Purchased I does not auto-equip"), HasEquipped(Run, RuneI));
	TestEqual(TEXT("First I has one copy"), Run->RuneAcquisitionCounts.FindRef(RuneI), 1);
	TestTrue(TEXT("Core equips through backpack"), Run->TryEquipPurchasedPart(Core, Error));
	TestTrue(TEXT("I equips through backpack"), Run->TryEquipPurchasedPart(RuneI, Error));
	TestTrue(TEXT("Next page restores"), SetRunePage(Run, {RuneI}));
	const int32 Before = Run->TimeShards;
	const int32 Price = Run->GetWeaponPartShopView().SlotOffers[0].EffectivePrice;
	TestTrue(TEXT("Second I purchase succeeds"), Run->PurchaseShopItem(RuneI));
	TestEqual(TEXT("Charges once"), Run->TimeShards, Before - Price);
	TestTrue(TEXT("Second I upgrades equipped I to II"), HasEquipped(Run, RuneII));
	TestTrue(TEXT("Unrelated core remains equipped"), HasEquipped(Run, Core));
	TestFalse(TEXT("Consumed I has no stale ownership"), Run->OwnedPartIds.Contains(RuneI));
	TestTrue(TEXT("Third page restores"), SetRunePage(Run, {RuneI}));
	TestTrue(TEXT("Third I purchase succeeds"), Run->PurchaseShopItem(RuneI));
	TestTrue(TEXT("Different level does not displace equipped II"), HasEquipped(Run, RuneII));
	TestEqual(TEXT("Third I remains in backpack"), Run->RuneAcquisitionCounts.FindRef(RuneI), 1);
	TestTrue(TEXT("Fourth page restores"), SetRunePage(Run, {RuneI}));
	TestTrue(TEXT("Fourth I purchase succeeds"), Run->PurchaseShopItem(RuneI));
	TestTrue(TEXT("Backpack II and equipped II cascade to III"), HasEquipped(Run, RuneIII));
	TestFalse(TEXT("No I remains after cascade"), Run->RuneAcquisitionCounts.Contains(RuneI));
	TestFalse(TEXT("No II remains after cascade"), Run->RuneAcquisitionCounts.Contains(RuneII));
	TestEqual(TEXT("Exactly one III remains"), Run->RuneAcquisitionCounts.FindRef(RuneIII), 1);
	TestTrue(TEXT("Original slot order survives cascade"),
	         Run->CurrentBuild.EquippedParts[0].PartId == Core && Run->CurrentBuild.EquippedParts[1].PartId == RuneIII);
	const FName BowRune(TEXT("P_BOW_SPLIT_ARROWHEAD_I"));
	TestTrue(TEXT("Foreign rune page restores"), SetRunePage(Run, {BowRune}));
	TestTrue(TEXT("Foreign weapon rune is purchasable"), Run->PurchaseShopItem(BowRune));
	TestTrue(TEXT("Foreign rune is owned"), Run->OwnedPartIds.Contains(BowRune));
	TestFalse(TEXT("Foreign rune never auto-equips"), HasEquipped(Run, BowRune));
	TestFalse(TEXT("Foreign rune stays out of current weapon backpack view"),
	          Run->GetWeaponPartShopView().OwnedParts.ContainsByPredicate(
	              [&](const FReEchoShopOffer& Offer)
	              {
		              return Offer.ContentId == BowRune;
	              }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneSaveTest,
                                 "ReEcho.Shop.Rune.SavePageAndNewRun",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneSaveTest::RunTest(const FString&)
{
	UReEchoRunSubsystem* Source = NewRuneRun();
	TestTrue(TEXT("Fixture page restores"), SetRunePage(Source, {RuneI, Core}));
	const UReEchoRunSaveGame* BeforePurchase = Source->CreateSaveSnapshot();
	TestTrue(TEXT("Purchase consumes only one offer"), Source->PurchaseShopItem(RuneI));
	TArray<uint8> Bytes;
	TestTrue(TEXT("Snapshot serializes to bytes"),
	         UGameplayStatics::SaveGameToMemory(Source->CreateSaveSnapshot(), Bytes));
	const UReEchoRunSaveGame* Saved = Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Snapshot deserializes"), Saved))
	{
		return false;
	}
	UReEchoRunSubsystem* Restored = NewRuneRun();
	TestTrue(TEXT("Fresh instance restores purchased page"), Restored->RestoreSaveSnapshot(*Saved));
	TestEqual(TEXT("Saved I count restores"), Restored->RuneAcquisitionCounts.FindRef(RuneI), 1);
	TestTrue(TEXT("Consumed slot stays empty after real serialization"),
	         Restored->GetWeaponPartShopView().SlotOffers[0].ItemId.IsNone());
	TestEqual(TEXT("Unpurchased neighboring slot stays stable"),
	          Restored->GetWeaponPartShopView().SlotOffers[1].ItemId,
	          Core);
	const int32 Cash = Restored->TimeShards;
	TestFalse(TEXT("Purchased page cannot charge again"), Restored->PurchaseShopItem(RuneI));
	TestEqual(TEXT("Rejected duplicate preserves cash"), Restored->TimeShards, Cash);
	TestTrue(TEXT("Earlier save restores into same live instance"), Restored->RestoreSaveSnapshot(*BeforePurchase));
	TestFalse(TEXT("Same-instance restore clears later purchase marks"),
	          Restored->PurchasedWeaponPartOfferIds.Contains(RuneI));
	TestTrue(TEXT("Earlier unconsumed offer can be purchased"), Restored->PurchaseShopItem(RuneI));
	Restored->StartRun(TEXT("J_SPADE"), TEXT("W_J_09"));
	TestTrue(TEXT("New run clears counts"), Restored->RuneAcquisitionCounts.IsEmpty());
	TestTrue(TEXT("New run clears page marks"), Restored->PurchasedWeaponPartOfferIds.IsEmpty());
	Restored->TimeShards = 10000;
	TestTrue(TEXT("New run exact page restores"), SetRunePage(Restored, {RuneI}));
	TestTrue(TEXT("New run first purchase succeeds"), Restored->PurchaseShopItem(RuneI));
	TestEqual(TEXT("New run does not inherit phantom copies"), Restored->RuneAcquisitionCounts.FindRef(RuneI), 1);
	TestFalse(TEXT("New run does not wrongly synthesize II"), Restored->OwnedPartIds.Contains(RuneII));
	UReEchoRunSaveGame* Legacy = Source->CreateSaveSnapshot();
	Legacy->SaveVersion = 25;
	Legacy->RuneAcquisitionCounts.Reset();
	Legacy->RuneAcquisitionCounts.Add(TEXT("P_BOW_SPLIT_ARROWHEAD_I"), 99);
	TestTrue(TEXT("Legacy save migrates"), Restored->RestoreSaveSnapshot(*Legacy));
	TestEqual(TEXT("Legacy ownership creates minimum one copy"), Restored->RuneAcquisitionCounts.FindRef(RuneI), 1);
	TestFalse(TEXT("Unproven orphan count is discarded"),
	          Restored->RuneAcquisitionCounts.Contains(TEXT("P_BOW_SPLIT_ARROWHEAD_I")));
	TestTrue(TEXT("Legacy held-family offer conservatively consumed"),
	         Restored->GetWeaponPartShopView().SlotOffers[0].ItemId.IsNone());
	TestEqual(
	    TEXT("Legacy unrelated offer not rerolled"), Restored->GetWeaponPartShopView().SlotOffers[1].ItemId, Core);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneTerminalTest,
                                 "ReEcho.Shop.Rune.TerminalCacheAndAtomicFailure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneTerminalTest::RunTest(const FString&)
{
	UReEchoRunSubsystem* Run = NewRuneRun();
	Run->OwnedPartIds.Add(RuneIII);
	Run->RuneAcquisitionCounts.Add(RuneIII, 1);
	TestTrue(TEXT("Restore stale I beside unpurchased core"), SetRunePage(Run, {RuneI, Core}));
	const auto Page = Run->GetWeaponPartShopView();
	TestTrue(TEXT("Held III suppresses cached I"), Page.SlotOffers[0].ItemId.IsNone());
	TestEqual(TEXT("III filtering leaves neighboring quote stable"), Page.SlotOffers[1].ItemId, Core);
	const int32 Cash = Run->TimeShards;
	TestFalse(TEXT("Stale I purchase cannot bypass III gate"), Run->PurchaseShopItem(RuneI));
	TestEqual(TEXT("Terminal rejection does not charge"), Run->TimeShards, Cash);
	TestEqual(
	    TEXT("Projection preserves original cached IDs"), Run->CreateSaveSnapshot()->WeaponPartShopOfferIds[0], RuneI);
	for (int32 Encounter = 1; Encounter < 16; ++Encounter)
	{
		Run->EncounterIndex = Encounter;
		TestFalse(TEXT("New rolled pages exclude terminal family"),
		          Run->GetWeaponPartShopView().SlotOffers.ContainsByPredicate(
		              [](const FReEchoWeaponSlotOffer& Offer)
		              {
			              return Offer.PartId == RuneI;
		              }));
	}
	// Invalid inventory is rejected by the candidate planner after normal pricing/mutation evaluation.
	TestTrue(TEXT("Valid sale page restored for rollback"), SetRunePage(Run, {Core}));
	Run->OwnedPartIds.Add(TEXT("P_INVALID_FIXTURE"));
	Run->RuneAcquisitionCounts.Add(TEXT("P_INVALID_FIXTURE"), 1);
	const auto OwnedBefore = Run->OwnedPartIds;
	const auto CountsBefore = Run->RuneAcquisitionCounts;
	const float HpBefore = Run->CurrentBuild.Stats.HpMax;
	const auto Failed = Run->PurchaseShopItemDetailed(Core);
	TestEqual(TEXT("Invalid candidate reports mutation rejection"),
	          Failed.Result,
	          EReEchoShopPurchaseResult::MutationRejected);
	TestEqual(TEXT("Failure preserves shards"), Run->TimeShards, Cash);
	TestTrue(TEXT("Failure preserves exact ownership"), Run->OwnedPartIds == OwnedBefore);
	TestTrue(TEXT("Failure preserves exact counts"),
	         Run->RuneAcquisitionCounts.OrderIndependentCompareEqual(CountsBefore));
	TestEqual(TEXT("Failure preserves stats"), Run->CurrentBuild.Stats.HpMax, HpBefore);
	TestFalse(TEXT("Failure does not mark offer purchased"), Run->PurchasedWeaponPartOfferIds.Contains(Core));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneEquipSettlementTest,
                                 "ReEcho.Shop.Rune.ManualAndWeaponSettlement",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneEquipSettlementTest::RunTest(const FString&)
{
	UReEchoRunSubsystem* Run = NewRuneRun();
	FString Error;
	TestTrue(TEXT("Initial equipped II"), Run->TryEquipParts({Core, RuneII}, Error));
	Run->OwnedPartIds.Add(RuneI);
	Run->RuneAcquisitionCounts.Add(RuneI, 2);
	TestTrue(TEXT("Manual loadout invokes backpack cascade"), Run->TryEquipParts({Core, RuneII}, Error));
	TestTrue(TEXT("Manual equip settles into III"), HasEquipped(Run, RuneIII));
	TestTrue(TEXT("Unequip command succeeds"), Run->TryEquipParts({Core}, Error));
	const auto View = Run->GetWeaponPartShopView();
	const auto* BagIII = View.OwnedParts.FindByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.ContentId == RuneIII;
	    });
	TestNotNull(TEXT("Unequipped III projected in backpack"), BagIII);
	if (BagIII)
	{
		TestEqual(TEXT("Backpack reports actual copy count"), BagIII->BackpackCount, 1);
	}
	Run = NewRuneRun();
	TestTrue(TEXT("Initial gun I equipped"), Run->TryEquipParts({Core, RuneI}, Error));
	Run->RuneAcquisitionCounts[RuneI] = 2;
	Run->OwnedWeaponIds.Add(TEXT("W_J_08"));
	TestTrue(TEXT("Weapon switch settles removed gun runes"), Run->TryEquipOwnedWeapon(TEXT("W_J_08"), Error));
	TestTrue(TEXT("Core survives weapon switch"), HasEquipped(Run, Core));
	TestFalse(TEXT("Foreign gun rune removed from equipment"), HasEquipped(Run, RuneI));
	TestEqual(TEXT("Removed I plus bag I become bag II"), Run->RuneAcquisitionCounts.FindRef(RuneII), 1);
	TestTrue(TEXT("Switch back succeeds"), Run->TryEquipOwnedWeapon(TEXT("W_J_09"), Error));
	TestFalse(TEXT("Switching back does not auto-equip bag II"), HasEquipped(Run, RuneII));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneDualSlotTest,
                                 "ReEcho.Shop.Rune.DualSlotAndReplacement",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneDualSlotTest::RunTest(const FString&)
{
	UReEchoRunSubsystem* Run = NewRuneRun();
	Run->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_22"));
	const FName Other(TEXT("P_GUN_TRIPLESPREAD_MUZZLE_I"));
	FString Error;
	if (!TestTrue(TEXT("Dual muzzle loadout is valid"), Run->TryEquipParts({Core, Other, RuneI}, Error)))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Dual slot purchase page restores"), SetRunePage(Run, {RuneI}));
	TestTrue(TEXT("Matching second muzzle purchase succeeds"), Run->PurchaseShopItem(RuneI));
	TestEqual(TEXT("Dual slot upgrade preserves number of slots"), Run->CurrentBuild.EquippedParts.Num(), 3);
	TestEqual(TEXT("Unrelated first muzzle remains in its slot"), Run->CurrentBuild.EquippedParts[1].PartId, Other);
	TestEqual(TEXT("Upgraded second muzzle remains in its slot"), Run->CurrentBuild.EquippedParts[2].PartId, RuneII);
	TestTrue(TEXT("Manual replacement explicitly selects second slot"), Run->TryEquipPurchasedPartAt(RuneII, 1, Error));
	TestEqual(TEXT("Idempotent re-equip does not mint a second copy"), Run->RuneAcquisitionCounts.FindRef(RuneII), 1);
	// Replacing equipped I with a different rune puts I back into its backpack, where it meets another I.
	Run = NewRuneRun();
	TestTrue(TEXT("Replacement fixture equipped I"), Run->TryEquipParts({Core, RuneI}, Error));
	Run->RuneAcquisitionCounts[RuneI] = 2;
	Run->OwnedPartIds.Add(Other);
	Run->RuneAcquisitionCounts.Add(Other, 1);
	TestTrue(TEXT("Backpack replacement invokes synthesis"), Run->TryEquipPurchasedPartAt(Other, 0, Error));
	TestTrue(TEXT("Selected replacement stays equipped"), HasEquipped(Run, Other));
	TestEqual(TEXT("Displaced I settles with bag I into II"), Run->RuneAcquisitionCounts.FindRef(RuneII), 1);
	TestFalse(TEXT("Backpack synthesis does not displace replacement"), HasEquipped(Run, RuneII));
	return true;
}
#endif
