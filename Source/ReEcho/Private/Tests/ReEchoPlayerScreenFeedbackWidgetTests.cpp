#include "UI/ReEchoPlayerScreenFeedbackWidget.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Misc/AutomationTest.h"
#include "UI/ReEchoPlayerHudWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerScreenFeedbackMathTest,
                                 "ReEcho.UI.PlayerScreenFeedback.Math",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerScreenFeedbackMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full health is normalized"),
	          UReEchoPlayerScreenFeedbackWidget::CalculateHealthRatio(100.0f, 100.0f),
	          1.0f);
	TestEqual(TEXT("Health is clamped at zero"),
	          UReEchoPlayerScreenFeedbackWidget::CalculateHealthRatio(-10.0f, 100.0f),
	          0.0f);
	TestEqual(TEXT("Combined intensity is capped"),
	          UReEchoPlayerScreenFeedbackWidget::CalculateCombinedIntensity(0.6f, 0.8f, 0.85f),
	          0.85f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerScreenFeedbackFilterTest,
                                 "ReEcho.UI.PlayerScreenFeedback.Filter",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerScreenFeedbackFilterTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Applied unblocked damage plays feedback"),
	         UReEchoPlayerScreenFeedbackWidget::ShouldPlayHurtFeedback(1.0f, false));
	TestFalse(TEXT("Zero damage does not play feedback"),
	          UReEchoPlayerScreenFeedbackWidget::ShouldPlayHurtFeedback(0.0f, false));
	TestFalse(TEXT("Blocked damage does not play feedback"),
	          UReEchoPlayerScreenFeedbackWidget::ShouldPlayHurtFeedback(1.0f, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerScreenFeedbackLifecycleTest,
                                 "ReEcho.UI.PlayerScreenFeedback.Lifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerScreenFeedbackLifecycleTest::RunTest(const FString& Parameters)
{
	UReEchoPlayerScreenFeedbackWidget* Feedback = NewObject<UReEchoPlayerScreenFeedbackWidget>();
	Feedback->SetHealth(10.0f, 100.0f);
	TestTrue(TEXT("Low health maps to a positive configurable target"),
	         Feedback->GetTargetLowHealthIntensityForTests() > 0.0f);
	Feedback->PlayHurtFeedback(10.0f, 100.0f);
	TestTrue(TEXT("Damage starts a hurt pulse"), Feedback->GetHurtIntensityForTests() > 0.0f);
	TestEqual(TEXT("A retrigger starts from the beginning"), Feedback->GetHurtElapsedSecondsForTests(), 0.0f);

	UReEchoPlayerHudWidget* Hud = NewObject<UReEchoPlayerHudWidget>();
	TestNotNull(TEXT("HUD resolves the editable feedback Widget Blueprint class"),
	            Hud->GetScreenFeedbackClassForTests());
	if (!Hud->GetScreenFeedbackClassForTests())
	{
		return false;
	}
	TestTrue(TEXT("HUD uses the cooked feedback Widget Blueprint class"),
	         Hud->GetScreenFeedbackClassForTests()->GetPathName().Contains(TEXT("WBP_ReEchoPlayerScreenFeedback_C")));
	UReEchoPlayerScreenFeedbackWidget* BlueprintFeedback =
	    NewObject<UReEchoPlayerScreenFeedbackWidget>(GetTransientPackage(), Hud->GetScreenFeedbackClassForTests());
	BlueprintFeedback->TakeWidget();
	TestTrue(TEXT("Empty feedback WBP receives the native full-screen visual tree"),
	         BlueprintFeedback->HasVignetteVisualForTests());
	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>();
	UReEchoCombatEventsComponent* FirstEvents = NewObject<UReEchoCombatEventsComponent>();
	UReEchoCombatEventsComponent* SecondEvents = NewObject<UReEchoCombatEventsComponent>();
	Hud->InitializePlayerHud(Combatant, FirstEvents, nullptr);
	Hud->InitializePlayerHud(Combatant, FirstEvents, nullptr);
	TestTrue(TEXT("Repeated initialization keeps the current event binding"),
	         Hud->IsBoundToCombatEventsForTests(FirstEvents));
	Hud->InitializePlayerHud(Combatant, SecondEvents, nullptr);
	TestFalse(TEXT("Rebinding removes the old event source"), Hud->IsBoundToCombatEventsForTests(FirstEvents));
	TestTrue(TEXT("Rebinding installs the new event source"), Hud->IsBoundToCombatEventsForTests(SecondEvents));
	return true;
}

#endif
