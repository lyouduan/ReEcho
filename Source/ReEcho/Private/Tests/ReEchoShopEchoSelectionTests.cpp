// Plan32 rework verification: the authoritative echo store/skip/replace/select
// commands (owned by UReEchoRunSubsystem) and the presentation-only intermission
// widget. The GameMode close-gate that consumes these commands is covered by the
// Editor (UHT/UBT) build; its data dependency is exactly `bHasPendingRecording`,
// which these tests exercise directly. No PIE / visual verification is performed.
//
// NOTE: This rework deliberately does NOT introduce SHOP_REPLAY_UNLOCK or reuse
// the rejected plan/31-shop-echo-selection-ui self-contained widget API.
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

	// Storing the pending recording moves it into permanent storage.
	TestEqual(TEXT("Storing the pending recording succeeds"),
	          RunSubsystem->StorePendingRecording(),
	          EReEchoEchoStorageResult::Success);
	TestFalse(TEXT("No pending recording remains after storing"),
	          RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);
	const FReEchoEchoStorageSummary AfterStore = RunSubsystem->GetEchoStorageSummary();
	const bool bStoredFirst = AfterStore.StoredEchoes.ContainsByPredicate(
	    [&First](const FReEchoStoredEchoSummary& Stored) { return Stored.RecordingId == First.Id; });
	TestTrue(TEXT("The stored echo snapshot contains the staged recording"), bStoredFirst);

	// Replacing: stage a second recording and replace the first stored echo with it.
	const FReEchoRecording Second = MakeRecording();
	TestEqual(TEXT("Staging the replacement recording succeeds"),
	          RunSubsystem->StagePendingRecording(Second),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Replacing a stored echo with the pending recording succeeds"),
	          RunSubsystem->StorePendingRecordingReplacing(First.Id),
	          EReEchoEchoStorageResult::Success);
	TestFalse(TEXT("No pending recording remains after replacing"),
	          RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);
	const FReEchoEchoStorageSummary AfterReplace = RunSubsystem->GetEchoStorageSummary();
	TestEqual(TEXT("Storage still holds exactly one echo after replace"), AfterReplace.StoredEchoes.Num(), 1);
	const bool bStoredSecond = AfterReplace.StoredEchoes.ContainsByPredicate(
	    [&Second](const FReEchoStoredEchoSummary& Stored) { return Stored.RecordingId == Second.Id; });
	TestTrue(TEXT("The stored echo snapshot now contains the replacement recording"), bStoredSecond);

	// Replacing with an unknown target is rejected and leaves the pending recording intact.
	const FReEchoRecording Third = MakeRecording();
	TestEqual(TEXT("Staging the third recording succeeds"),
	          RunSubsystem->StagePendingRecording(Third),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Replacing with an unknown target is rejected"),
	          RunSubsystem->StorePendingRecordingReplacing(FGuid::NewGuid()),
	          EReEchoEchoStorageResult::InvalidReplacementTarget);
	TestTrue(TEXT("The pending recording is preserved after a rejected replacement"),
	         RunSubsystem->GetEchoStorageSummary().bHasPendingRecording);

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

	// Selection: choosing replays is bounded by the specific-replay limit and must reference stored echoes.
	UGameInstance* SelectGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* SelectSubsystem = NewObject<UReEchoRunSubsystem>(SelectGameInstance);
	const FReEchoRecording Fifth = MakeRecording();
	TestEqual(TEXT("Staging for the selection path succeeds"),
	          SelectSubsystem->StagePendingRecording(Fifth),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Storing the selection-path recording succeeds"),
	          SelectSubsystem->StorePendingRecording(),
	          EReEchoEchoStorageResult::Success);

	SelectSubsystem->SetSpecificReplayLimit(0);
	TestEqual(TEXT("Selecting any replay is rejected when the limit is zero"),
	          SelectSubsystem->SetSelectedReplayIds({Fifth.Id}),
	          EReEchoEchoStorageResult::ReplayLimitExceeded);

	SelectSubsystem->SetSpecificReplayLimit(2);
	TestEqual(TEXT("Selecting a stored echo succeeds within the limit"),
	          SelectSubsystem->SetSelectedReplayIds({Fifth.Id}),
	          EReEchoEchoStorageResult::Success);
	TestTrue(TEXT("The chosen replay id is recorded in the summary"),
	         SelectSubsystem->GetEchoStorageSummary().SelectedReplayIds.Contains(Fifth.Id));

	TestEqual(TEXT("Selecting an unknown echo is rejected"),
	          SelectSubsystem->SetSelectedReplayIds({FGuid::NewGuid()}),
	          EReEchoEchoStorageResult::InvalidRecordingId);

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
	TestEqual(TEXT("Inventory has its own typed context"),
	          Widget->GetMode(),
	          EReEchoInventoryShopMode::Inventory);

	Widget->ShowShop(0, {});
	TestEqual(TEXT("Manual shop has its own typed context"),
	          Widget->GetMode(),
	          EReEchoInventoryShopMode::ManualShop);

	FReEchoEchoStorageSummary Summary;
	Summary.bHasPendingRecording = true;
	Widget->ShowPostTraitIntermission(0, {}, Summary);
	TestEqual(TEXT("Echo management is restricted to the post-trait context"),
	          Widget->GetMode(),
	          EReEchoInventoryShopMode::PostTraitIntermission);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
