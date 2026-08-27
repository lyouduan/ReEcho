#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
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
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(3.01f),
	          0.0f);
	TestEqual(TEXT("Three seconds starts at zero"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(3.0f),
	          0.0f);
	TestEqual(TEXT("Two and a half seconds eases in"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(2.5f),
	          0.15625f);
	TestEqual(TEXT("Two seconds is half strength"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(2.0f),
	          0.5f);
	TestEqual(TEXT("One and a half seconds eases toward full strength"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(1.5f),
	          0.84375f);
	TestEqual(TEXT("One second holds full strength"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(1.0f),
	          1.0f);
	TestEqual(TEXT("Zero clears before sequence"),
	          AReEchoArenaCameraActor::CalculateEncounterCountdownPostProcessIntensity(0.0f),
	          0.0f);

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
	TestTrue(TEXT("Encounter 1 proceeds directly to its CG"), AReEchoGameMode::ShouldPlayStage01To02Cg(1));
	TestFalse(TEXT("Encounter 2 retains the normal post-card shop"), AReEchoGameMode::ShouldPlayStage01To02Cg(2));

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Run->BeginEncounter();
	TestFalse(TEXT("An active Encounter 1 cannot be mistaken for its post-encounter CG route"),
	          Run->SkipPostEncounterCardChoiceForStageTransitionCg());
	Run->CompleteEncounter(FReEchoRecording(), true, false);
	TestTrue(TEXT("Encounter 1 reward phase can be skipped for the direct CG route"),
	         Run->SkipPostEncounterCardChoiceForStageTransitionCg());
	TestEqual(TEXT("Direct CG route leaves Run ready for the next encounter"), Run->Phase, EReEchoRunPhase::Planning);
	Run->BeginEncounter();
	TestFalse(TEXT("The direct CG reward skip is restricted to Encounter 1"),
	          Run->SkipPostEncounterCardChoiceForStageTransitionCg());
	return true;
}

#endif
