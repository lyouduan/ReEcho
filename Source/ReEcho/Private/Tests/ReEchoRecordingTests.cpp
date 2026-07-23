#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/ReEchoTypes.h"
#include "Recording/ReEchoRecorderComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRecordingInterpolationTest,
                                 "ReEcho.Recording.InterpolatesAndHolds",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRecordingInterpolationTest::RunTest(const FString& Parameters)
{
	FReEchoRecording Recording;
	FReEchoPositionSample Start;
	Start.Time = 0.f;
	Start.Position = FVector::ZeroVector;
	FReEchoPositionSample End;
	End.Time = 1.f;
	End.Position = FVector(100.f, 0.f, 0.f);
	Recording.Positions = {Start, End};
	TestTrue("Midpoint is linearly interpolated",
	         Recording.EvaluatePosition(0.5f).Equals(FVector(50.f, 0.f, 0.f), KINDA_SMALL_NUMBER));
	TestTrue("Playback holds its final position",
	         Recording.EvaluatePosition(30.f).Equals(End.Position, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FReEchoWeaponTimelineRecordingTest,
	"ReEcho.Recording.CapturesWeaponTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponTimelineRecordingTest::RunTest(const FString& Parameters)
{
	UReEchoRecorderComponent* Recorder =
		NewObject<UReEchoRecorderComponent>(GetTransientPackage());

	FReEchoBuildSnapshot InitialBuild;
	InitialBuild.WeaponId = TEXT("W_J_02");
	Recorder->BeginRecording(1, TEXT("TestArena"), 1337, InitialBuild);
	Recorder->RecordWeaponChange(4.0f, TEXT("W_J_01"));
	Recorder->RecordWeaponChange(5.0f, TEXT("W_J_01"));
	Recorder->RecordWeaponChange(9.5f, TEXT("W_J_03"));

	const FReEchoRecording Recording = Recorder->FinishRecording(10.0f);
	TestEqual(
		TEXT("Initial and two distinct changes are recorded"),
		Recording.WeaponChanges.Num(),
		3);
	TestEqual(
		TEXT("Timeline starts with the initial weapon"),
		Recording.WeaponChanges[0].WeaponId,
		FName(TEXT("W_J_02")));
	TestEqual(
		TEXT("Sword switch keeps its encounter time"),
		Recording.WeaponChanges[1].Time,
		4.0f);
	TestEqual(
		TEXT("Duplicate weapon selection is ignored"),
		Recording.WeaponChanges[2].WeaponId,
		FName(TEXT("W_J_03")));
	TestEqual(
		TEXT("Build snapshot retains the final weapon"),
		Recording.BuildSnapshot.WeaponId,
		FName(TEXT("W_J_03")));
	return true;
}
#endif

