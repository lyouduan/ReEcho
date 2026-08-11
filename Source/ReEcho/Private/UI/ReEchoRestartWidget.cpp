#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
UButton* AddMenuButton(UWidgetTree* WidgetTree,
                       UVerticalBox* Content,
                       const FName ButtonName,
                       const FString& Label,
                       const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	Button->SetBackgroundColor(Color);
	UVerticalBoxSlot* ButtonSlot = Content->AddChildToVerticalBox(Button);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetPadding(FMargin(0.0f, 5.0f));

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonLabel->SetText(FText::FromString(Label));
	ButtonLabel->SetJustification(ETextJustify::Center);
	ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonLabel->SetMargin(FMargin(42.0f, 12.0f));
	FSlateFontInfo ButtonFont = ButtonLabel->GetFont();
	ButtonFont.Size = 24;
	ButtonLabel->SetFont(ButtonFont);
	Button->SetContent(ButtonLabel);
	return Button;
}
}

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
	if ((bDeathScreen || bVictoryScreen) && RestartButton)
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
	bDeathScreen = bInDeathScreen;
	bVictoryScreen = false;
	bQuitConfirmation = false;
	bSaveFailed = false;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetVictoryScreen(const int32 TimeShards, const int32 TraitCount)
{
	bDeathScreen = false;
	bVictoryScreen = true;
	bQuitConfirmation = false;
	bSaveFailed = false;
	VictoryTimeShards = TimeShards;
	VictoryTraitCount = TraitCount;
	RefreshMenuMode();
}

void UReEchoRestartWidget::SetQuitConfirmation(const bool bInQuitConfirmation)
{
	bQuitConfirmation = bInQuitConfirmation;
	bSaveFailed = false;
	RefreshMenuMode();
	if (bQuitConfirmation && ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::ShowSaveFailure()
{
	bSaveFailed = true;
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

	ResumeButton = AddMenuButton(
	    WidgetTree, Content, TEXT("ResumeButton"), TEXT("返回游戏"), FLinearColor(0.08f, 0.42f, 0.32f, 1.0f));
	RestartButton = AddMenuButton(
	    WidgetTree, Content, TEXT("RestartButton"), TEXT("重新开始"), FLinearColor(0.65f, 0.18f, 0.06f, 1.0f));
	SettingsButton = AddMenuButton(
	    WidgetTree, Content, TEXT("SettingsButton"), TEXT("游戏设置"), FLinearColor(0.16f, 0.22f, 0.34f, 1.0f));
	QuitButton = AddMenuButton(
	    WidgetTree, Content, TEXT("QuitButton"), TEXT("退出游戏"), FLinearColor(0.55f, 0.05f, 0.08f, 1.0f));
	QuitButtonText = Cast<UTextBlock>(QuitButton->GetContent());
	RefreshMenuMode();
}

void UReEchoRestartWidget::RefreshMenuMode()
{
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
