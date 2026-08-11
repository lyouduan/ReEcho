#include "UI/ReEchoStartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
UButton* AddStartButton(UWidgetTree* WidgetTree,
                        UVerticalBox* Content,
                        const FName ButtonName,
                        const FString& Label,
                        const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	Button->SetBackgroundColor(Color);
	UVerticalBoxSlot* ButtonSlot = Content->AddChildToVerticalBox(Button);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetPadding(FMargin(0.0f, 7.0f));

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonLabel->SetText(FText::FromString(Label));
	ButtonLabel->SetJustification(ETextJustify::Center);
	ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonLabel->SetMargin(FMargin(58.0f, 14.0f));
	FSlateFontInfo ButtonFont = ButtonLabel->GetFont();
	ButtonFont.Size = 26;
	ButtonLabel->SetFont(ButtonFont);
	Button->SetContent(ButtonLabel);
	return Button;
}
}

TSharedRef<SWidget> UReEchoStartMenuWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UReEchoStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildWidgetTree();
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleContinueClicked);
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleNewGameClicked);
	}
	if (GameSettingsButton)
	{
		GameSettingsButton->OnClicked.AddUniqueDynamic(this, &UReEchoStartMenuWidget::HandleGameSettingClicked);
	}
	RefreshMenu();
	if (bHasSavedRun && ContinueButton)
	{
		ContinueButton->SetKeyboardFocus();
	}
	else if (NewGameButton)
	{
		NewGameButton->SetKeyboardFocus();
	}
	else if (GameSettingsButton)
	{
		GameSettingsButton->SetKeyboardFocus();
	}
}

void UReEchoStartMenuWidget::InitializeMenu(const bool bInHasSavedRun)
{
	bHasSavedRun = bInHasSavedRun;
	RefreshMenu();
}

void UReEchoStartMenuWidget::BuildWidgetTree()
{
	if (NewGameButton || !WidgetTree)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StartBackground"));
	Background->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.035f, 0.95f));
	Background->SetPadding(FMargin(140.0f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content =
	    WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StartContent"));
	Background->SetContent(Content);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartTitle"));
	TitleText->SetText(NSLOCTEXT("ReEcho", "StartTitle", "时间回响"));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.88f, 1.0f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 54;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartStatus"));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 21;
	StatusText->SetFont(StatusFont);
	UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 30.0f));

	ContinueButton = AddStartButton(
	    WidgetTree, Content, TEXT("ContinueButton"), TEXT("继续游戏"), FLinearColor(0.08f, 0.42f, 0.32f, 1.0f));
	NewGameButton = AddStartButton(
	    WidgetTree, Content, TEXT("NewGameButton"), TEXT("新开始游戏"), FLinearColor(0.12f, 0.32f, 0.62f, 1.0f));
	GameSettingsButton = AddStartButton(
	    WidgetTree, Content, TEXT("GameSettingsButton"), TEXT("游戏设置"), FLinearColor(0.12f, 0.32f, 0.62f, 1.0f));
	RefreshMenu();
}

void UReEchoStartMenuWidget::RefreshMenu()
{
	if (StatusText)
	{
		StatusText->SetText(bHasSavedRun ? NSLOCTEXT("ReEcho", "SavedRunFound", "检测到未完成的时间线")
		                                 : NSLOCTEXT("ReEcho", "NoSavedRun", "开始一条新的时间线"));
	}
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(bHasSavedRun ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UReEchoStartMenuWidget::HandleNewGameClicked()
{
	OnNewGameRequested.Broadcast();
}

void UReEchoStartMenuWidget::HandleContinueClicked()
{
	OnContinueGameRequested.Broadcast();
}

void UReEchoStartMenuWidget::HandleGameSettingClicked()
{
	OnGameSettingRequested.Broadcast();
}
