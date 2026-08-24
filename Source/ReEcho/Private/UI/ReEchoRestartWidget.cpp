#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoMenuWidgetHelpers.h"

TSharedRef<SWidget> UReEchoRestartWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UReEchoRestartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	EnsureAttackModeWidget();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleResumeClicked);
	}
	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleRestartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleQuitClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleSettingsClicked);
	}
	if (PauseSettingsButton)
	{
		PauseSettingsButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleSettingsClicked);
	}
	RefreshMenuMode();
	if (ScreenMode != EReEchoRestartScreenMode::Pause && RestartButton)
	{
		RestartButton->SetKeyboardFocus();
	}
	else if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::SetDeathScreen(const bool bInDeathScreen)
{
	ScreenMode = bInDeathScreen ? EReEchoRestartScreenMode::Death : EReEchoRestartScreenMode::Pause;
	QuitPromptState = EReEchoQuitPromptState::None;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetVictoryScreen(const int32 TimeShards, const int32 TraitCount)
{
	ScreenMode = EReEchoRestartScreenMode::Victory;
	QuitPromptState = EReEchoQuitPromptState::None;
	VictoryTimeShards = TimeShards;
	VictoryTraitCount = TraitCount;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetQuitConfirmation(const bool bInQuitConfirmation,
                                               const bool bInExitToMainMenu,
                                               const int32 InEncounterIndex)
{
	QuitPromptState = bInQuitConfirmation ? EReEchoQuitPromptState::Confirm : EReEchoQuitPromptState::None;
	bExitToMainMenu = bInQuitConfirmation && bInExitToMainMenu;
	PauseEncounterIndex = bInQuitConfirmation ? FMath::Max(0, InEncounterIndex) : 0;
	RefreshMenuMode();
	if (QuitPromptState == EReEchoQuitPromptState::Confirm && ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::ShowSaveFailure()
{
	QuitPromptState = EReEchoQuitPromptState::SaveFailed;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetAutomaticAttackMode(const bool bAutomatic)
{
	bAutomaticAttackMode = bAutomatic;
	EnsureAttackModeWidget();
	if (AttackModeWidget)
	{
		AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
	}
}

void UReEchoRestartWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.92f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	MenuContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	Background->SetContent(MenuContent);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuTitle"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = MenuContent->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuMessage"));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MessageText->SetJustification(ETextJustify::Center);
	FSlateFontInfo MessageFont = MessageText->GetFont();
	MessageFont.Size = 21;
	MessageText->SetFont(MessageFont);
	UVerticalBoxSlot* MessageSlot = MenuContent->AddChildToVerticalBox(MessageText);
	MessageSlot->SetHorizontalAlignment(HAlign_Center);
	MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

	ReEcho::UI::FMenuButtonStyle ButtonStyle{
	    FLinearColor(0.08f, 0.42f, 0.32f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	ResumeButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("ResumeButton"), FText::FromString(TEXT("返回游戏")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.65f, 0.18f, 0.06f, 1.0f);
	RestartButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("RestartButton"), FText::FromString(TEXT("重新开始")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.16f, 0.22f, 0.34f, 1.0f);
	SettingsButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("SettingsButton"), FText::FromString(TEXT("游戏设置")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.55f, 0.05f, 0.08f, 1.0f);
	QuitButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *MenuContent, TEXT("QuitButton"), FText::FromString(TEXT("退出游戏")), ButtonStyle);
	QuitButtonText = Cast<UTextBlock>(QuitButton->GetContent());
	RefreshMenuMode();
}

void UReEchoRestartWidget::RefreshMenuMode()
{
	const bool bVictoryScreen = ScreenMode == EReEchoRestartScreenMode::Victory;
	const bool bDeathScreen = ScreenMode == EReEchoRestartScreenMode::Death;
	const bool bQuitConfirmation = QuitPromptState == EReEchoQuitPromptState::Confirm;
	const bool bSaveFailed = QuitPromptState == EReEchoQuitPromptState::SaveFailed;

	if (TitleText)
	{
		const FString Title = bVictoryScreen      ? TEXT("时间线收束")
		                      : bDeathScreen      ? TEXT("回响中断")
		                      : bSaveFailed       ? TEXT("保存失败")
		                      : bQuitConfirmation ? (bExitToMainMenu ? TEXT("退出到主菜单?") : TEXT("退出游戏?"))
		                                          : TEXT("游戏暂停");
		const FLinearColor TitleColor = bVictoryScreen
		                                    ? FLinearColor(1.0f, 0.78f, 0.16f)
		                                    : (bDeathScreen ? FLinearColor(0.95f, 0.12f, 0.12f) : FLinearColor::White);
		TitleText->SetText(FText::FromString(Title));
		TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
	}
	if (MessageText)
	{
		if (bVictoryScreen)
		{
			MessageText->SetText(FText::Format(
			    NSLOCTEXT("ReEcho", "VictorySummary", "Boss 已击败 · 完成 {0}/{0}\n时间碎片：{1} · 强化数量：{2}"),
			    FText::AsNumber(GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount()),
			    FText::AsNumber(VictoryTimeShards),
			    FText::AsNumber(VictoryTraitCount)));
		}
		else if (bSaveFailed)
		{
			MessageText->SetText(FText::FromString(TEXT("未能保存退出前状态，游戏不会退出，请重试")));
		}
		else if (bQuitConfirmation)
		{
			MessageText->SetText(FText::Format(NSLOCTEXT("ReEcho", "PauseSavePoint", "存档点：第 {0} 关"),
			                                   FText::AsNumber(PauseEncounterIndex)));
		}
		else
		{
			MessageText->SetText(FText::FromString(bDeathScreen ? TEXT("玩家已阵亡，本次时间线结束")
			                                                    : TEXT("游戏已暂停 · 按 P 可继续")));
		}
		MessageText->SetVisibility(!bDeathScreen && !bVictoryScreen && !bQuitConfirmation && !bSaveFailed
		                               ? ESlateVisibility::Collapsed
		                               : ESlateVisibility::HitTestInvisible);
	}
	const bool bPauseMenu = ScreenMode == EReEchoRestartScreenMode::Pause;
	const bool bPausePrompt = bPauseMenu && (bQuitConfirmation || bSaveFailed);
	const bool bPauseRootVisible = bPauseMenu;
	if (ArtPauseDimmer)
	{
		ArtPauseDimmer->SetVisibility(bPauseRootVisible ? ESlateVisibility::HitTestInvisible
		                                                : ESlateVisibility::Hidden);
	}
	auto SetPauseArtVisibility = [](UImage* Image, const bool bVisible)
	{
		if (Image)
		{
			Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};
	SetPauseArtVisibility(ArtPausePrimaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseSecondaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseTertiaryButton, bPauseMenu);
	SetPauseArtVisibility(ArtPauseSettings, bPauseMenu && !bPausePrompt);
	const ESlateVisibility ResultArtVisibility =
	    bVictoryScreen || bDeathScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (ArtRestartDialogPanel)
	{
		// The pause exit prompts are composed directly over the dimmed game scene.
		// The restart-dialog plate belongs to the save-failure fallback, not to either exit confirmation.
		ArtRestartDialogPanel->SetVisibility(bSaveFailed ? ESlateVisibility::HitTestInvisible
		                                                 : ESlateVisibility::Hidden);
	}
	if (ArtRestartCharacter)
	{
		ArtRestartCharacter->SetVisibility(ResultArtVisibility);
	}
	if (ArtResultSummaryPanel)
	{
		ArtResultSummaryPanel->SetVisibility(ResultArtVisibility);
	}
	if (ArtSelectedCardsPanel)
	{
		ArtSelectedCardsPanel->SetVisibility(ResultArtVisibility);
	}
	if (ArtVictoryTitle)
	{
		ArtVictoryTitle->SetVisibility(bVictoryScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (ArtDefeatTitle)
	{
		ArtDefeatTitle->SetVisibility(bDeathScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (ResumeButton)
	{
		ResumeButton->SetVisibility(bDeathScreen || bVictoryScreen ? ESlateVisibility::Collapsed
		                                                           : ESlateVisibility::Visible);
	}
	if (RestartButton)
	{
		RestartButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (SettingsButton)
	{
		const bool bShowSettingsEntry =
		    !PauseSettingsButton && !bDeathScreen && !bVictoryScreen && !bQuitConfirmation && !bSaveFailed;
		SettingsButton->SetVisibility(bShowSettingsEntry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PauseSettingsButton)
	{
		PauseSettingsButton->SetVisibility(bPauseMenu && !bPausePrompt ? ESlateVisibility::Visible
		                                                               : ESlateVisibility::Collapsed);
	}
	const FLinearColor ButtonBackground = bPauseMenu ? FLinearColor::Transparent : FLinearColor::White;
	for (UButton* Button : {ResumeButton.Get(), RestartButton.Get(), QuitButton.Get()})
	{
		if (Button)
		{
			Button->SetBackgroundColor(ButtonBackground);
		}
	}
	if (ResumeButtonLabel)
	{
		ResumeButtonLabel->SetText(FText::FromString(bPausePrompt ? TEXT("保存并退出") : TEXT("继续游戏")));
		ResumeButtonLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (RestartButtonLabel)
	{
		const FString RestartLabel = bPausePrompt      ? TEXT("不保存并退出")
		                             : bPauseMenu      ? TEXT("退出至主菜单")
		                                               : TEXT("重新开始");
		RestartButtonLabel->SetText(FText::FromString(RestartLabel));
		RestartButtonLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (QuitButtonText)
	{
		const FString QuitLabel = bPausePrompt ? TEXT("返回") : TEXT("退出游戏");
		QuitButtonText->SetText(FText::FromString(QuitLabel));
		QuitButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (AttackModeWidget)
	{
		const bool bShowAttackMode = ScreenMode == EReEchoRestartScreenMode::Pause &&
		                             QuitPromptState == EReEchoQuitPromptState::None && !ArtPausePrimaryButton;
		AttackModeWidget->SetVisibility(bShowAttackMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
	}
}

void UReEchoRestartWidget::EnsureAttackModeWidget()
{
	if (AttackModeWidget || !WidgetTree)
	{
		return;
	}
	if (!MenuContent)
	{
		MenuContent = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("MenuContent")));
	}
	if (!MenuContent && SettingsButton)
	{
		MenuContent = Cast<UVerticalBox>(SettingsButton->GetParent());
	}
	if (!MenuContent)
	{
		return;
	}
	AttackModeWidget = WidgetTree->ConstructWidget<UReEchoAttackModeWidget>(UReEchoAttackModeWidget::StaticClass(),
	                                                                        TEXT("AttackModePanel"));
	MenuContent->AddChildToVerticalBox(AttackModeWidget)->SetPadding(FMargin(4.0f, 8.0f));
	AttackModeWidget->OnAutomaticRequested.AddDynamic(this, &UReEchoRestartWidget::HandleAutomaticAttackClicked);
	AttackModeWidget->OnManualRequested.AddDynamic(this, &UReEchoRestartWidget::HandleManualAttackClicked);
	AttackModeWidget->SetAutomaticMode(bAutomaticAttackMode);
}

void UReEchoRestartWidget::HandleResumeClicked()
{
	if (QuitPromptState == EReEchoQuitPromptState::None)
	{
		OnResumeRequested.Broadcast();
		return;
	}
	OnQuitRequested.Broadcast();
}

void UReEchoRestartWidget::HandleRestartClicked()
{
	if (QuitPromptState != EReEchoQuitPromptState::None)
	{
		OnExitWithoutSavingRequested.Broadcast();
		return;
	}
	if (ScreenMode == EReEchoRestartScreenMode::Pause)
	{
		OnExitToMainMenuRequested.Broadcast();
		return;
	}
	OnRestartRequested.Broadcast();
}

void UReEchoRestartWidget::HandleQuitClicked()
{
	if (QuitPromptState != EReEchoQuitPromptState::None)
	{
		OnCancelExitRequested.Broadcast();
		return;
	}
	OnQuitRequested.Broadcast();
}

void UReEchoRestartWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}

void UReEchoRestartWidget::HandleAutomaticAttackClicked()
{
	OnAutomaticAttackRequested.Broadcast();
}

void UReEchoRestartWidget::HandleManualAttackClicked()
{
	OnManualAttackRequested.Broadcast();
}
