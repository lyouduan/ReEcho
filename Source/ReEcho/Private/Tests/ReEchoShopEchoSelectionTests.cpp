// Plan31 UI state / delegate automation: echo store / skip / replace / replay-selection.
// Exercises the widget's decision state machine by injecting a RunSubsystem directly;
// the widget only ever holds the read-only summary and stable GUIDs, never full recordings.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/ReEchoTypes.h"
#include "Engine/GameInstance.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoInventoryShopWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoShopEchoSelectionTest,
                                 "ReEcho.Shop.EchoSelection.Plan31",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	FReEchoRecording MakeRecording(int32 Index)
	{
		FReEchoRecording R;
		R.Id = FGuid::NewGuid();
		R.EncounterIndex = Index;
		R.Duration = 1.0f;
		R.MapId = FName(TEXT("Map"));
		R.BuildSnapshot.CharacterId = FName(*FString::Printf(TEXT("Char%d"), Index));
		R.BuildSnapshot.WeaponId = FName(*FString::Printf(TEXT("Wpn%d"), Index));
		return R;
	}
} // namespace

bool FReEchoShopEchoSelectionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	if (!TestNotNull(TEXT("RunSubsystem created"), Run))
	{
		return false;
	}

	UReEchoInventoryShopWidget* Widget = NewObject<UReEchoInventoryShopWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Widget created"), Widget))
	{
		return false;
	}

	// ---- Store into a free slot ----
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	Run->SetSpecificReplayLimit(0);
	FReEchoRecording R1 = MakeRecording(1);
	Run->StagePendingRecording(R1);
	Widget->RefreshEchoState(Run);
	TestTrue(TEXT("Pending staged"), Widget->GetEchoSummary().bHasPendingRecording);
	TestFalse(TEXT("Pending undecided before store"), Widget->IsPendingEchoDecided());

	Widget->RequestStoreEcho(Run);
	TestFalse(TEXT("Pending cleared after store"), Widget->GetEchoSummary().bHasPendingRecording);
	TestEqual(TEXT("One stored echo"), Widget->GetEchoSummary().StoredEchoes.Num(), 1);
	TestTrue(TEXT("Pending decided after store"), Widget->IsPendingEchoDecided());

	// ---- Skip keeps the rolling latest echo but drops the storage decision ----
	FReEchoRecording R2 = MakeRecording(2);
	Run->StagePendingRecording(R2);
	Widget->RefreshEchoState(Run);
	Widget->RequestSkipEcho(Run);
	TestFalse(TEXT("Pending cleared after skip"), Widget->GetEchoSummary().bHasPendingRecording);
	TestEqual(TEXT("Stored unchanged after skip"), Widget->GetEchoSummary().StoredEchoes.Num(), 1);

	// ---- Full storage: store requires replacement; cancel returns to selection ----
	FReEchoRecording R3 = MakeRecording(3);
	Run->StagePendingRecording(R3);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	FReEchoRecording R4 = MakeRecording(4);
	Run->StagePendingRecording(R4);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	TestEqual(TEXT("Storage full (3)"), Widget->GetEchoSummary().StoredEchoes.Num(), 3);

	FReEchoRecording R5 = MakeRecording(5);
	Run->StagePendingRecording(R5);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run); // full -> replacing mode (never silent evict)
	TestTrue(TEXT("Enter replacing mode when full"),
	         Widget->GetEchoPendingDecision() == EReEchoShopEchoPendingDecision::Replacing);
	TestTrue(TEXT("Pending still undecided in replacing"), !Widget->IsPendingEchoDecided());
	TestTrue(TEXT("Pending still present in replacing"), Widget->GetEchoSummary().bHasPendingRecording);

	Widget->RequestCancelReplaceEcho();
	TestTrue(TEXT("Cancel replace returns to Undecided"),
	         Widget->GetEchoPendingDecision() == EReEchoShopEchoPendingDecision::Undecided);

	const FGuid TargetId = Widget->GetEchoSummary().StoredEchoes[1].RecordingId;
	Widget->RequestReplaceEcho(Run, TargetId);
	TestFalse(TEXT("Pending cleared after replace"), Widget->GetEchoSummary().bHasPendingRecording);
	TestEqual(TEXT("Still 3 stored after replace"), Widget->GetEchoSummary().StoredEchoes.Num(), 3);
	TestTrue(TEXT("Pending decided after replace"), Widget->IsPendingEchoDecided());

	// ---- SpecificReplayLimit == 1: single-select + toggle off ----
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	Run->SetSpecificReplayLimit(1);
	FReEchoRecording B1 = MakeRecording(11);
	Run->StagePendingRecording(B1);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	FReEchoRecording B2 = MakeRecording(12);
	Run->StagePendingRecording(B2);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	TestEqual(TEXT("Two stored for limit-1 selection"), Widget->GetEchoSummary().StoredEchoes.Num(), 2);

	const FGuid S0 = Widget->GetEchoSummary().StoredEchoes[0].RecordingId;
	const FGuid S1 = Widget->GetEchoSummary().StoredEchoes[1].RecordingId;
	Widget->ToggleReplaySelection(Run, S0);
	TestEqual(TEXT("Selection size 1 after first toggle"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 1);
	Widget->ToggleReplaySelection(Run, S1); // limit 1: replaces previous
	TestEqual(TEXT("Selection still 1 after second toggle (single-select)"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 1);
	TestTrue(TEXT("Selection now S1"), Widget->GetEchoSummary().SelectedReplayIds[0] == S1);
	Widget->ToggleReplaySelection(Run, S1); // toggle off
	TestEqual(TEXT("Selection empty after toggle off"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 0);

	// ---- SpecificReplayLimit == 2: multi-select, third rejected ----
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	Run->SetSpecificReplayLimit(2);
	FReEchoRecording C1 = MakeRecording(21);
	Run->StagePendingRecording(C1);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	FReEchoRecording C2 = MakeRecording(22);
	Run->StagePendingRecording(C2);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	FReEchoRecording C3 = MakeRecording(23);
	Run->StagePendingRecording(C3);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	const FGuid S0b = Widget->GetEchoSummary().StoredEchoes[0].RecordingId;
	const FGuid S1b = Widget->GetEchoSummary().StoredEchoes[1].RecordingId;
	const FGuid S2b = Widget->GetEchoSummary().StoredEchoes[2].RecordingId;
	Widget->ToggleReplaySelection(Run, S0b);
	Widget->ToggleReplaySelection(Run, S1b);
	TestEqual(TEXT("Two selected"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 2);
	Widget->ToggleReplaySelection(Run, S2b); // at limit -> rejected
	TestEqual(TEXT("Third rejected at limit 2"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 2);

	// ---- SpecificReplayLimit == 0: no selection offered ----
	Run->StartRun(TEXT("J_CAT"), TEXT("W_J_02"));
	Run->SetSpecificReplayLimit(0);
	FReEchoRecording D1 = MakeRecording(31);
	Run->StagePendingRecording(D1);
	Widget->RefreshEchoState(Run);
	Widget->RequestStoreEcho(Run);
	const FGuid S0c = Widget->GetEchoSummary().StoredEchoes[0].RecordingId;
	Widget->ToggleReplaySelection(Run, S0c);
	TestEqual(TEXT("No selection when limit 0"), Widget->GetEchoSummary().SelectedReplayIds.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
