#include "Misc/AutomationTest.h"

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

	UTextBlock* TitleText = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("TitleText"));
	UVerticalBox* RootPanel = FindRestartWidget<UVerticalBox>(RestartWidget, TEXT("RootPanel"));
	UButton* ResumeButton = FindRestartWidget<UButton>(RestartWidget, TEXT("ResumeButton"));
	UButton* RestartButton = FindRestartWidget<UButton>(RestartWidget, TEXT("RestartButton"));
	UButton* QuitButton = FindRestartWidget<UButton>(RestartWidget, TEXT("QuitButton"));
	UButton* PauseSettingsButton = FindRestartWidget<UButton>(RestartWidget, TEXT("PauseSettingsButton"));
	UImage* ArtPauseDimmer = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseDimmer"));
	UImage* ArtPauseResume = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseResume"));
	UImage* ArtPauseExitToMenu = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseExitToMenu"));
	UImage* ArtPauseExitGame = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseExitGame"));
	UImage* ArtPauseSaveAndExit = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseSaveAndExit"));
	UImage* ArtPauseExitWithoutSave = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseExitWithoutSave"));
	UImage* ArtPauseBack = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtPauseBack"));
	UImage* ArtRestartDialogPanel = FindRestartWidget<UImage>(RestartWidget, TEXT("ArtRestartDialogPanel"));

	TestNotNull(TEXT("Pause title exists"), TitleText);
	TestNotNull(TEXT("Pause root exists"), RootPanel);
	TestNotNull(TEXT("Resume button exists"), ResumeButton);
	TestNotNull(TEXT("Exit-to-menu button exists"), RestartButton);
	TestNotNull(TEXT("Exit-game button exists"), QuitButton);
	TestNotNull(TEXT("Pause settings button exists"), PauseSettingsButton);
	TestNotNull(TEXT("Pause dimmer exists"), ArtPauseDimmer);
	TestNotNull(TEXT("Pause resume art exists"), ArtPauseResume);
	TestNotNull(TEXT("Pause exit-to-menu art exists"), ArtPauseExitToMenu);
	TestNotNull(TEXT("Pause exit-game art exists"), ArtPauseExitGame);
	TestNotNull(TEXT("Pause save-and-exit art exists"), ArtPauseSaveAndExit);
	TestNotNull(TEXT("Pause exit-without-save art exists"), ArtPauseExitWithoutSave);
	TestNotNull(TEXT("Pause back art exists"), ArtPauseBack);
	TestNotNull(TEXT("Restart dialog panel exists"), ArtRestartDialogPanel);
	if (!TitleText || !RootPanel || !ResumeButton || !RestartButton || !QuitButton || !PauseSettingsButton ||
	    !ArtPauseDimmer || !ArtPauseResume || !ArtPauseExitToMenu || !ArtPauseExitGame || !ArtPauseSaveAndExit ||
	    !ArtPauseExitWithoutSave || !ArtPauseBack || !ArtRestartDialogPanel)
	{
		return false;
	}

	RestartWidget->SetDeathScreen(false);
	TestEqual(TEXT("Normal pause title"), TitleText->GetText().ToString(), FString(TEXT("游戏暂停")));
	TestEqual(TEXT("Pause content is lifted into the target composition"),
	          RootPanel->GetRenderTransform().Translation,
	          FVector2D(0.0f, -72.0f));
	TestEqual(TEXT("Resume button is interactive"), ResumeButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-to-menu button is interactive"), RestartButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Exit-game button is interactive"), QuitButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Settings gear is interactive"), PauseSettingsButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Pause dimmer does not intercept input"),
	          ArtPauseDimmer->GetVisibility(),
	          ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Resume art is visible"), ArtPauseResume->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Exit-to-menu art is visible"), ArtPauseExitToMenu->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Exit-game art is visible"), ArtPauseExitGame->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Resume art retains its source width"), ArtPauseResume->GetBrush().ImageSize.X >= 420.0f);
	TestTrue(TEXT("Resume art retains its source height"), ArtPauseResume->GetBrush().ImageSize.Y >= 86.0f);

	RestartWidget->SetQuitConfirmation(true, true);
	TestEqual(TEXT("Exit-to-menu confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出到主菜单?")));
	TestEqual(
	    TEXT("Save-and-exit art is visible"), ArtPauseSaveAndExit->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Exit-without-save art is visible"),
	          ArtPauseExitWithoutSave->GetVisibility(),
	          ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Back art is visible"), ArtPauseBack->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Settings gear is hidden during confirmation"),
	          PauseSettingsButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Exit-to-menu confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	RestartWidget->SetQuitConfirmation(true, false);
	TestEqual(TEXT("Exit-game confirmation title"), TitleText->GetText().ToString(), FString(TEXT("退出游戏?")));
	TestEqual(TEXT("Exit-game confirmation has no dialog plate"),
	          ArtRestartDialogPanel->GetVisibility(),
	          ESlateVisibility::Hidden);

	return true;
}
