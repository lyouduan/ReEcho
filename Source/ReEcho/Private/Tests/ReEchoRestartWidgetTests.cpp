#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/ReEchoRestartWidget.h"

namespace
{
template <typename WidgetType> WidgetType* FindRestartWidget(UReEchoRestartWidget* RestartWidget, const FName Name)
{
	return Cast<WidgetType>(RestartWidget->GetWidgetFromName(Name));
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRestartWidgetPresentationTest,
                                 "ReEcho.UI.RestartWidgetPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRestartWidgetPresentationTest::RunTest(const FString& Parameters)
{
	UClass* RestartClass =
	    LoadClass<UReEchoRestartWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart.WBP_ReEchoRestart_C"));
	TestNotNull(TEXT("Designed restart widget class loads"), RestartClass);
	if (!RestartClass)
	{
		return false;
	}

	UReEchoRestartWidget* RestartWidget = NewObject<UReEchoRestartWidget>(GetTransientPackage(), RestartClass);
	TestNotNull(TEXT("Designed restart widget can be instantiated"), RestartWidget);
	if (!RestartWidget)
	{
		return false;
	}
	TestTrue(TEXT("Designed restart widget initializes its widget tree"), RestartWidget->Initialize());
	RestartWidget->TakeWidget();
	TestEqual(TEXT("Runtime class is the authored restart WBP"),
	          RestartWidget->GetClass()->GetPathName(),
	          FString(TEXT("/Game/ReEcho/UI/WBP_ReEchoRestart.WBP_ReEchoRestart_C")));
	TestEqual(TEXT("Designed restart keeps its authored root"),
	          RestartWidget->WidgetTree->RootWidget->GetFName(),
	          FName(TEXT("CanvasPanel_0")));

	UTextBlock* TitleText = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("TitleText"));
	UVerticalBox* RootPanel = FindRestartWidget<UVerticalBox>(RestartWidget, TEXT("RootPanel"));
	UButton* ResumeButton = FindRestartWidget<UButton>(RestartWidget, TEXT("ResumeButton"));
	UButton* RestartButton = FindRestartWidget<UButton>(RestartWidget, TEXT("RestartButton"));
	UButton* QuitButton = FindRestartWidget<UButton>(RestartWidget, TEXT("QuitButton"));
	UButton* PauseSettingsButton = FindRestartWidget<UButton>(RestartWidget, TEXT("PauseSettingsButton"));
	UImage* ArtPauseDimmer = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseDimmer"));
	UImage* ArtPausePrimaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPausePrimaryButton"));
	UImage* ArtPauseSecondaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseSecondaryButton"));
	UImage* ArtPauseTertiaryButton = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseTertiaryButton"));
	UImage* ArtRestartDialogPanel = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtRestartDialogPanel"));

	TestNotNull(TEXT("Pause title exists"), TitleText);
	TestNotNull(TEXT("Pause root exists"), RootPanel);
	TestNotNull(TEXT("Resume button exists"), ResumeButton);
	TestNotNull(TEXT("Exit-to-menu button exists"), RestartButton);
	TestNotNull(TEXT("Exit-game button exists"), QuitButton);
	TestNotNull(TEXT("Pause settings button exists"), PauseSettingsButton);
	TestNotNull(TEXT("Pause dimmer exists"), ArtPauseDimmer);
	TestNotNull(TEXT("Pause primary button art exists"), ArtPausePrimaryButton);
	TestNotNull(TEXT("Pause secondary button art exists"), ArtPauseSecondaryButton);
	TestNotNull(TEXT("Pause tertiary button art exists"), ArtPauseTertiaryButton);
	TestNotNull(TEXT("Restart dialog panel exists"), ArtRestartDialogPanel);
	if (!TitleText || !RootPanel || !ResumeButton || !RestartButton || !QuitButton || !PauseSettingsButton ||
	    !ArtPauseDimmer || !ArtPausePrimaryButton || !ArtPauseSecondaryButton || !ArtPauseTertiaryButton ||
	    !ArtRestartDialogPanel)
	{
		return false;
	}

	RestartWidget->SetDeathScreen(false);
	TestEqual(TEXT("Normal pause title"), TitleText->GetText().ToString(), FString(TEXT("游戏暂停")));
	TestEqual(TEXT("C++ preserves the authored pause composition position"),
	          RootPanel->GetRenderTransform().Translation,
	          FVector2D::ZeroVector);
	TestEqual(TEXT("Resume button is interactive"), ResumeButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-to-menu button is interactive"), RestartButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-game button is interactive"), QuitButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Settings gear is interactive"), PauseSettingsButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Pause dimmer does not intercept input"),
	          ArtPauseDimmer->GetVisibility(),
	          ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Primary art is visible"), ArtPausePrimaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Secondary art is visible"), ArtPauseSecondaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Tertiary art is visible"), ArtPauseTertiaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Primary art retains its source width"), ArtPausePrimaryButton->GetBrush().ImageSize.X >= 420.0f);
	TestTrue(TEXT("Primary art preserves the formal aspect height"), ArtPausePrimaryButton->GetBrush().ImageSize.Y >= 140.0f);

	RestartWidget->SetQuitConfirmation(true, true, 4);
	TestEqual(TEXT("Exit-to-menu confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出到主菜单?")));
	TestEqual(TEXT("Exit-to-menu confirmation uses the current encounter as its save point"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("MessageText"))->GetText().ToString(),
	          FString(TEXT("存档点：第 4 关")));
	TestEqual(TEXT("Exit-without-save button remains interactive"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Save-and-exit label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("ResumeButtonLabel"))->GetText().ToString(),
	          FString(TEXT("保存并退出")));
	TestEqual(TEXT("Exit-without-save label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("RestartButtonLabel"))->GetText().ToString(),
	          FString(TEXT("不保存并退出")));
	TestEqual(TEXT("Back label"),
	          FindRestartWidget<UTextBlock>(RestartWidget, TEXT("QuitButtonText"))->GetText().ToString(),
	          FString(TEXT("返回")));
	TestEqual(TEXT("Settings gear is hidden during confirmation"),
	          PauseSettingsButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Exit-to-menu confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	RestartWidget->SetQuitConfirmation(true, false, 4);
	TestEqual(TEXT("Exit-game confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出游戏?")));
	TestEqual(TEXT("Exit-game confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	return true;
}
