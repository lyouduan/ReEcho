#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Encounter/ReEchoEncounterFlowSettings.h"
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
	TestEqual(TEXT("Stage camera ease clamps before start"),
	          AReEchoArenaCameraActor::CalculateStage01To02CameraEaseAlpha(-1.0f),
	          0.0f);
	TestEqual(TEXT("Stage camera ease is symmetric at midpoint"),
	          AReEchoArenaCameraActor::CalculateStage01To02CameraEaseAlpha(0.5f),
	          0.5f);
	TestEqual(TEXT("Stage camera ease clamps after completion"),
	          AReEchoArenaCameraActor::CalculateStage01To02CameraEaseAlpha(2.0f),
	          1.0f);
	const AReEchoArenaCameraActor* CameraDefaults = GetDefault<AReEchoArenaCameraActor>();
	TestEqual(TEXT("Pre-CG camera push lasts one second while globally paused"),
	          CameraDefaults->GetStage01To02PlayerFocusDuration(),
	          1.0f);
	TestEqual(
	    TEXT("Player close-up holds for half a second"), CameraDefaults->GetStage01To02PlayerHoldDuration(), 0.5f);
	TestEqual(TEXT("Player and Echo use the same doubled close-up ratio"),
	          CameraDefaults->GetStage01To02PlayerFocusRatio(),
	          CameraDefaults->GetStage01To02EchoFocusRatio());
	TestEqual(TEXT("Close-up ratio is twice as near as the previous candidate"),
	          CameraDefaults->GetStage01To02PlayerFocusRatio(),
	          0.325f);
	TestEqual(TEXT("Echo stays locked while zooming out for one second"),
	          CameraDefaults->GetStage01To02EchoZoomOutDuration(),
	          1.0f);
	TestEqual(TEXT("Pan from Echo to player at standard width lasts one second"),
	          CameraDefaults->GetStage01To02MoveToPlayerDuration(),
	          1.0f);

	const FVector2D WideFill = UReEchoEncounterTransitionWidget::CalculateFillSize(FVector2D(2560.0f, 1080.0f));
	TestTrue(TEXT("Ultrawide Fill covers width"), WideFill.X >= 2560.0f);
	TestTrue(TEXT("Ultrawide Fill crops height"), WideFill.Y >= 1080.0f);
	TestEqual(TEXT("Fill preserves source aspect"), static_cast<double>(WideFill.X / WideFill.Y), 16.0 / 9.0, 0.0001);
	const FVector2D CardChoiceFrameSize =
	    UReEchoEncounterTransitionWidget::CalculateCardChoiceFrameSize(FVector2D(1920.0f, 1080.0f));
	const FVector2D CardChoiceFramePosition =
	    UReEchoEncounterTransitionWidget::CalculateCardChoiceFramePosition(FVector2D(1920.0f, 1080.0f));
	TestEqual(TEXT("Playing transition uses the same width as the card-choice Fit frame"),
	          CardChoiceFrameSize.X,
	          1267.2,
	          0.01);
	TestEqual(TEXT("Playing transition preserves 16:9 inside the card-choice frame"),
	          static_cast<double>(CardChoiceFrameSize.X / CardChoiceFrameSize.Y),
	          16.0 / 9.0,
	          0.0001);
	TestEqual(
	    TEXT("Playing transition is horizontally centered on the game view"), CardChoiceFramePosition.X, 326.4, 0.01);
	TestEqual(TEXT("Both card transitions align with the authored ArtClockNeedle top edge"),
	          CardChoiceFramePosition.Y,
	          87.0,
	          0.01);
	TestEqual(TEXT("Card-to-shop stays framed before the masked figure enters"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseAlpha(0.249),
	          0.0f);
	TestEqual(TEXT("Card-to-shop collapse starts when source frame 68 enters"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseAlpha(0.25),
	          0.0f);
	TestEqual(TEXT("Card-to-shop background starts on the card-choice page"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopBackgroundBlendAlpha(0.0),
	          0.0f);
	TestEqual(TEXT("Card-to-shop background is halfway through its fade when source frame 68 enters"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopBackgroundBlendAlpha(0.25),
	          0.5f);
	TestEqual(TEXT("Card-to-shop background reaches full shop opacity on source frame 78"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopBackgroundBlendAlpha(0.5),
	          1.0f);
	TestEqual(TEXT("Card-to-shop collapse finishes on source frame 120"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseAlpha(1.55),
	          1.0f);
	const FVector2D CollapsedSize =
	    UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseSize(FVector2D(1920.0f, 1080.0f), 1.0f);
	const FVector2D CollapsedPosition = UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapsePosition(
	    FVector2D(1920.0f, 1080.0f), 1.0f, FVector2D(1347.0f, 701.0f));
	TestEqual(TEXT("Card-to-shop finishes at the authored shopkeeper width"), CollapsedSize.X, 728.64, 0.01);
	TestEqual(TEXT("Card-to-shop finishes at the authored shopkeeper height"), CollapsedSize.Y, 409.86, 0.01);
	TestEqual(TEXT("Card-to-shop is transparent when it reaches the baked shopkeeper"),
	          UReEchoEncounterTransitionWidget::CalculateCardChoiceToShopCollapseOpacity(1.0f),
	          0.0f);
	TestEqual(TEXT("Card-to-shop final rectangle is centered on the authored shopkeeper X"),
	          CollapsedPosition.X + CollapsedSize.X * 0.5,
	          1347.0,
	          0.01);
	TestEqual(TEXT("Card-to-shop final rectangle is centered on the authored shopkeeper Y"),
	          CollapsedPosition.Y + CollapsedSize.Y * 0.5,
	          701.0,
	          0.01);

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
	TestEqual(TEXT("Echo reveals after the birth circle has established for 0.4 seconds"),
	          AReEchoGameMode::GetStage01To02EchoRevealDelaySeconds(),
	          0.4f);
	TestEqual(TEXT("Echo birth reveal has a bounded Niagara completion fallback"),
	          AReEchoGameMode::GetStage01To02EchoRevealTimeoutSeconds(),
	          1.2f);
	TestFalse(TEXT("First viewing never offers a skip button"),
	          AReEchoGameMode::ShouldOfferStage01To02CgSkip(false, true));
	TestFalse(TEXT("A watched CG does not expose skip outside playback"),
	          AReEchoGameMode::ShouldOfferStage01To02CgSkip(true, false));
	TestTrue(TEXT("A watched CG exposes skip during Stage01To02 playback"),
	         AReEchoGameMode::ShouldOfferStage01To02CgSkip(true, true));
	TestEqual(TEXT("Native encounter-entry protection defaults to half a second"),
	          GetDefault<UReEchoEncounterFlowSettings>()->PostEntryInvulnerabilitySeconds,
	          0.5f);
	TestTrue(TEXT("Encounter 1 receives configured entry protection"),
	         AReEchoGameMode::ShouldGrantPostEntryInvulnerabilityForTests(1, 0.5f));
	TestTrue(TEXT("Encounter 2 receives configured transition protection"),
	         AReEchoGameMode::ShouldGrantPostEntryInvulnerabilityForTests(2, 0.5f));
	TestFalse(TEXT("An inactive Encounter index cannot receive entry protection"),
	          AReEchoGameMode::ShouldGrantPostEntryInvulnerabilityForTests(0, 0.5f));
	TestFalse(TEXT("Zero seconds disables transition protection"),
	          AReEchoGameMode::ShouldGrantPostEntryInvulnerabilityForTests(2, 0.0f));
	const UClass* EncounterFlowBlueprintClass = LoadClass<UReEchoEncounterFlowSettings>(
	    nullptr, TEXT("/Game/ReEcho/Gameplay/Encounter/BP_EncounterFlowSettings.BP_EncounterFlowSettings_C"));
	TestNotNull(TEXT("Dedicated encounter-flow Blueprint is loadable"), EncounterFlowBlueprintClass);
	if (EncounterFlowBlueprintClass)
	{
		const UReEchoEncounterFlowSettings* BlueprintDefaults =
		    EncounterFlowBlueprintClass->GetDefaultObject<UReEchoEncounterFlowSettings>();
		TestTrue(TEXT("Encounter-flow Blueprint keeps a non-negative designer value"),
		         BlueprintDefaults->PostEntryInvulnerabilitySeconds >= 0.0f);
	}
	TestTrue(TEXT("Encounter 2 final free card transitions to shop"),
	         AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(2, EReEchoRunPhase::Planning, true, false));
	TestTrue(TEXT("Encounter 7 final free card transitions to shop"),
	         AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(7, EReEchoRunPhase::Planning, true, false));
	TestFalse(TEXT("Another pending free card does not transition to shop"),
	          AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(2, EReEchoRunPhase::CardChoice, true, false));
	TestFalse(TEXT("Encounter 1 direct CG route does not use card-to-shop media"),
	          AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(1, EReEchoRunPhase::Planning, true, false));
	TestFalse(TEXT("Final encounter does not use card-to-shop media"),
	          AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(8, EReEchoRunPhase::Planning, true, false));
	TestFalse(TEXT("Paid shop card pack return does not use card-to-shop media"),
	          AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(3, EReEchoRunPhase::Planning, true, true));
	TestFalse(TEXT("Failed card application does not use card-to-shop media"),
	          AReEchoGameMode::ShouldPlayCardChoiceToShopTransition(3, EReEchoRunPhase::Planning, false, false));

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
