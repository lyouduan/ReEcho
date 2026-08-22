#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "UI/Framework/ReEchoButtonVisualFeedback.h"
#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoButtonVisualFeedbackTest,
                                 "ReEcho.UI.ButtonVisualFeedback.HoverScale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoButtonVisualFeedbackTest::RunTest(const FString& Parameters)
{
	UButton* Button = NewObject<UButton>();
	UOverlay* VisualRoot = NewObject<UOverlay>();
	VisualRoot->SetRenderScale(FVector2D(0.8f, 0.9f));
	VisualRoot->SetRenderTransformPivot(FVector2D(0.2f, 0.3f));

	UReEchoButtonVisualFeedback* Feedback = NewObject<UReEchoButtonVisualFeedback>(Button);
	Feedback->Bind(Button, VisualRoot);
	Button->OnHovered.Broadcast();

	TestTrue(TEXT("Hover multiplies the authored X scale"),
	         FMath::IsNearlyEqual(VisualRoot->GetRenderTransform().Scale.X, 0.84f));
	TestTrue(TEXT("Hover multiplies the authored Y scale"),
	         FMath::IsNearlyEqual(VisualRoot->GetRenderTransform().Scale.Y, 0.945f));
	TestEqual(TEXT("Hover uses a centered pivot"), VisualRoot->GetRenderTransformPivot(), FVector2D(0.5f, 0.5f));

	Button->OnUnhovered.Broadcast();
	TestEqual(TEXT("Unhover restores authored scale"), VisualRoot->GetRenderTransform().Scale, FVector2D(0.8f, 0.9f));
	TestEqual(TEXT("Unhover restores authored pivot"), VisualRoot->GetRenderTransformPivot(), FVector2D(0.2f, 0.3f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoButtonVisualRootTest,
                                 "ReEcho.UI.ButtonVisualFeedback.VisualRoot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoButtonVisualRootTest::RunTest(const FString& Parameters)
{
	UOverlay* TransparentButtonRoot = NewObject<UOverlay>();
	UImage* Background = NewObject<UImage>(TransparentButtonRoot);
	UButton* TransparentButton = NewObject<UButton>(TransparentButtonRoot);
	TransparentButtonRoot->AddChild(Background);
	TransparentButtonRoot->AddChild(TransparentButton);
	TestEqual(TEXT("A lone transparent button scales its complete overlay"),
	          UReEchoUIFlowCoordinatorSubsystem::ResolveButtonVisualRoot(TransparentButton),
	          static_cast<UWidget*>(TransparentButtonRoot));

	UButton* ContentButton = NewObject<UButton>();
	ContentButton->AddChild(NewObject<UImage>(ContentButton));
	TestEqual(TEXT("A button containing its art scales itself"),
	          UReEchoUIFlowCoordinatorSubsystem::ResolveButtonVisualRoot(ContentButton),
	          static_cast<UWidget*>(ContentButton));

	UOverlay* SharedOverlay = NewObject<UOverlay>();
	UButton* FirstButton = NewObject<UButton>(SharedOverlay);
	SharedOverlay->AddChild(FirstButton);
	SharedOverlay->AddChild(NewObject<UButton>(SharedOverlay));
	TestEqual(TEXT("A shared overlay never scales as one button"),
	          UReEchoUIFlowCoordinatorSubsystem::ResolveButtonVisualRoot(FirstButton),
	          static_cast<UWidget*>(FirstButton));
	return true;
}

#endif
