#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/ReEchoBalanceSettings.h"
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
	TestNull(TEXT("Legacy result character is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtRestartCharacter")));
	TestNull(TEXT("Legacy result summary panel is removed"),
	         RestartWidget->GetWidgetFromName(TEXT("ArtResultSummaryPanel")));
	TestNull(TEXT("Legacy selected-cards panel is removed"),
	         RestartWidget->GetWidgetFromName(TEXT("ArtSelectedCardsPanel")));
	TestNull(TEXT("Legacy victory title is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtVictoryTitle")));
	TestNull(TEXT("Legacy defeat title is removed"), RestartWidget->GetWidgetFromName(TEXT("ArtDefeatTitle")));

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
	UCanvasPanel* VictoryCanvas = FindRestartWidget<UCanvasPanel>(RestartWidget, TEXT("VictoryCanvas"));
	UTextBlock* VictoryEncounterValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryEncounterValue"));
	UTextBlock* VictoryTimeShardsValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryTimeShardsValue"));
	UTextBlock* VictoryTraitCountValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryTraitCountValue"));
	UButton* VictoryContinueButton = FindRestartWidget<UButton>(RestartWidget, TEXT("VictoryContinueButton"));
	UCanvasPanel* DefeatCanvas = FindRestartWidget<UCanvasPanel>(RestartWidget, TEXT("DefeatCanvas"));
	UTextBlock* DefeatEncounterValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatEncounterValue"));
	UTextBlock* DefeatTimeShardsValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatTimeShardsValue"));
	UTextBlock* DefeatTraitCountValue = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatTraitCountValue"));
	UButton* DefeatRestartButton = FindRestartWidget<UButton>(RestartWidget, TEXT("DefeatRestartButton"));
	UButton* DefeatMainMenuButton = FindRestartWidget<UButton>(RestartWidget, TEXT("DefeatMainMenuButton"));
	UImage* ArtVictoryContinueButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtVictoryContinueButtonFormal"));
	UImage* ArtDefeatRestartButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatRestartButtonFormal"));
	UImage* ArtDefeatMainMenuButtonFormal =
	    FindRestartWidget<UImage>(RestartWidget, TEXT("ArtDefeatMainMenuButtonFormal"));
	UTextBlock* VictoryContinueLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("VictoryContinueLabel"));
	UTextBlock* DefeatRestartLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatRestartLabel"));
	UTextBlock* DefeatMainMenuLabel = FindRestartWidget<UTextBlock>(RestartWidget, TEXT("DefeatMainMenuLabel"));

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
	TestNotNull(TEXT("Formal victory canvas exists"), VictoryCanvas);
	TestNotNull(TEXT("Formal victory encounter value exists"), VictoryEncounterValue);
	TestNotNull(TEXT("Formal victory shard value exists"), VictoryTimeShardsValue);
	TestNotNull(TEXT("Formal victory build value exists"), VictoryTraitCountValue);
	TestNotNull(TEXT("Formal victory continue button exists"), VictoryContinueButton);
	TestNotNull(TEXT("Formal defeat canvas exists"), DefeatCanvas);
	TestNotNull(TEXT("Formal defeat encounter value exists"), DefeatEncounterValue);
	TestNotNull(TEXT("Formal defeat shard value exists"), DefeatTimeShardsValue);
	TestNotNull(TEXT("Formal defeat build value exists"), DefeatTraitCountValue);
	TestNotNull(TEXT("Formal defeat restart button exists"), DefeatRestartButton);
	TestNotNull(TEXT("Formal defeat main-menu button exists"), DefeatMainMenuButton);
	TestNotNull(TEXT("Formal victory continue art exists"), ArtVictoryContinueButtonFormal);
	TestNotNull(TEXT("Formal defeat restart art exists"), ArtDefeatRestartButtonFormal);
	TestNotNull(TEXT("Formal defeat main-menu art exists"), ArtDefeatMainMenuButtonFormal);
	TestNotNull(TEXT("Formal victory continue label exists"), VictoryContinueLabel);
	TestNotNull(TEXT("Formal defeat restart label exists"), DefeatRestartLabel);
	TestNotNull(TEXT("Formal defeat main-menu label exists"), DefeatMainMenuLabel);
	if (!TitleText || !RootPanel || !ResumeButton || !RestartButton || !QuitButton || !PauseSettingsButton ||
	    !ArtPauseDimmer || !ArtPausePrimaryButton || !ArtPauseSecondaryButton || !ArtPauseTertiaryButton ||
	    !ArtRestartDialogPanel || !VictoryCanvas || !VictoryEncounterValue || !VictoryTimeShardsValue ||
	    !VictoryTraitCountValue || !VictoryContinueButton || !DefeatCanvas || !DefeatEncounterValue ||
	    !DefeatTimeShardsValue || !DefeatTraitCountValue || !DefeatRestartButton || !DefeatMainMenuButton ||
	    !ArtVictoryContinueButtonFormal || !ArtDefeatRestartButtonFormal || !ArtDefeatMainMenuButtonFormal ||
	    !VictoryContinueLabel || !DefeatRestartLabel || !DefeatMainMenuLabel)
	{
		return false;
	}

	const FVector2D DefeatRestartRestingScale = ArtDefeatRestartButtonFormal->GetRenderTransform().Scale;
	const FVector2D DefeatRestartLabelRestingScale = DefeatRestartLabel->GetRenderTransform().Scale;
	DefeatRestartButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal defeat restart art scales on hover"),
	         ArtDefeatRestartButtonFormal->GetRenderTransform().Scale.X > DefeatRestartRestingScale.X);
	TestTrue(TEXT("Formal defeat restart label scales with its art"),
	         DefeatRestartLabel->GetRenderTransform().Scale.X > DefeatRestartLabelRestingScale.X);
	DefeatRestartButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal defeat restart art restores after hover"),
	         ArtDefeatRestartButtonFormal->GetRenderTransform().Scale.Equals(DefeatRestartRestingScale));

	const FVector2D DefeatMainMenuRestingScale = ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale;
	const FVector2D DefeatMainMenuLabelRestingScale = DefeatMainMenuLabel->GetRenderTransform().Scale;
	DefeatMainMenuButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal defeat main-menu art scales on hover"),
	         ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale.X > DefeatMainMenuRestingScale.X);
	TestTrue(TEXT("Formal defeat main-menu label scales with its art"),
	         DefeatMainMenuLabel->GetRenderTransform().Scale.X > DefeatMainMenuLabelRestingScale.X);
	DefeatMainMenuButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal defeat main-menu art restores after hover"),
	         ArtDefeatMainMenuButtonFormal->GetRenderTransform().Scale.Equals(DefeatMainMenuRestingScale));

	const FVector2D VictoryContinueRestingScale = ArtVictoryContinueButtonFormal->GetRenderTransform().Scale;
	const FVector2D VictoryContinueLabelRestingScale = VictoryContinueLabel->GetRenderTransform().Scale;
	VictoryContinueButton->OnHovered.Broadcast();
	TestTrue(TEXT("Formal victory continue art scales on hover"),
	         ArtVictoryContinueButtonFormal->GetRenderTransform().Scale.X > VictoryContinueRestingScale.X);
	TestTrue(TEXT("Formal victory continue label scales with its art"),
	         VictoryContinueLabel->GetRenderTransform().Scale.X > VictoryContinueLabelRestingScale.X);
	VictoryContinueButton->OnUnhovered.Broadcast();
	TestTrue(TEXT("Formal victory continue art restores after hover"),
	         ArtVictoryContinueButtonFormal->GetRenderTransform().Scale.Equals(VictoryContinueRestingScale));

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
	TestEqual(
	    TEXT("Primary art is visible"), ArtPausePrimaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Secondary art is visible"), ArtPauseSecondaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(
	    TEXT("Tertiary art is visible"), ArtPauseTertiaryButton->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Primary art retains its source width"), ArtPausePrimaryButton->GetBrush().ImageSize.X >= 420.0f);
	TestTrue(TEXT("Primary art preserves the formal aspect height"),
	         ArtPausePrimaryButton->GetBrush().ImageSize.Y >= 140.0f);

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

	RestartWidget->SetDeathScreen(true, 4, 126, 5);
	TestEqual(TEXT("Formal defeat canvas is visible"), DefeatCanvas->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Formal defeat encounter value is projected"),
	          DefeatEncounterValue->GetText().ToString(),
	          FString(TEXT("4")));
	TestEqual(TEXT("Formal defeat shard value is projected"),
	          DefeatTimeShardsValue->GetText().ToString(),
	          FString(TEXT("126")));
	TestEqual(TEXT("Formal defeat build value is projected"),
	          DefeatTraitCountValue->GetText().ToString(),
	          FString(TEXT("5")));
	TestEqual(TEXT("Formal defeat restart button is interactive"),
	          DefeatRestartButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Formal defeat main-menu button is interactive"),
	          DefeatMainMenuButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Legacy restart action is hidden during formal defeat"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Legacy quit action is hidden during formal defeat"),
	          QuitButton->GetVisibility(),
	          ESlateVisibility::Collapsed);

	RestartWidget->SetVictoryScreen(126, 5);
	TestEqual(TEXT("Formal defeat canvas is hidden during victory"),
	          DefeatCanvas->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Formal victory canvas is visible"), VictoryCanvas->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Formal victory encounter is real balance data"),
	          VictoryEncounterValue->GetText().ToString(),
	          FText::AsNumber(GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount()).ToString());
	TestEqual(TEXT("Formal victory shard value is projected"),
	          VictoryTimeShardsValue->GetText().ToString(),
	          FString(TEXT("126")));
	TestEqual(TEXT("Formal victory build value is projected"),
	          VictoryTraitCountValue->GetText().ToString(),
	          FString(TEXT("5")));
	TestEqual(TEXT("Formal victory continue button is interactive"),
	          VictoryContinueButton->GetVisibility(),
	          ESlateVisibility::Visible);
	TestEqual(TEXT("Legacy result action is hidden during formal victory"),
	          RestartButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Legacy exit action is hidden during formal victory"),
	          QuitButton->GetVisibility(),
	          ESlateVisibility::Collapsed);
	TestEqual(TEXT("Pause composition is hidden during formal victory"),
	          ArtPauseDimmer->GetVisibility(),
	          ESlateVisibility::Hidden);

	return true;
}
