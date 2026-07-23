#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
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
	RefreshMenuMode();
	if (bDeathScreen && RestartButton)
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
	    WidgetTree, Content, TEXT("ResumeButton"), TEXT("继续游戏"), FLinearColor(0.08f, 0.42f, 0.32f, 1.0f));
	RestartButton = AddMenuButton(
	    WidgetTree, Content, TEXT("RestartButton"), TEXT("重新开始"), FLinearColor(0.65f, 0.18f, 0.06f, 1.0f));
	QuitButton = AddMenuButton(
	    WidgetTree, Content, TEXT("QuitButton"), TEXT("退出游戏"), FLinearColor(0.55f, 0.05f, 0.08f, 1.0f));
	RefreshMenuMode();
}

void UReEchoRestartWidget::RefreshMenuMode()
{
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(bDeathScreen ? TEXT("回响中断") : TEXT("游戏菜单")));
		TitleText->SetColorAndOpacity(
		    FSlateColor(bDeathScreen ? FLinearColor(0.95f, 0.12f, 0.12f) : FLinearColor(0.4f, 0.85f, 1.0f)));
	}
	if (MessageText)
	{
		MessageText->SetText(
		    FText::FromString(bDeathScreen ? TEXT("玩家已阵亡，本次时间线结束") : TEXT("游戏已暂停 · 按 Esc 可继续")));
	}
	if (ResumeButton)
	{
		ResumeButton->SetVisibility(bDeathScreen ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
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