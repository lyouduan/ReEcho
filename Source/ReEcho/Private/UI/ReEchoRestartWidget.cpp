#include "UI/ReEchoRestartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UReEchoRestartWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoRestartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UReEchoRestartWidget::HandleRestartClicked);
		RestartButton->SetKeyboardFocus();
	}
}

void UReEchoRestartWidget::BuildWidgetTree()
{
	if (RestartButton || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("RestartBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.92f));
	Background->SetPadding(FMargin(120.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("RestartContent"));
	Background->SetContent(Content);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DeathTitle"));
	Title->SetText(FText::FromString(TEXT("回响中断")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.12f, 0.12f)));
	Title->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 44;
	Title->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DeathMessage"));
	Message->SetText(FText::FromString(TEXT("玩家已阵亡，本次时间线结束")));
	Message->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Message->SetJustification(ETextJustify::Center);
	FSlateFontInfo MessageFont = Message->GetFont();
	MessageFont.Size = 22;
	Message->SetFont(MessageFont);
	UVerticalBoxSlot* MessageSlot = Content->AddChildToVerticalBox(Message);
	MessageSlot->SetHorizontalAlignment(HAlign_Center);
	MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 36.0f));

	RestartButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("RestartButton"));
	RestartButton->SetBackgroundColor(FLinearColor(0.7f, 0.05f, 0.05f, 1.0f));
	UVerticalBoxSlot* ButtonSlot = Content->AddChildToVerticalBox(RestartButton);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("RestartButtonLabel"));
	ButtonLabel->SetText(FText::FromString(TEXT("重新开始")));
	ButtonLabel->SetJustification(ETextJustify::Center);
	ButtonLabel->SetMargin(FMargin(36.0f, 12.0f));
	FSlateFontInfo ButtonFont = ButtonLabel->GetFont();
	ButtonFont.Size = 24;
	ButtonLabel->SetFont(ButtonFont);
	RestartButton->SetContent(ButtonLabel);
}

void UReEchoRestartWidget::HandleRestartClicked()
{
	OnRestartRequested.Broadcast();
}