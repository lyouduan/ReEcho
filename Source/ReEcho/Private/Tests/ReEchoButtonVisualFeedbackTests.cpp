#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Engine/GameInstance.h"
#include "ReEchoAudioEvents.h"
#include "UI/Framework/ReEchoButtonAudioFeedback.h"
#include "UI/Framework/ReEchoButtonVisualFeedback.h"
#include "UI/Framework/ReEchoUIFlowCoordinatorSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoButtonAudioFeedbackBindingTest,
                                 "ReEcho.UI.ButtonAudioFeedback.SemanticBinding",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoButtonAudioFeedbackBindingTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UButton* Button = NewObject<UButton>(GameInstance);
	UReEchoButtonAudioFeedback* Feedback = NewObject<UReEchoButtonAudioFeedback>(GameInstance);
	Feedback->Bind(Button, GameInstance, FReEchoAudioEvents::UiHover, FReEchoAudioEvents::UiCardSelect);

	TestTrue(TEXT("Semantic audio binding retains its button"), Feedback->IsBoundTo(Button));
	TestTrue(TEXT("Semantic audio binding remains valid"), Feedback->HasValidButton());
	TestTrue(TEXT("Hover delegate is bound"), Button->OnHovered.IsBound());
	TestTrue(TEXT("Click delegate is bound"), Button->OnClicked.IsBound());
	return true;
}

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
	Feedback->Bind(Button, VisualRoot, TEXT("TestScreen"), TEXT("TestWidget"));
	Button->OnHovered.Broadcast();

	TestTrue(TEXT("Hover multiplies the authored X scale"),
	         FMath::IsNearlyEqual(VisualRoot->GetRenderTransform().Scale.X, 0.84f, 1.e-5f));
	TestTrue(TEXT("Hover multiplies the authored Y scale"),
	         FMath::IsNearlyEqual(VisualRoot->GetRenderTransform().Scale.Y, 0.945f, 1.e-5f));
	TestEqual(TEXT("Hover uses a centered pivot"), VisualRoot->GetRenderTransformPivot(), FVector2D(0.5f, 0.5f));

	Button->OnUnhovered.Broadcast();
	TestEqual(TEXT("Unhover restores authored scale"), VisualRoot->GetRenderTransform().Scale, FVector2D(0.8f, 0.9f));
	TestEqual(TEXT("Unhover restores authored pivot"), VisualRoot->GetRenderTransformPivot(), FVector2D(0.2f, 0.3f));

	Button->OnClicked.Broadcast();
	FString AuditContents;
	const FString AuditPath = FPaths::Combine(FPaths::ProjectLogDir(), TEXT("UIInteractionAudit.log"));
	TestTrue(TEXT("Button click audit file is readable"), FFileHelper::LoadFileToString(AuditContents, *AuditPath));
	TestTrue(TEXT("Button click audit records the screen context"), AuditContents.Contains(TEXT("screen=TestScreen")));
	TestTrue(TEXT("Button click audit records the widget context"), AuditContents.Contains(TEXT("widget=TestWidget")));
	TestTrue(TEXT("Button click audit records the button name"),
	         AuditContents.Contains(FString::Printf(TEXT("button=%s"), *Button->GetName())));
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

	UOverlay* ContentButtonRoot = NewObject<UOverlay>();
	ContentButtonRoot->AddChild(NewObject<UImage>(ContentButtonRoot));
	UButton* ButtonWithContentAndExternalArt = NewObject<UButton>(ContentButtonRoot);
	ButtonWithContentAndExternalArt->AddChild(NewObject<UImage>(ButtonWithContentAndExternalArt));
	ContentButtonRoot->AddChild(ButtonWithContentAndExternalArt);
	TestEqual(TEXT("A content button still scales its single-button overlay with external art"),
	          UReEchoUIFlowCoordinatorSubsystem::ResolveButtonVisualRoot(ButtonWithContentAndExternalArt),
	          static_cast<UWidget*>(ContentButtonRoot));

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
