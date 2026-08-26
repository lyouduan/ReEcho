#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ReEchoGameMode.h"
#include "Run/ReEchoRunSubsystem.h"
#include "UI/ReEchoEncounterTransitionWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEncounterTransitionPolicyTest,
                                 "ReEcho.UI.EncounterTransition.Policy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEncounterTransitionPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Above three seconds is clear"),
	          UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(3.01f),
	          0.0f);
	TestEqual(TEXT("Three seconds starts at zero"),
	          UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(3.0f),
	          0.0f);
	TestEqual(TEXT("Two seconds is half strength"),
	          UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(2.0f),
	          0.5f);
	TestEqual(TEXT("One second holds full strength"),
	          UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(1.0f),
	          1.0f);
	TestEqual(
	    TEXT("Zero clears before sequence"), UReEchoEncounterTransitionWidget::CalculateCountdownIntensity(0.0f), 0.0f);

	const FVector2D WideFill = UReEchoEncounterTransitionWidget::CalculateFillSize(FVector2D(2560.0f, 1080.0f));
	TestTrue(TEXT("Ultrawide Fill covers width"), WideFill.X >= 2560.0f);
	TestTrue(TEXT("Ultrawide Fill crops height"), WideFill.Y >= 1080.0f);
	TestEqual(TEXT("Fill preserves source aspect"), static_cast<double>(WideFill.X / WideFill.Y), 16.0 / 9.0, 0.0001);

	TestTrue(TEXT("Three seconds starts countdown post process"),
	         AReEchoGameMode::ShouldStartEncounterTransition(3.0f, false, false));
	TestTrue(TEXT("Sub-three keeps countdown post process active"),
	         AReEchoGameMode::ShouldStartEncounterTransition(2.9f, false, false));
	TestFalse(TEXT("Above three seconds does not start"),
	          AReEchoGameMode::ShouldStartEncounterTransition(3.01f, false, false));
	TestFalse(TEXT("Zero sequence is started by authoritative encounter end"),
	          AReEchoGameMode::ShouldStartEncounterTransition(0.0f, false, false));
	TestFalse(TEXT("Boss encounter does not start"),
	          AReEchoGameMode::ShouldStartEncounterTransition(3.0f, true, false));
	TestFalse(TEXT("Active transition does not restart"),
	          AReEchoGameMode::ShouldStartEncounterTransition(3.0f, false, true));

	TestFalse(TEXT("Finished media cannot complete before encounter transition state"),
	          AReEchoGameMode::ShouldCompleteEncounterTransition(false, false, true, 3.1f));
	TestFalse(TEXT("Media failure cannot complete before encounter transition state"),
	          AReEchoGameMode::ShouldCompleteEncounterTransition(false, true, false, 0.1f));
	TestTrue(TEXT("Finished media completes after zero-started transition"),
	         AReEchoGameMode::ShouldCompleteEncounterTransition(true, false, true, 3.1f));
	TestFalse(TEXT("Elapsed time alone cannot bypass MediaPlayer completion"),
	          AReEchoGameMode::ShouldCompleteEncounterTransition(true, false, false, 30.0f));
	return true;
}

#endif
