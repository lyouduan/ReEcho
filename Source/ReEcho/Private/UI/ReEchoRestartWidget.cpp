#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ReEchoMenuWidgetHelpers.h"

TSharedRef<SWidget> UReEchoRestartWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoRestartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();

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

void UReEchoRestartWidget::SetQuitConfirmation(const bool bInQuitConfirmation)
{
	QuitPromptState = bInQuitConfirmation ? EReEchoQuitPromptState::Confirm : EReEchoQuitPromptState::None;
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

void UReEchoRestartWidget::BuildWidgetTree()
{
	if (RestartButton || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.92f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	Background->SetContent(Content);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuTitle"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuMessage"));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MessageText->SetJustification(ETextJustify::Center);
	FSlateFontInfo MessageFont = MessageText->GetFont();
	MessageFont.Size = 21;
	MessageText->SetFont(MessageFont);
	UVerticalBoxSlot* MessageSlot = Content->AddChildToVerticalBox(MessageText);
	MessageSlot->SetHorizontalAlignment(HAlign_Center);
	MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

	ReEcho::UI::FMenuButtonStyle ButtonStyle{
	    FLinearColor(0.08f, 0.42f, 0.32f, 1.0f), FMargin(0.0f, 5.0f), FMargin(42.0f, 12.0f), 24};
	ResumeButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *Content, TEXT("ResumeButton"), FText::FromString(TEXT("返回游戏")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.65f, 0.18f, 0.06f, 1.0f);
	RestartButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *Content, TEXT("RestartButton"), FText::FromString(TEXT("重新开始")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.16f, 0.22f, 0.34f, 1.0f);
	SettingsButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *Content, TEXT("SettingsButton"), FText::FromString(TEXT("游戏设置")), ButtonStyle);
	ButtonStyle.Color = FLinearColor(0.55f, 0.05f, 0.08f, 1.0f);
	QuitButton = ReEcho::UI::AddMenuButton(
	    *WidgetTree, *Content, TEXT("QuitButton"), FText::FromString(TEXT("退出游戏")), ButtonStyle);
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
		                      : bQuitConfirmation ? TEXT("确认退出游戏")
		                                          : TEXT("游戏菜单");
		const FLinearColor TitleColor =
		    bVictoryScreen ? FLinearColor(1.0f, 0.78f, 0.16f)
		                   : (bDeathScreen ? FLinearColor(0.95f, 0.12f, 0.12f) : FLinearColor(0.4f, 0.85f, 1.0f));
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
			MessageText->SetText(FText::FromString(TEXT("确认退出后将保存当前局内状态")));
		}
		else
		{
			MessageText->SetText(FText::FromString(bDeathScreen ? TEXT("玩家已阵亡，本次时间线结束")
			                                                    : TEXT("游戏已暂停 · 按 Esc 可继续")));
		}
	}
	if (ResumeButton)
	{
		ResumeButton->SetVisibility(bDeathScreen || bVictoryScreen ? ESlateVisibility::Collapsed
		                                                           : ESlateVisibility::Visible);
	}
	if (RestartButton)
	{
		RestartButton->SetVisibility(bQuitConfirmation || bSaveFailed ? ESlateVisibility::Collapsed
		                                                              : ESlateVisibility::Visible);
	}
	if (SettingsButton)
	{
		const bool bShowSettingsEntry = !bDeathScreen && !bVictoryScreen && !bQuitConfirmation && !bSaveFailed;
		SettingsButton->SetVisibility(bShowSettingsEntry ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (QuitButtonText)
	{
		const FString QuitLabel = bQuitConfirmation ? TEXT("确认退出")
		                          : bSaveFailed     ? TEXT("重试退出")
		                                            : TEXT("退出游戏");
		QuitButtonText->SetText(FText::FromString(QuitLabel));
	}
}

void UReEchoRestartWidget::HandleResumeClicked()
{
	OnResumeRequested.Broadcast();
}

void UReEchoRestartWidget::HandleRestartClicked()
{
	OnRestartRequested.Broadcast();
}

void UReEchoRestartWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

void UReEchoRestartWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}
