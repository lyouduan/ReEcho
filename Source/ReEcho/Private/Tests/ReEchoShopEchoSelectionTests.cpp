// Echo-storage verification: the authoritative store/skip/replace
// commands (owned by UReEchoRunSubsystem) and the presentation-only intermission
// widget. The GameMode close-gate that consumes these commands is covered by the
// Editor (UHT/UBT) build; its data dependency is exactly `bHasPendingRecording`,
// which these tests exercise directly. No PIE / visual verification is performed.
#if WITH_DEV_AUTOMATION_TESTS

#include "Core/ReEchoTypes.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoInventoryShopWidget.h"

#include "Misc/AutomationTest.h"

namespace
{
FReEchoRecording MakeRecording()
{
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	return Recording;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopEchoSelectionPlan32Commands,
                                 "ReEcho.Shop.EchoSelection.Plan32.Commands",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopEchoSelectionPlan32Commands::RunTest(const FString& Parameters)
{
	// UReEchoRunSubsystem has ClassWithin=GameInstance, so it must be created with a
	// valid GameInstance outer (a transient-package outer triggers a ClassWithin ensure).
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	TestNotNull(TEXT("Run subsystem is available via a GameInstance"), RunSubsystem);
	if (!RunSubsystem)
	{
		return false;
	}

	// Stage a completed encounter as the pending recording.
	const FReEchoRecording First = MakeRecording();
	TestEqual(TEXT("Staging a valid recording succeeds"),
	          RunSubsystem->StagePendingRecording(First),
	          EReEchoEchoStorageResult::Success);
	TestTrue(TEXT("A staged recording is reported as pending"),
	         RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);

	// G_3_02 commits the pending encounter directly as the only time anchor.
	TestEqual(TEXT("Setting the pending recording as time anchor succeeds"),
	          RunSubsystem->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	TestFalse(TEXT("No pending recording remains after anchoring"),
	          RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);
	const FReEchoEchoStorageSummary AfterStore = RunSubsystem->GetEchoStorageSummary();
	TestTrue(TEXT("The time anchor is active"), AfterStore.bHasTimeAnchor);
	TestEqual(TEXT("The anchor contains the staged recording"), AfterStore.TimeAnchorRecording.RecordingId, First.Id);

	// The next choice replaces the single anchor directly; there is no slot-selection step.
	const FReEchoRecording Second = MakeRecording();
	TestEqual(TEXT("Staging the replacement recording succeeds"),
	          RunSubsystem->StagePendingRecording(Second),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Replacing the time anchor succeeds"),
	          RunSubsystem->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	TestFalse(TEXT("No pending recording remains after replacing"),
	          RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);
	const FReEchoEchoStorageSummary AfterReplace = RunSubsystem->GetEchoStorageSummary();
	TestTrue(TEXT("The replacement remains the only anchor"), AfterReplace.bHasTimeAnchor);
	TestEqual(
	    TEXT("The anchor now contains the second recording"), AfterReplace.TimeAnchorRecording.RecordingId, Second.Id);

	// Skipping the pending recording drops permanent-storage eligibility.
	UGameInstance* SkipGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* SkipSubsystem = NewObject<UReEchoRunSubsystem>(SkipGameInstance);
	const FReEchoRecording Fourth = MakeRecording();
	TestEqual(TEXT("Staging for the skip path succeeds"),
	          SkipSubsystem->StagePendingRecording(Fourth),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Skipping the pending recording succeeds"),
	          SkipSubsystem->SkipPendingRecordingStorage(),
	          EReEchoEchoStorageResult::Success);
	TestFalse(TEXT("No pending recording remains after skipping"),
	          SkipSubsystem->GetEchoStorageSummary().bHasPendingRecording);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopEchoSelectionPlan32IntermissionWidget,
                                 "ReEcho.Shop.EchoSelection.Plan32.IntermissionWidget",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoShopEchoSelectionPlan32IntermissionWidget::RunTest(const FString& Parameters)
{
	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage());
	TestNotNull(TEXT("Inventory/shop widget can be created without a gameplay subsystem"), Widget);
	if (!Widget)
	{
		return false;
	}

	Widget->ShowInventory(0, {});
	TestEqual(TEXT("Inventory has its own typed context"), Widget->GetMode(), EReEchoInventoryShopMode::Inventory);

	Widget->ShowShop(0, {});
	TestEqual(TEXT("Manual shop has its own typed context"), Widget->GetMode(), EReEchoInventoryShopMode::ManualShop);

	FReEchoEchoStorageSummary Summary;
	Summary.bHasPendingRecording = true;
	Widget->ShowPostTraitIntermission(0, {}, Summary);
	TestEqual(TEXT("Echo management is restricted to the post-trait context"),
	          Widget->GetMode(),
	          EReEchoInventoryShopMode::PostTraitIntermission);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDeprecatedReplayUnlockRemoved,
                                 "ReEcho.Shop.EchoSelection.DeprecatedReplayUnlockRemoved",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDeprecatedReplayUnlockRemoved::RunTest(const FString& Parameters)
{
	const bool bCatalogContainsDeprecatedItem = GetReEchoShopCatalog().ContainsByPredicate(
	    [](const FReEchoShopOffer& Offer)
	    {
		    return Offer.ItemId == TEXT("SHOP_REPLAY_UNLOCK");
	    });
	TestFalse(TEXT("Legacy replay unlock is absent from the catalog"), bCatalogContainsDeprecatedItem);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RunSubsystem = NewObject<UReEchoRunSubsystem>(GameInstance);
	RunSubsystem->TimeShards = 50;
	TestFalse(TEXT("Legacy replay unlock cannot be purchased by id"),
	          RunSubsystem->PurchaseShopItem(TEXT("SHOP_REPLAY_UNLOCK")));
	TestEqual(TEXT("Rejected legacy id does not consume shards"), RunSubsystem->TimeShards, 50);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
